#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <Windows.h>

namespace nsK2EngineLow {
    /// <summary>
    /// PMXVertex 構造体は、3D モデルの頂点データを表現します。
    /// </summary>
    struct PMXVertex {
        float pos[3];
        float normal[3];
        float uv[2];
        // TODO: 後でボーン・ウェイト情報など追加
            // --- 追加要素 ---
        std::vector<uint32_t> boneIndices;  // ボーンインデックス（1〜4個）
        std::vector<float> boneWeights;     // 各ボーンのウェイト（1〜4個）
        std::vector<std::array<float, 4>> addUVs; // もしAddUV使う場合は有効化
        uint8_t skinningType;
    };
    /// <summary>
    /// PMXMaterial 構造体は、PMX モデルのマテリアル情報を表します。
    /// </summary>
    struct PMXMaterial {
        std::string name;
        std::string nameEnglish;

        float diffuse[4];
        float specular[3];
        float shininess;
        float ambient[3];
        uint8_t flags;

        std::string textureFile;
        std::string sphereTextureFile;
        uint8_t sphereMode;
        std::string toonTextureFile;

        // 追加しておくと後で便利
        float edgeColor[4];
        float edgeSize;
        int32_t faceCount;
    };

    // ボーンフラグの定義
    enum PMXBoneFlags : uint16_t {
        Connection = 0x0001, // 接続先(0:座標オフセットで指定 1:ボーンで指定)
        Rotatable = 0x0002, // 回転可能
        Movable = 0x0004, // 移動可能
        Display = 0x0008, // 表示
        Operatable = 0x0010, // 操作可
        IK = 0x0020, // IK
        LocalGrant = 0x0080, // ローカル付与 | 付与対象 0:ユーザー変形値／IKリンク／他付与の後に付与 1:親のローカル変形量に付与
        RotGrant = 0x0100, // 回転付与
        MoveGrant = 0x0200, // 移動付与
        AxisFixed = 0x0400, // 軸固定
        LocalAxis = 0x0800, // ローカル軸
        PhysicsAfter = 0x1000, // 物理後変形
        ExternalParent = 0x2000, // 外部親変形
    };

    struct PMXBone {
        // --- 基本情報 ---
        std::string name;              // ボーン名（日本語）
        std::string enName;            // ボーン名（英語）
        Vector3 position;              // モデル原点からの座標
        int parentIndex;               // 親ボーンのインデックス（-1 の場合なし）
        int32_t deformLayer;           // 変形階層（0が最初に処理される）
        uint16_t boneFlag;             // ボーンフラグ（IKや可動などのビットフラグ）

        // --- 接続関連 ---
        bool hasParent = false;        // 親が存在するか（※ readIndexで-1ならfalse）
        int32_t connectIndex;          // 接続先ボーンのインデックス（または-1）
        Vector3 offset;                // 接続先が存在しない場合のオフセットベクトル

        // --- 付与（Grant）関連 ---
        int32_t grantParentIndex;      // 回転／移動付与 親ボーン
        float grantWeight;             // 付与率（0.0～1.0）
        bool isLocalGrant = false;     // ローカル付与かどうか
        bool isTranslationGrant = false;   // ←★これを追加！（移動付与かどうか）

        // --- 軸・外部親関連 ---
        Vector3 axisDirection;         // 軸制限の方向ベクトル
        Vector3 localX;                // ローカル座標系X軸
        Vector3 localZ;                // ローカル座標系Z軸
        int32_t externalKey;           // 外部親キー（エクスタボーン）

        // --- IK関連 ---
        bool hasIK;                    // IKボーンかどうか
        int32_t ikTargetIndex;         // IKターゲットボーン
        int32_t ikLoopCount;           // IK再帰回数
        float ikLimitAngle;            // 1回あたりの最大角度（ラジアン）

        struct IKLink {
            int32_t linkBoneIndex;     // IKリンクボーン
            bool hasLimit;             // 制限角があるか
            Vector3 limitMin;          // 制限角（最小）
            Vector3 limitMax;          // 制限角（最大）
        };
        std::vector<IKLink> ikLinks;   // IKリンク配列

        // ★ 現在のローカル補正（VMDから来る相対）
        DirectX::XMFLOAT3 currentPosition = { 0,0,0 };
        DirectX::XMFLOAT4 currentRotation = { 0,0,0,1 };

        // ★ 逆バインド（Tポーズのグローバル行列の逆行列）
        DirectX::XMFLOAT4X4 inverseBindPoseMatrix = {};

        // 任意：デバッグ用
        DirectX::XMFLOAT4X4 bindPoseGlobal = {};

        DirectX::XMFLOAT3 localOffset = { 0,0,0 };  // 親からの相対（PostProcessで計算）
    };

    enum class PMXMorphType : uint8_t
    {
        Group,
        Position,
        Bone,
        UV,
        AddUV1,
        AddUV2,
        AddUV3,
        AddUV4,
        Material,
        Flip,
        Impluse,
    };
    /// <summary>
    /// PMXMorph 構造体は、3D モデルのモーフィング操作を表現するためのデータ構造です。
    /// </summary>
    struct PMXMorph
    {
        std::wstring name;
        std::string englishName;

        unsigned char controlPanel;
        PMXMorphType morphType;

        struct PositionMorph
        {
            unsigned int vertexIndex;
            DirectX::XMFLOAT3 position;
        };

        struct UVMorph
        {
            unsigned int vertexIndex;
            DirectX::XMFLOAT4 uv;
        };
        /// <summary>
        /// 骨の変形を表す構造体。
        /// </summary>
        struct BoneMorph
        {
            unsigned int boneIndex;
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT4 quaternion;
        };
        /// <summary>
        /// MaterialMorph 構造体は、マテリアルのモーフィング操作を表します。
        /// </summary>
        struct MaterialMorph
        {
            enum class OpType : uint8_t
            {
                Mul,
                Add,
            };

            unsigned int	materialIndex;
            OpType	opType;
            DirectX::XMFLOAT4 diffuse;
            DirectX::XMFLOAT3 specular;
            float specularPower;
            DirectX::XMFLOAT3 ambient;
            DirectX::XMFLOAT4 edgeColor;
            float edgeSize;
            DirectX::XMFLOAT4 textureFactor;
            DirectX::XMFLOAT4 sphereTextureFactor;
            DirectX::XMFLOAT4 toonTextureFactor;
        };

        struct GroupMorph
        {
            unsigned int morphIndex;
            float weight;
        };

        struct FlipMorph
        {
            unsigned int morphIndex;
            float weight;
        };

        struct ImpulseMorph
        {
            unsigned int rigidBodyIndex;
            unsigned char localFlag;	//0:OFF 1:ON
            DirectX::XMFLOAT3 translateVelocity;
            DirectX::XMFLOAT3 rotateTorque;
        };

        std::vector<PositionMorph> positionMorph;
        std::vector<UVMorph> uvMorph;
        std::vector<BoneMorph> boneMorph;
        std::vector<MaterialMorph> materialMorph;
        std::vector<GroupMorph> groupMorph;
        std::vector<FlipMorph> flipMorph;
        std::vector<ImpulseMorph> impulseMorph;
    };
    /// <summary>
    /// PMXDisplayFrame 構造体は、PMX モデルの表示フレームを表します。
    /// </summary>
    struct PMXDisplayFrame
    {
        std::wstring name;
        std::string englishName;

        enum class TargetType : uint8_t
        {
            BoneIndex,
            MorphIndex,
        };
        struct Target
        {
            TargetType type;
            unsigned int index;
        };

        enum class FrameType : uint8_t
        {
            DefaultFrame,
            SpecialFrame
        };

        FrameType flag;
        std::vector<Target> targets;
    };
    /// <summary>
    /// PMXRigidBody 構造体は、物理剛体のプロパティを定義します。
    /// </summary>
    struct PMXRigidBody
    {
        std::wstring name;
        std::string englishName;

        unsigned int boneIndex;
        unsigned char group;
        unsigned short collisionGroup;

        enum class Shape : uint8_t
        {
            Sphere,
            Box,
            Capsule
        };

        Shape shape;
        DirectX::XMFLOAT3 shapeSize;
        DirectX::XMFLOAT3 translate;
        DirectX::XMFLOAT3 rotate;

        float mass;
        float translateDimmer;
        float rotateDimmer;
        float repulsion;
        float friction;

        enum class Operation : uint8_t
        {
            Static,
            Dynamic,
            DynamicAndBoneMerge,
        };
        Operation op;
    };
    /// <summary>
    /// PMXJoint 構造体は、PMX モデルのジョイント（関節）を表します。
    /// </summary>
    struct PMXJoint
    {
        std::wstring name;
        std::string englishName;

        enum class JointType : uint8_t
        {
            SpringDOF6,
            DOF6,
            P2P,
            ConeTwist,
            Slider,
            Hinge,
        };
        JointType type;
        unsigned int rigidBodyAIndex;
        unsigned int rigidBodyBIndex;

        DirectX::XMFLOAT3 translate;
        DirectX::XMFLOAT3 rotate;

        DirectX::XMFLOAT3 translateLowerLimit;
        DirectX::XMFLOAT3 translateUpperLimit;
        DirectX::XMFLOAT3 rotateLowerLimit;
        DirectX::XMFLOAT3 rotateUpperLimit;

        DirectX::XMFLOAT3 springTranslateFactor;
        DirectX::XMFLOAT3 springRotateFactor;
    };


    /// <summary>
/// PMXファイルのヘッダー情報を表す構造体。
/// </summary>
    struct PMXHeader {
        char magic[4];       // "PMX " のマジックナンバー
        float version;       // PMXバージョン（例 2.0）
        uint8_t dataSize;    // ヘッダ情報のサイズ
        uint8_t global[8];   // インデックスサイズなど各種設定(各要素数が何を示すかはヘッダーファイルにコメントにて記載。F12で飛んでね)
        // global[0] : エンコーディング (0=UTF16, 1=UTF8)
        // global[1] : 追加UVの数
        // global[2] : 頂点インデックスサイズ
        // global[3] : テクスチャインデックスサイズ
        // global[4] : マテリアルインデックスサイズ
        // global[5] : ボーンインデックスサイズ
        // global[6] : モーフインデックスサイズ
        // global[7] : 剛体インデックスサイズ
    };
    /// <summary>
    /// PMXモデルを表す構造体。
    /// </summary>
    struct PMXModel {
        std::string modelName;
		std::string modelNameEnglish;
        std::string comment;
		std::string commentEnglish;
        std::vector<PMXVertex> vertices;
        std::vector<int32_t> indices;
        std::vector<std::string> textures;
        std::vector<PMXMaterial> materials;
        std::vector<PMXBone> bones;
		std::vector<PMXMorph> morphs;
        std::vector<PMXDisplayFrame> displayFrames;
		std::vector<PMXRigidBody> rigidBodies;
		std::vector<PMXJoint> joints;

		PMXHeader header;   

        // ★ VMD 適用後にレンダ側が検出できるように
        bool needBoneRecalc = true;
        // ★ VMD が「骨を更新した」というフラグ（UpdateBoneMatrices の前で使う）
        void RequestBoneRecalc() { needBoneRecalc = true; }

        // ★ ボーン名→インデックス（VMD適用の名前解決を高速化）
        std::unordered_map<std::string, int> boneIndexByName;

        // ★ モーフ名 → 重み（0.0～1.0）
        std::unordered_map<std::wstring, float> activeMorphWeights;

        // ★追加：変形階層順にソートされたボーンインデックス
        std::vector<int> sortedBoneIndices;
    };

    /// <summary>
    /// PMXモデルを読み込むためのローダーのヘッダーファイル
    /// </summary>
    class PMXLoader {
    public:
        /// <summary>
        /// 指定されたファイルパスからPMXモデルを読み込みます。
        /// </summary>
        /// <param name="path">読み込むファイルのパス。</param>
        /// <param name="outModel">読み込んだPMXモデルを格納する出力パラメータ。</param>
        /// <returns>ファイルの読み込みに成功した場合はtrue、失敗した場合はfalseを返します。</returns>
        bool LoadFromFile(const std::string& path, PMXModel& outModel);

    private:
        bool LoadPMXHeader(std::ifstream& ifs, PMXHeader& header);
        /// <summary>
        /// 指定されたエンコーディングでファイルストリームから文字列を読み取ります。
        /// </summary>
        /// <param name="ifs">入力ファイルストリーム（std::ifstream 型）。読み取るデータのソース。</param>
        /// <param name="encoding">文字列を読み取る際に使用するエンコーディングを指定する整数値。</param>
        /// <returns>読み取られた文字列（std::string 型）。</returns>
        std::string readString(std::ifstream& ifs, int encoding);

        std::wstring readStringW(std::ifstream& ifs, int encoding);
        /// <summary>
        /// 指定されたインデックスサイズでファイルストリームからインデックスを読み取ります。
        /// </summary>
        /// <param name="ifs">インデックスを読み取るための入力ファイルストリーム。</param>
        /// <param name="indexSize">読み取るインデックスのサイズ（バイト単位）。</param>
        /// <returns>読み取られたインデックスの値（int32_t 型）。</returns>
        int32_t readIndex(std::ifstream& ifs, int indexSize);
        /// <summary>
        /// UTF-8エンコードされた文字列をワイド文字列に変換します。
        /// </summary>
        /// <param name="utf8">変換するUTF-8エンコードされた文字列。</param>
        /// <returns>変換されたワイド文字列。</returns>
        /// 
        std::wstring Utf8ToWstring(const std::string& utf8);
        /// <summary>
        /// ワイド文字列をUTF-8文字列に変換します。
        /// </summary>
        /// <param name="wstr">変換するワイド文字列。</param>
        /// <returns>UTF-8形式の文字列。</returns>
        std::string  WstringToUtf8(const std::wstring& wstr);
		
        bool LoadBones(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXBone>& bones);
	    bool LoadMorphs(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXMorph>& morphs);
        bool LoadDisplayFrames(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXDisplayFrame>& frames);
        bool LoadRigidBodies(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXRigidBody>& rigidBodies);
        void LoadJoints(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXJoint>& joints);


        // ★ 追加：読み込み後の後処理（必須）
        void PostProcess(PMXModel& model);

        // ★ bind pose と inverseBindPose を作る
        void BuildBindPoseAndInverse(PMXModel& model);

        // ★ ボーン名→インデックス辞書を作る
        void BuildBoneNameMap(PMXModel& model);

        // ★ 頂点ウェイトを 4本・合計1.0 に束ねる
        void NormalizeAndClampVertexWeights(PMXModel& model);

        // （必要なら）右手↔左手の座標系変換
        void ConvertCoordinateIfNeeded(PMXModel& model, bool mmdRightHandToDxLeftHand);
    };

}
