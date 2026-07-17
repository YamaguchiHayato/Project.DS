#include "k2EngineLowPreCompile.h"
#include "FontRender.h"


void nsK2EngineLow::FontRender::Draw(RenderContext& rc)
{
	//テキストがない場合はリターン。
	if (m_text == nullptr) {
		return;
	}
	g_renderingEngine->AddRenderObject(this);

}
