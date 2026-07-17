#pragma once


namespace nsK2EngineLow {

    // SpriteRenderをベースにした、ひし形ワイプ専用のレンダークラス
    class DiamondWipe : public IRender
    {
    public:
        // 初期化（テクスチャパスなどを指定）
        void Init(const char* filePath, float w, float h);

        // 描画（RenderContextを受け取る）
        void Draw(RenderContext& rc);

        // --- 制御用関数 ---

        // 進行度を設定 (0.0:開始 ～ 1.0:終了)
        void SetProgress(float progress) {
            m_cbData.progress = progress;
        }

        // ひし形の細かさを設定
        void SetTiling(float tiling) {
            m_cbData.tiling = tiling;
        }

        // 座標設定
        void SetPosition(const Vector3& pos) {
            m_position = pos;
        }

    private:
        // IRenderの仮想関数実装（レンダリングエンジンから呼ばれる）
        void OnRender2D(RenderContext& rc) override {
            m_sprite.Draw(rc);
        }

        // シェーダーに送るデータ構造体 (b1レジスタ用)
        struct DiamondConstants {
            float progress = 0.0f;
            float tiling = 16.0f;
            float padding[2];
        };

        Sprite m_sprite;           // 実体のスプライト
        DiamondConstants m_cbData; // 拡張定数バッファ用データ

        Vector3 m_position = Vector3::Zero;
        Quaternion m_rotation = Quaternion::Identity;
        Vector3 m_scale = Vector3::One;
        Vector2 m_pivot = Sprite::DEFAULT_PIVOT;
    };
}