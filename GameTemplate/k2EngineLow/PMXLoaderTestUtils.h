#pragma once
#include <string>

// PMXModel を使う（ボーン行列を参照するため）
#include "PMXLoader.h"

// bindPose テスト実行関数
void TestPMXBindPose(const std::string& pmxPath);
