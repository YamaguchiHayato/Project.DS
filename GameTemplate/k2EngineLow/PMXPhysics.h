#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include "btBulletDynamicsCommon.h"

// 前方宣言
namespace nsK2EngineLow {
    struct PMXModel;

    class RenderContext; // ★前方宣言を追加
    class PMXPhysics
    {
    public:
        PMXPhysics();
        ~PMXPhysics();

        // 初期化（剛体とジョイントを作る）
        void Init(const nsK2EngineLow::PMXModel& model);

        // ★追加：物理剛体をボーン位置に強制リセットする
        void ResetRigidBodies(const std::vector<DirectX::XMMATRIX>& bonesMatrices);

        // 更新（物理シミュレーションを進めて、結果をボーンに反映する）
        // deltaT: 経過時間（秒）
        // ★修正: NodeManager ではなく、計算中の行列配列(globals)を受け取るように変更
        void Update(float deltaT, std::vector<DirectX::XMMATRIX>& bonesMatrices);

        // ★修正: RenderContextを受け取って描画する
        void DebugDraw(RenderContext& rc);

        // ★追加：ワールド取得（デバッグ描画などで使う）
        btDiscreteDynamicsWorld* GetDynamicsWorld() const { return m_dynamicsWorld.get(); }

        /// <summary>
        /// 物理剛体を1つでも持っているか。
        /// 剛体ゼロのモデル（地面など）は物理更新自体をスキップして軽量化できる。
        /// </summary>
        bool HasRigidBodies() const { return !m_rigidBodies.empty(); }

    private:
        // 物理ワールドのクリーンアップ
        void Cleanup();

        // Bullet Physics の基本オブジェクト群
        std::unique_ptr<btBroadphaseInterface> m_broadphase;
        std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
        std::unique_ptr<btCollisionDispatcher> m_dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
        std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

        // ★変更: エンジンのデバッグ描画クラスを直接持つ
        DebugWireframe m_debugWireFrame;

        // 管理用構造体
        struct RigidBodyInfo {
            btRigidBody* body = nullptr;
            int boneIndex = -1;
            int type = 0; // 0:Bone追従, 1:物理演算, 2:物理+位置合わせ
            int group = 0;
            int groupMask = 0;

            // ★追加: ボーンから剛体への相対変換行列 (Offset)
            btTransform boneOffset;
        };

        // 生成した剛体とジョイントのリスト
        std::vector<RigidBodyInfo> m_rigidBodies;
        std::vector<btTypedConstraint*> m_constraints;

        // 足元(y=0)の静的な床。長い髪やスカートが床を貫通しないように止める
        btRigidBody* m_groundBody = nullptr;

        // ボーン追従剛体（Kinematic）をアニメーション位置へ動かす
        void SyncKinematicBody(const std::vector<DirectX::XMMATRIX>& bonesMatrices);

        // 物理演算剛体（Dynamic）の結果をボーンに書き戻す
        void SyncDynamicBone(std::vector<DirectX::XMMATRIX>& bonesMatrices);
    };
}