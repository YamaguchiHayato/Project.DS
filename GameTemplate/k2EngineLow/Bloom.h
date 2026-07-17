#pragma once
namespace nsK2EngineLow {
	class Bloom:public IGameObject
	{
	public:
		Bloom();//コンストラクタ。
		~Bloom();//デストラクタ。
		//初期化
		void Init(RenderTarget& renderTarget);
		void Update();//更新処理。
		//描画。
		void Render(RenderContext& rc, RenderTarget&renderTarget);

		RenderTarget luminanceRenderTarget;
	private:
		SpriteInitData luminanceSpriteInitData;
		SpriteInitData finalSpriteInitData;
		Sprite m_luminanceSprite;
		Sprite m_finalSprite;
		GaussianBlur gaussianBulur[4];
	
	};
}
