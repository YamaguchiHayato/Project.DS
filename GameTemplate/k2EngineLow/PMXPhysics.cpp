#include "k2EngineLowPreCompile.h"
#include "PMXPhysics.h"
#include "pmxLoader.h"
#include "BoneNode.h"

using namespace nsK2EngineLow;
using namespace DirectX;

// ヘルパー：DirectX::XMFLOAT3 -> btVector3
static btVector3 ToBtVec(const XMFLOAT3& v) {
    return btVector3(v.x, v.y, v.z);
}
// ヘルパー：Vector3 -> btVector3
static btVector3 ToBtVec(const Vector3& v) {
    return btVector3(v.x, v.y, v.z);
}
// ヘルパー：btVector3 -> DirectX::XMFLOAT3
static XMFLOAT3 ToDxVec(const btVector3& v) {
    return XMFLOAT3(v.x(), v.y(), v.z());
}

// ヘルパー：回転変換（シンプルで安定する版）
static btQuaternion ToBtQuat(const XMFLOAT3& rot) {
    btQuaternion q;
    // MMD(PMX)のオイラー角をBulletのクォータニオンに変換
    // setEulerZYX(yaw, pitch, roll) 系の関数を使います
    q.setEulerZYX(rot.z, rot.y, rot.x);
    return q;
}



PMXPhysics::PMXPhysics()
{
}

PMXPhysics::~PMXPhysics()
{
    Cleanup();
}

void PMXPhysics::Cleanup()
{
    // ワールドから削除してメモリ解放
    if (m_dynamicsWorld) {
        for (auto* c : m_constraints) {
            m_dynamicsWorld->removeConstraint(c);
            delete c;
        }
        m_constraints.clear();

        for (auto& rbInfo : m_rigidBodies) {
            m_dynamicsWorld->removeRigidBody(rbInfo.body);
            delete rbInfo.body->getMotionState();
            delete rbInfo.body->getCollisionShape();
            delete rbInfo.body;
        }
        m_rigidBodies.clear();

        // 床の解放
        if (m_groundBody) {
            m_dynamicsWorld->removeRigidBody(m_groundBody);
            delete m_groundBody->getMotionState();
            delete m_groundBody->getCollisionShape();
            delete m_groundBody;
            m_groundBody = nullptr;
        }
    }

    // Core削除
    m_dynamicsWorld.reset();
    m_solver.reset();
    m_dispatcher.reset();
    m_collisionConfig.reset();
    m_broadphase.reset();

}

void PMXPhysics::Init(const PMXModel& model)
{
    Cleanup();

    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(), m_broadphase.get(), m_solver.get(), m_collisionConfig.get());

    auto& solverInfo = m_dynamicsWorld->getSolverInfo();
    solverInfo.m_numIterations = 30;
    // ★修正後：標準的な重力（-9.8）～少し強め（-20.0）くらいにする
    // 髪が落ち着くまで、まずは標準的な値に戻しましょう
    m_dynamicsWorld->setGravity(btVector3(0, -120.0f, 0)); // -19.6f くらいがMMDっぽい挙動になります

    // ★足元(y=0)に静的な床を追加。長い髪やスカートが床を貫通して垂れ下がるのを防ぐ。
    //   物理は足元原点のローカル空間で計算されるので、法線上向き・y=0の無限平面でよい。
    {
        btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0.0f);
        btTransform groundTransform;
        groundTransform.setIdentity();
        btDefaultMotionState* groundMotion = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo groundInfo(0.0f, groundMotion, groundShape, btVector3(0, 0, 0));
        groundInfo.m_friction = 0.5f;
        m_groundBody = new btRigidBody(groundInfo);
        // group/mask を全bitにして、すべての動的剛体（髪・スカート）と必ず衝突させる。
        // 静的同士（足などKinematic）は Bullet が解決しないので影響しない。
        m_dynamicsWorld->addRigidBody(m_groundBody, 0xFFFF, 0xFFFF);
    }

    // ★重要: DebugWireframe の初期化
    m_debugWireFrame.Init();

    // ★Bulletに登録（アドレスを渡すだけでOK）
    // DebugWireframe は btIDebugDraw を継承しているのでそのまま渡せます
    m_dynamicsWorld->setDebugDrawer(&m_debugWireFrame);
    // 2. 剛体 (RigidBody) の作成
    m_rigidBodies.reserve(model.rigidBodies.size());

    for (const auto& src : model.rigidBodies)
    {

        // ★修正1: 体(Static)のサイズ調整
        float shrinkScale = 1.0f;
        if (src.op == PMXRigidBody::Operation::Static) {
            // 1.0f から 1.2f に変更して、当たり判定を「太め」にする
            shrinkScale = 0.75f;
        }

        // 形状作成
        btCollisionShape* shape = nullptr;
        switch (src.shape) {
        case PMXRigidBody::Shape::Sphere:
            shape = new btSphereShape(src.shapeSize.x * shrinkScale);
            break;
        case PMXRigidBody::Shape::Box:
            shape = new btBoxShape(btVector3(
                src.shapeSize.x * shrinkScale,
                src.shapeSize.y * shrinkScale,
                src.shapeSize.z * shrinkScale));
            break;
        case PMXRigidBody::Shape::Capsule:
            shape = new btCapsuleShape(
                src.shapeSize.x * shrinkScale,
                src.shapeSize.y * shrinkScale);
            break;
        }

        if (!shape) continue;

        // マージン設定
        shape->setMargin(0.01f);

        // 初期トランスフォーム
        btTransform startTransform;
        startTransform.setIdentity();
        btVector3 localPos = ToBtVec(src.translate);
        btQuaternion localRot = ToBtQuat(src.rotate);
        startTransform.setOrigin(localPos);
        startTransform.setRotation(localRot);

        // 質量
        float mass = (src.op != PMXRigidBody::Operation::Static) ? src.mass : 0.0f;


        // ★修正: Dynamic（スカート等）の場合、質量を3倍にして重力を強く感じさせる
        if (mass > 0.0f) {
            mass *= 3.0f;
        }
        // ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
        btVector3 localInertia(0, 0, 0);
        if (mass > 0.0f) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        // Body作成
        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
        
        // ★対策3（追加）：空気抵抗を減らして、動きをキビキビさせる
        if (mass > 0.0f) {
            rbInfo.m_linearDamping = src.translateDimmer; // 抵抗半減
            rbInfo.m_angularDamping = src.rotateDimmer;
        }
        else {
            rbInfo.m_linearDamping = src.translateDimmer;
            rbInfo.m_angularDamping = src.rotateDimmer;
        }

        rbInfo.m_restitution = src.repulsion;

        // 摩擦設定
        if (src.op == PMXRigidBody::Operation::Static) {
            rbInfo.m_friction = 0.0f; // 足はツルツル
        }
        else {
            rbInfo.m_friction = src.friction;
        }
        btRigidBody* body = new btRigidBody(rbInfo);

        // 剛体のタイプに応じて、Bulletの設定を変える
        if (src.op == PMXRigidBody::Operation::Static) {
            // Type 0 (Bone追従):
            // これを入れないと「不動の壁」扱いになり、動かしても物理計算がついてきません
            body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

            // 勝手にスリープ（計算サボり）しないようにする
            body->setActivationState(DISABLE_DEACTIVATION);
        }
        else {
            // Type 1, 2 (物理演算):
            // 髪の毛なども、動きが止まった時に勝手にスリープすると困るので切っておく
            body->setActivationState(DISABLE_DEACTIVATION);
        }

        // ★修正3: CCD（すり抜け防止）を有効にする（重要）
             // これを入れると、足が速く動いてもスカートを貫通しなくなります
        if (src.op != PMXRigidBody::Operation::Static) {
            // Dynamic（スカートなど）
            body->setCcdMotionThreshold(0.05f);
            body->setCcdSweptSphereRadius(0.1f);
        }
        else {
            // Kinematic（足など）
            body->setCcdMotionThreshold(0.05f);
            body->setCcdSweptSphereRadius(0.1f);
        }

        // 修正前（今のコード）
  // int group = 1 << src.group;
  // int mask = 0xFFFF; // ← これが犯人！
  // m_dynamicsWorld->addRigidBody(body, group, mask);

  // 修正後（本来の姿に戻す）
        int group = 1 << src.group;
        int mask = src.collisionGroup; // PMXデータの指定に従う
        m_dynamicsWorld->addRigidBody(body, group, mask);

        // 管理配列に追加
        RigidBodyInfo info;
        info.body = body;
        info.boneIndex = src.boneIndex;
        info.type = (int)src.op;
        info.group = src.group;
        info.groupMask = src.collisionGroup;

        // オフセット計算
        info.boneOffset.setIdentity();
        if (src.boneIndex >= 0 && src.boneIndex < (int)model.bones.size()) {
            const auto& bone = model.bones[src.boneIndex];
            btVector3 bonePos = ToBtVec(bone.position);
            btTransform boneTr;
            boneTr.setIdentity();
            boneTr.setOrigin(bonePos);
            info.boneOffset = boneTr.inverse() * startTransform;
        }
        else {
            info.boneOffset = startTransform;
        }
        m_rigidBodies.push_back(info);
    }


    // 3. ジョイント (Constraint) の作成
    for (const auto& src : model.joints)
    {
        int idxA = src.rigidBodyAIndex;
        int idxB = src.rigidBodyBIndex;
        if (idxA < 0 || idxA >= (int)m_rigidBodies.size()) continue;
        if (idxB < 0 || idxB >= (int)m_rigidBodies.size()) continue;

        btRigidBody* rbA = m_rigidBodies[idxA].body;
        btRigidBody* rbB = m_rigidBodies[idxB].body;

        btTransform jointTr;
        jointTr.setIdentity();
        jointTr.setOrigin(ToBtVec(src.translate));
        jointTr.setRotation(ToBtQuat(src.rotate));

        btTransform transA = rbA->getWorldTransform().inverse() * jointTr;
        btTransform transB = rbB->getWorldTransform().inverse() * jointTr;

        btGeneric6DofSpringConstraint* dof6 = new btGeneric6DofSpringConstraint(
            *rbA, *rbB, transA, transB, true
        );

        // ★修正: 移動制限に「遊び」を持たせて、発狂を防ぐ
        btVector3 linearLower = ToBtVec(src.translateLowerLimit);
        btVector3 linearUpper = ToBtVec(src.translateUpperLimit);
        const float SLACK = 0.2f;
        for (int k = 0; k < 3; ++k) {
            if (linearUpper[k] - linearLower[k] < 1.0e-5f) {
                linearLower[k] -= SLACK;
                linearUpper[k] += SLACK;
            }
        }
        dof6->setLinearLowerLimit(linearLower);
        dof6->setLinearUpperLimit(linearUpper);

        dof6->setAngularLowerLimit(ToBtVec(src.rotateLowerLimit));
        dof6->setAngularUpperLimit(ToBtVec(src.rotateUpperLimit));

        // バネ設定
        auto setSpring = [&](int index, float stiff) {
            if (stiff > 0.0f) {
                dof6->enableSpring(index, true);
                dof6->setStiffness(index, stiff);
            }
            };
        setSpring(0, src.springTranslateFactor.x);
        setSpring(1, src.springTranslateFactor.y);
        setSpring(2, src.springTranslateFactor.z);
        setSpring(3, src.springRotateFactor.x);
        setSpring(4, src.springRotateFactor.y);
        setSpring(5, src.springRotateFactor.z);

        m_dynamicsWorld->addConstraint(dof6, true);
        m_constraints.push_back(dof6);
    }
}


// ★ここが描画のメイン！
void PMXPhysics::DebugDraw(RenderContext& rc)
{
    if (!m_dynamicsWorld) return;

    // フレームの開始処理
    m_debugWireFrame.Begin();

    // Bulletに「描画して！」と命令する
    // -> 内部で m_debugWireFrame.drawLine() が大量に呼ばれる
    m_dynamicsWorld->debugDrawWorld();

    // フレームの終了処理（ここでGPUに描画命令発行）
    m_debugWireFrame.End(rc);
}

void PMXPhysics::Update(float deltaT, std::vector<XMMATRIX>& bonesMatrices)
{
    if (!m_dynamicsWorld) return;
    SyncKinematicBody(bonesMatrices);
    m_dynamicsWorld->stepSimulation(deltaT, 5, 1.0f / 60.0f);
    SyncDynamicBone(bonesMatrices);
}

void PMXPhysics::SyncKinematicBody(const std::vector<XMMATRIX>& bonesMatrices)
{
    for (auto& info : m_rigidBodies)
    {
        if (info.type == 0) // Bone追従
        {
            if (info.boneIndex < 0 || info.boneIndex >= (int)bonesMatrices.size()) continue;

            XMMATRIX m = bonesMatrices[info.boneIndex];
            btTransform tr;
            XMFLOAT4X4 mat; XMStoreFloat4x4(&mat, m);
            tr.setOrigin(btVector3(mat._41, mat._42, mat._43));
            XMVECTOR scale, rotQ, translation;
            XMMatrixDecompose(&scale, &rotQ, &translation, m);
            XMFLOAT4 qf; 
            XMStoreFloat4(&qf, rotQ);
            tr.setRotation(btQuaternion(qf.x, qf.y, qf.z, qf.w));

            // オフセット適用して移動
            btTransform finalTr = tr * info.boneOffset;
            info.body->getMotionState()->setWorldTransform(finalTr);
            info.body->setWorldTransform(finalTr);
        }
    }
}

void PMXPhysics::SyncDynamicBone(std::vector<XMMATRIX>& bonesMatrices)
{
    for (auto& info : m_rigidBodies)
    {
        // 物理演算結果の反映
        if (info.type == 1 || info.type == 2)
        {
            if (info.boneIndex < 0 || info.boneIndex >= (int)bonesMatrices.size()) continue;

            btTransform tr;
            info.body->getMotionState()->getWorldTransform(tr);

            // オフセットを戻してボーン位置へ
            btTransform boneTr = tr * info.boneOffset.inverse();

            btVector3 p = boneTr.getOrigin();
            btQuaternion q = boneTr.getRotation();

            XMVECTOR pos = XMVectorSet(p.x(), p.y(), p.z(), 1.0f);
            XMVECTOR rot = XMVectorSet(q.x(), q.y(), q.z(), q.w());

            // 行列更新（位置も回転も適用）
            bonesMatrices[info.boneIndex] = XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);
        }
    }
}

void PMXPhysics::ResetRigidBodies(const std::vector<XMMATRIX>& bonesMatrices)
{
    for (auto& info : m_rigidBodies)
    {
        if (info.boneIndex < 0 || info.boneIndex >= (int)bonesMatrices.size()) continue;

        XMMATRIX m = bonesMatrices[info.boneIndex];
        btTransform tr;
        XMFLOAT4X4 mat; XMStoreFloat4x4(&mat, m);
        tr.setOrigin(btVector3(mat._41, mat._42, mat._43));
        XMVECTOR q = XMQuaternionRotationMatrix(m);
        XMFLOAT4 qf; XMStoreFloat4(&qf, q);
        tr.setRotation(btQuaternion(qf.x, qf.y, qf.z, qf.w));

        // 初期位置へ強制移動
        btTransform finalTr = tr * info.boneOffset;
        info.body->setWorldTransform(finalTr);
        info.body->getMotionState()->setWorldTransform(finalTr);

        // 速度ゼロ
        info.body->setLinearVelocity(btVector3(0, 0, 0));
        info.body->setAngularVelocity(btVector3(0, 0, 0));
        info.body->clearForces();
    }

    if (m_dynamicsWorld) {
        m_dynamicsWorld->clearForces();
        m_dynamicsWorld->getBroadphase()->resetPool(m_dispatcher.get());

        // マニフォールドを安全にクリア
        int numManifolds = m_dispatcher->getNumManifolds();
        for (int i = 0; i < numManifolds; i++) {
            btPersistentManifold* manifold =
                m_dispatcher->getManifoldByIndexInternal(i);
            if (manifold) manifold->clearManifold();
        }
    }
}

