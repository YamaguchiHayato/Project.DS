#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <DirectXMath.h>

namespace nsK2EngineLow {

    /// 1キー分のデータ
    struct VMDKeyFrame
    {
        float frame;                 // フレーム番号（30fps）
        DirectX::XMFLOAT3 pos;       // 位置
        DirectX::XMFLOAT4 rot;       // 回転（クォータニオン）

        // 補間テーブル
        // interp[channel][0..3]
        //   channel: 0=X, 1=Y, 2=Z, 3=Rotation
        //   [0] = p1.x, [1] = p1.y, [2] = p2.x, [3] = p2.y
        uint8_t interp[4][4];
    };

    /// ボーンごとのアニメ
    struct VMDBoneAnim
    {
        std::string boneName;            // 正規化後のボーン名（UTF-8）
        std::vector<VMDKeyFrame> keyframes;
    };

    // ★追加: モーフ(表情)用のキーフレーム
    struct VMDMorphKeyFrame
    {
        float frame;    // フレーム番号
        float weight;   // ウェイト(0.0〜1.0)
    };

    // ★追加: モーフごとのアニメデータ
    struct VMDMorphAnim
    {
        std::string morphName; // モーフ名
        std::vector<VMDMorphKeyFrame> keyframes;
    };

    // ★追加: カメラキーフレーム構造体
    struct VMDCameraKeyFrame
    {
        float frame;            // フレーム番号
        float distance;         // 目標点からの距離(負の値で近づく)
        DirectX::XMFLOAT3 pos;  // 目標点(注視点)の位置 (x, y, z)
        DirectX::XMFLOAT3 rot;  // 回転 (x=pitch, y=yaw, z=roll) ※ラジアン
        int fov;                // 視野角
        uint8_t interp[6][4];   // 補間曲線 (X, Y, Z, Rot, Dist, FOV)
    };

    // ★追加: カメラアニメーションデータ全体
    struct VMDCameraAnim
    {
        std::vector<VMDCameraKeyFrame> keyframes;
    };

    // VMDLoader.h の VMDCameraAnim の下あたりに追加

    struct VMDIKState {
        std::string boneName;
        bool enable;
    };

    struct VMDDisplayIKKeyFrame {
        float frame;
        bool show; // モデルの表示設定（今回は使わなくてもOK）
        std::vector<VMDIKState> ikStates;
    };


    /// VMD モーションローダ
    class VMDLoader
    {
    public:
        /// VMDファイル読み込み
        bool Load(const std::string& path);

        /// ボーン名 → アニメーション
        const std::unordered_map<std::string, VMDBoneAnim>& GetBoneAnims() const
        {
            return m_boneAnims;
        }

        // ★追加: モーフアニメの取得
        const std::unordered_map<std::string, VMDMorphAnim>& GetMorphAnims() const {
            return m_morphAnims;
        }

        // ★追加: カメラデータの取得
        const VMDCameraAnim& GetCameraAnim() const { return m_cameraAnim; }

        // ★★★ 追加：別のVMDデータを自分の中に統合する ★★★
        void Merge(const VMDLoader& other);

        /// 全アニメデータを空にする（VMD未設定の曲に切り替えたとき、古いダンスを消す用）
        void Clear() {
            m_boneAnims.clear();
            m_morphAnims.clear();
            m_cameraAnim.keyframes.clear();
            m_displayIKAnim.clear();
        }

        const std::vector<VMDDisplayIKKeyFrame>& GetDisplayIKAnim() const { return m_displayIKAnim; }


    private:
        // ボーン名の正規化（センター／center／ｾﾝﾀｰ 等を揃える）
        static std::string CanonicalizeBoneName(const std::string& utf8Name);

        std::unordered_map<std::string, VMDBoneAnim> m_boneAnims;

        // ★追加
        std::unordered_map<std::string, VMDMorphAnim> m_morphAnims;

        // ★追加: カメラデータの実体
        VMDCameraAnim m_cameraAnim;

        // VMDLoader クラスのメンバ変数に追加
        std::vector<VMDDisplayIKKeyFrame> m_displayIKAnim;
    };

} // namespace nsK2EngineLow
