#pragma once
namespace nsK2EngineLow {
    class UIGaugeArc : public IRender{
public:
        struct HPGaugeCB {
            float hpRate;
            float startAngle;
            float maxAngle;
            float innerRadius;
            float outerRadius;
            float damageFlash;
            float padding[2];
        };
		//ダメやったら構造体追加でやろうかな？


        //初期化。
        void Init(const char* filePath, float w, float h, const char* fxPath = "Assets/shader/HPGauge.fx");
		//更新。
        void Update(float hpRate, float damageFlash, float startAngle, float maxAngle, float innerRadius, float outerRadius);
		//描画。
        void Draw(RenderContext& rc);
        // 座標設定
        void SetPosition(const Vector3& pos) {
            m_position = pos;
        }

        void SetPivot(const Vector2& pivot) {
            m_pivot = pivot;
		}


    private:
        // IRenderの仮想関数実装（レンダリングエンジンから呼ばれる）
        void OnRender2D(RenderContext& rc) override {
            m_sprite.Draw(rc);
        }

    private:
        Sprite   m_sprite;
		HPGaugeCB m_hpGaugeCBData;

        Vector3 m_position = Vector3::Zero;
        Quaternion m_rotation = Quaternion::Identity;
        Vector3 m_scale = Vector3::One;
        Vector2 m_pivot = Vector2(0.5f, 1.0f); // 下中央ピボット
    };
}