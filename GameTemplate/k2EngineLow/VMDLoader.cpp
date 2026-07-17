#include "k2EngineLowPreCompile.h"
#include "VMDLoader.h"
#include <fstream>
#include <Windows.h>
#include <algorithm>

using namespace nsK2EngineLow;
using namespace DirectX;
using namespace std;

// ★ Windows API 版に差し替え
static std::wstring Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wide(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wlen);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

static std::string SJISToUTF8(const char* sjisStr)
{
    if (!sjisStr) return "";
    int wlen = MultiByteToWideChar(932, 0, sjisStr, -1, nullptr, 0);
    if (wlen <= 0) return "";

    std::wstring wide(wlen, 0);
    MultiByteToWideChar(932, 0, sjisStr, -1, &wide[0], wlen);

    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";

    std::string utf8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    // 末尾の '\0' はあってもなくても良いので、消しておく
    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
    return utf8;
}

//------------------------------------------------------------
// ボーン名正規化
//------------------------------------------------------------
std::string VMDLoader::CanonicalizeBoneName(const std::string& utf8Name)
{
    // 基本はそのまま
    std::string name = utf8Name;

    // 前後の空白カット
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) name.erase(name.begin());
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) name.pop_back();

    // ざっくり全部小文字化（ASCII範囲だけ）
    std::string lower = name;
    for (auto& c : lower) {
        if ('A' <= c && c <= 'Z') c = (char)(c - 'A' + 'a');
    }

    // --- センター系 ---
    // "センター", "ｾﾝﾀｰ", "center" などは全部 "センター" に統一
    if (lower == "center" || lower == "ｾﾝﾀｰ" || name == u8"センター") {
        return u8"センター";
    }

    // --- 全ての親 / 全親 / root 系 ---
    if (name == u8"全ての親" || name == u8"全親" ||
        lower == "root" || lower == "parent" || lower == "allparent")
    {
        return u8"全ての親";
    }

    // ★★★ ここから追加：全角「ＩＫ」を半角「IK」に統一する処理 ★★★
    // UTF-8において、全角の「Ｉ」は \xEF\xBC\xA9、全角の「Ｋ」は \xEF\xBC\xAB です。
    // 簡単のため、文字列内に "ＩＫ" があれば "IK" に置換します。
    
    std::string zenkakuIK = u8"ＩＫ";
    std::string hankakuIK = "IK";
    size_t pos = 0;
    while ((pos = name.find(zenkakuIK, pos)) != std::string::npos) {
        name.replace(pos, zenkakuIK.length(), hankakuIK);
        pos += hankakuIK.length();
    }
    // 小文字の半角「ik」が含まれている場合も大文字「IK」に統一しておくと安全です
    pos = 0;
    while ((pos = name.find("ik", pos)) != std::string::npos) {
        name.replace(pos, 2, hankakuIK);
        pos += hankakuIK.length();
    }
    // ★★★ 追加ここまで ★★★

    // それ以外はそのまま返す
    return name;
}

//------------------------------------------------------------
// VMD読み込み本体
//------------------------------------------------------------
bool VMDLoader::Load(const std::string& path)
{
    // 再Loadで前回のデータが残らないよう、全アニメをクリアして冪等にする。
    // （別の曲に切り替えたとき、古いモーフ・カメラ・IKが残るのを防ぐ）
    m_boneAnims.clear();
    m_morphAnims.clear();
    m_cameraAnim.keyframes.clear();
    m_displayIKAnim.clear();

    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        OutputDebugStringW(L"[VMDLoader] ファイルを開けませんでした。\n");
        return false;
    }

    // --- Header ---
    char header[30] = {};
    ifs.read(header, 30);
    if (strncmp(header, "Vocaloid Motion Data 0002", 25) != 0) {
        OutputDebugStringW(L"[VMDLoader] 不正なVMDヘッダです。\n");
        return false;
    }

    // --- モデル名（Shift-JIS 20byte 固定）---
    char modelName[20] = {};
    ifs.read(modelName, 20);
    std::string modelNameUTF8 = SJISToUTF8(modelName);
    std::wstring wModelName = Utf8ToWstring(modelNameUTF8);

    //OutputDebugStringW(L"[VMDLoader] モデル名: ");
    //OutputDebugStringW(wModelName.c_str());
    //OutputDebugStringW(L"\n");
	DebugPrintW(L"[VMDLoader] モデル名: %s\n", wModelName.c_str());

    // --- ボーンキーフレーム数 ---
    uint32_t boneCount = 0;
    ifs.read(reinterpret_cast<char*>(&boneCount), sizeof(uint32_t));
    {
      /*  wchar_t msg[128];
        swprintf_s(msg, L"[VMDLoader] ボーンキーフレーム数(raw): %u\n", boneCount);
        OutputDebugStringW(msg);*/
		DebugPrintW(L"[VMDLoader] ボーンキーフレーム数(raw): %u\n", boneCount);
    }

#pragma pack(push, 1)
    struct RawBoneFrame {
        char    boneName[15];   // Shift-JIS (終端なしの可能性あり)
        uint32_t frame;         // フレーム番号
        float   pos[3];         // 位置
        float   rot[4];         // 回転 (x,y,z,w)
        uint8_t interp[64];     // 補間データ
    };
#pragma pack(pop)

    // --- 各キーフレームを読み込み ---
    for (uint32_t i = 0; i < boneCount; ++i)
    {
        RawBoneFrame bf{};
        ifs.read(reinterpret_cast<char*>(&bf), sizeof(bf));
        if (!ifs) {
            OutputDebugStringW(L"[VMDLoader] 途中でEOF/エラー。boneCountが多すぎるかも。\n");
            break;
        }
        // ---- 正しい VMD ボーン名読み取り（文字化けしない） ----

        // 元の15バイトをそのままコピー
        char sjisName[16];
        memcpy(sjisName, bf.boneName, 15);
        sjisName[15] = '\0';

        // Shift-JIS の末尾の無効バイトを除去（0x00パディング対策）
        int realLen = 15;
        while (realLen > 0 && sjisName[realLen - 1] == '\0')
            realLen--;

        // ★文字化け修正：可変長SJISとして扱う
        std::string sjisStr(sjisName, realLen);
        std::string boneNameUTF8 = SJISToUTF8(sjisStr.c_str());

        // 正規化
        std::string keyName = CanonicalizeBoneName(boneNameUTF8);
        std::wstring wBoneName = Utf8ToWstring(keyName);





        // マップからアニメを取得（なければ作成）
        auto& anim = m_boneAnims[keyName];
        anim.boneName = keyName;

        // キーフレーム作成
        VMDKeyFrame kf{};
        kf.frame = static_cast<float>(bf.frame);

        // ★ 修正版：軸反転いっさい無し
        kf.pos = {
            bf.pos[0],
            bf.pos[1],
            bf.pos[2]
        };

        kf.rot = {
            bf.rot[0],
            bf.rot[1],
            bf.rot[2],
            bf.rot[3]
        };

        // ★★★ ここに追加すると分かりやすい ★★★
        {
            // 「上半身／上半身2／首／頭」だけに絞ってもOK
            if (keyName == u8"上半身" ||
                keyName == u8"上半身2" ||
                keyName == u8"首" ||
                keyName == u8"頭")
            {
               /* wchar_t dbg[256];
                swprintf_s(
                    dbg,
                    L"[VMDLoader-KEY] bone=%ls frame=%u pos=(%.3f, %.3f, %.3f) rot=(%.3f, %.3f, %.3f, %.3f)\n",
                    wBoneName.c_str(),
                    (uint32_t)kf.frame,
                    kf.pos.x, kf.pos.y, kf.pos.z,
                    kf.rot.x, kf.rot.y, kf.rot.z, kf.rot.w
                );
                OutputDebugStringW(dbg);*/ //デバック出力を用意した関数に変更。

                DebugPrintW(L"[VMDLoader-KEY] bone=%ls frame=%u pos=(%.3f, %.3f, %.3f) rot=(%.3f, %.3f, %.3f, %.3f)\n",
                    wBoneName.c_str(),
                    (uint32_t)kf.frame,
                    kf.pos.x, kf.pos.y, kf.pos.z,
                    kf.rot.x, kf.rot.y, kf.rot.z, kf.rot.w);
            }
        }
        // ★★★ ここまで ★★★

        // ▼▼▼ デバッグ（最初の10キーだけ） ▼▼▼
        if (anim.keyframes.size() < 10) {
            //wchar_t dbg[256];
            //swprintf_s(
            //    dbg,
            //    L"[VMDLoader] bone=%ls frame=%u RAW pos=(%.3f,%.3f,%.3f) rot=(%.3f,%.3f,%.3f,%.3f)\n",
            //    wBoneName.c_str(),        // ← ここを使う
            //    (uint32_t)kf.frame,
            //    kf.pos.x, kf.pos.y, kf.pos.z,
            //    kf.rot.x, kf.rot.y, kf.rot.z, kf.rot.w
            //);
            //OutputDebugStringW(dbg);      // ← A じゃなくて W

            //デバック出力を用意した関数に変更。
            //DebugPrintW(L"[VMDLoader] bone=%ls frame=%u RAW pos=(%.3f,%.3f,%.3f) rot=(%.3f,%.3f,%.3f,%.3f)\n",
            //    wBoneName.c_str(),        // ← ここを使う
            //    (uint32_t)kf.frame,
            //    kf.pos.x, kf.pos.y, kf.pos.z,
            //    kf.rot.x, kf.rot.y, kf.rot.z, kf.rot.w);
        }
        // ▲▲▲ デバッグここまで ▲▲▲



            // --- 補間テーブル読み込み（VMD仕様通り） ---
            auto readInterp = [&](int idx, int p1x, int p1y, int p2x, int p2y) {
                kf.interp[idx][0] = bf.interp[p1x];
                kf.interp[idx][1] = bf.interp[p1y];
                kf.interp[idx][2] = bf.interp[p2x];
                kf.interp[idx][3] = bf.interp[p2y];
                };

            // X位置
            readInterp(0, 0, 4, 8, 12);
            // Y位置
            readInterp(1, 16, 20, 24, 28);
            // Z位置
            readInterp(2, 32, 36, 40, 44);
            // 回転
            readInterp(3, 48, 52, 56, 60);
       

        anim.keyframes.push_back(kf);
    }

    // --- ボーンごとにフレームをソート＆重複整理 ---
    for (auto& kv : m_boneAnims)
    {
        auto& anim = kv.second;
        auto& kfs = anim.keyframes;

        std::sort(kfs.begin(), kfs.end(),
            [](const VMDKeyFrame& a, const VMDKeyFrame& b) {
                return a.frame < b.frame;
            });

        // 同一 frame を持つキーが複数ある場合は「最後のものを採用」する
        std::vector<VMDKeyFrame> unique;
        unique.reserve(kfs.size());
        for (size_t i = 0; i < kfs.size(); ++i) {
            if (!unique.empty() && unique.back().frame == kfs[i].frame) {
                unique.back() = kfs[i];           // 上書き
            }
            else {
                unique.push_back(kfs[i]);
            }
        }
        kfs.swap(unique);

        // ログ
       /* wchar_t msg[256];
        swprintf_s(msg, L"[VMDLoader] Bone \"%ls\" : %zu keys\n",
            Utf8ToWstring(anim.boneName).c_str(), anim.keyframes.size());
        OutputDebugStringW(msg);*/

        //デバック出力を用意した関数に変更。
        DebugPrintW(L"[VMDLoader] Bone \"%ls\" : %zu keys\n",
            Utf8ToWstring(anim.boneName).c_str(), anim.keyframes.size());
    }

   /* wchar_t finalMsg[128];
    swprintf_s(finalMsg, L"[VMDLoader] 読み込み完了: ボーン数 = %zu\n", m_boneAnims.size());
    OutputDebugStringW(finalMsg);*/

	DebugPrintW(L"[VMDLoader] 読み込み完了: ボーン数 = %zu\n", m_boneAnims.size());


    // ★★★ ここから追加：表情(Morph)データの読み込み ★★★

    // VMD仕様: ボーンデータの次に「表情キーフレーム数(uint32_t)」がある
    uint32_t morphCount = 0;
    ifs.read(reinterpret_cast<char*>(&morphCount), sizeof(uint32_t));

    if (ifs.good())
    {
        {
			DebugPrintW(L"[VMDLoader] 表情キーフレーム数: %u\n", morphCount);
        }

#pragma pack(push, 1)
        struct RawMorphFrame {
            char    morphName[15];  // 表情名
            uint32_t frame;         // フレーム
            float   weight;         // ウェイト
        };
#pragma pack(pop)

        for (uint32_t i = 0; i < morphCount; ++i)
        {
            RawMorphFrame rf{};
            ifs.read(reinterpret_cast<char*>(&rf), sizeof(rf));
            if (!ifs) break;

            // 名前処理（文字化け対策含む）
            char sjisName[16];
            memcpy(sjisName, rf.morphName, 15);
            sjisName[15] = '\0';

            // 末尾ゴミ除去
            int realLen = 15;
            while (realLen > 0 && sjisName[realLen - 1] == '\0') realLen--;
            std::string sjisStr(sjisName, realLen);
            std::string nameUTF8 = SJISToUTF8(sjisStr.c_str());

            // モーフ名は正規化（Canonicalize）してもいいが、
            // 表情は表記揺れが少ないのでそのままでも動くことが多い
            // 一応ボーンと同じ関数を通すか、そのままマップへ

            VMDMorphKeyFrame mkf{};
            mkf.frame = (float)rf.frame;
            mkf.weight = rf.weight;

            m_morphAnims[nameUTF8].morphName = nameUTF8;
            m_morphAnims[nameUTF8].keyframes.push_back(mkf);
        }

        // ソートと重複整理
        for (auto& kv : m_morphAnims)
        {
            auto& kfs = kv.second.keyframes;
            std::sort(kfs.begin(), kfs.end(), [](const auto& a, const auto& b) {
                return a.frame < b.frame;
                });

            // 重複除去（同じフレームなら後勝ち）
            std::vector<VMDMorphKeyFrame> unique;
            unique.reserve(kfs.size());
            for (auto& k : kfs) {
                if (!unique.empty() && unique.back().frame == k.frame) {
                    unique.back() = k;
                }
                else {
                    unique.push_back(k);
                }
            }
            kfs = std::move(unique);
        }

		DebugPrintW(L"[VMDLoader] 表情アニメ数: %zu\n", m_morphAnims.size());
    }
    // ★★★ 追加ここまで ★★★


    // ★★★ ここから追加：カメラデータの読み込み ★★★
    uint32_t cameraCount = 0;
    ifs.read(reinterpret_cast<char*>(&cameraCount), sizeof(uint32_t));

    if (ifs.good() && cameraCount > 0 && cameraCount < 100000)
    {
    /*    wchar_t msg[128];
        swprintf_s(msg, L"[VMDLoader] カメラキーフレーム数: %u\n", cameraCount);
        OutputDebugStringW(msg);*/

		DebugPrintW(L"[VMDLoader] カメラキーフレーム数: %u\n", cameraCount);

        m_cameraAnim.keyframes.reserve(cameraCount);

#pragma pack(push, 1)
        struct RawCameraFrame {
            uint32_t frame;
            float dist;     // 距離
            float pos[3];   // 注視点(x, y, z)
            float rot[3];   // 回転(x, y, z)
            uint8_t interp[24]; // 補間 (6軸 * 4点)
            uint32_t fov;
            uint8_t perspective; // 0:ON, 1:OFF (今回は無視)
        };
#pragma pack(pop)

        for (uint32_t i = 0; i < cameraCount; ++i)
        {
            RawCameraFrame rc{};
            ifs.read(reinterpret_cast<char*>(&rc), sizeof(rc));
            if (!ifs) break;

            VMDCameraKeyFrame kf{};
            kf.frame = (float)rc.frame;
            kf.distance = rc.dist; // そのまま読み込む（距離）

            // 座標変換 (MMD: 右手系 → DirectX: 左手系)
            // Z軸を反転させるのが一般的
            kf.pos = { rc.pos[0], rc.pos[1], -rc.pos[2] };

            // 回転 (オイラー角)
            // MMDの回転は X軸, Y軸, Z軸。Z軸反転の影響で X,Y回転も符号反転が必要な場合があるが、
            // いったん「X反転, Y反転, Zそのまま」で試すのが定石（計算式による）
            kf.rot = { -rc.rot[0], -rc.rot[1], rc.rot[2] };

            kf.fov = (int)rc.fov;

            // 補間曲線
            // VMDの並び順は [X, Y, Z, Rot, Dist, FOV] の順で各4バイト
            for (int j = 0; j < 6; ++j) {
                kf.interp[j][0] = rc.interp[j * 4 + 0];
                kf.interp[j][1] = rc.interp[j * 4 + 1];
                kf.interp[j][2] = rc.interp[j * 4 + 2];
                kf.interp[j][3] = rc.interp[j * 4 + 3];
            }

            m_cameraAnim.keyframes.push_back(std::move(kf));
        }

        // フレーム順にソート
        auto& kfs = m_cameraAnim.keyframes;
        std::sort(kfs.begin(), kfs.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
            });
    }
    // ★★★ 追加ここまで ★★★

    // --- 1. 照明データ（スキップ） ---
    uint32_t lightCount = 0;
    ifs.read(reinterpret_cast<char*>(&lightCount), sizeof(uint32_t));
    if (ifs.good() && lightCount > 0) {
        // frame(4) + color(12) + pos(12) = 28 bytes
        ifs.seekg(lightCount * 28, std::ios_base::cur);
    }

    // --- 2. 影データ（スキップ） ---
    uint32_t shadowCount = 0;
    ifs.read(reinterpret_cast<char*>(&shadowCount), sizeof(uint32_t));
    if (ifs.good() && shadowCount > 0) {
        // frame(4) + mode(1) + dist(4) = 9 bytes
        ifs.seekg(shadowCount * 9, std::ios_base::cur);
    }

    // --- 3. IK/表示枠データ読み込み ---
    uint32_t ikDispCount = 0;
    ifs.read(reinterpret_cast<char*>(&ikDispCount), sizeof(uint32_t));
    if (ifs.good() && ikDispCount > 0) {
        DebugPrintW(L"[VMDLoader] IK/表示枠キーフレーム数: %u\n", ikDispCount);

        // ループに入る「前」に reserve を追加してメモリ確保を1回にまとめる！
        m_displayIKAnim.reserve(ikDispCount);
        for (uint32_t i = 0; i < ikDispCount; ++i) {
            VMDDisplayIKKeyFrame kf{};
            uint32_t frame = 0;
            uint8_t show = 0;
            uint32_t ikCount = 0;

            ifs.read(reinterpret_cast<char*>(&frame), sizeof(uint32_t));
            ifs.read(reinterpret_cast<char*>(&show), sizeof(uint8_t));
            ifs.read(reinterpret_cast<char*>(&ikCount), sizeof(uint32_t));

            kf.frame = static_cast<float>(frame);
            kf.show = (show != 0);

            for (uint32_t j = 0; j < ikCount; ++j) {
                char sjisName[20];
                uint8_t enable = 0;
                ifs.read(sjisName, 20);
                ifs.read(reinterpret_cast<char*>(&enable), sizeof(uint8_t));

                int realLen = 20;
                while (realLen > 0 && sjisName[realLen - 1] == '\0') realLen--;
                std::string sjisStr(sjisName, realLen);
                std::string nameUTF8 = SJISToUTF8(sjisStr.c_str());

                VMDIKState state;
                state.boneName = CanonicalizeBoneName(nameUTF8);
                state.enable = (enable != 0);
                kf.ikStates.push_back(state);
            }
            m_displayIKAnim.push_back(std::move(kf)); // std::moveで無駄な配列コピーを回避
        }
        // フレーム順にソート
        std::sort(m_displayIKAnim.begin(), m_displayIKAnim.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
            });
    }

    return true;
}

// ------------------------------------------------------------
// 別のVMDデータを統合する（カメラVMDなどを合体させる用）
// ------------------------------------------------------------
void VMDLoader::Merge(const VMDLoader& other)
{
    // 1. ボーンアニメの結合（もしあれば）
    for (const auto& kv : other.m_boneAnims) {
        // 同じボーン名がなければ追加、あれば上書き（または追加）
        // カメラVMDにはボーンは入っていないはずですが念のため
        m_boneAnims[kv.first] = kv.second;
    }

    // 2. モーフアニメの結合
    for (const auto& kv : other.m_morphAnims) {
        m_morphAnims[kv.first] = kv.second;
    }

    // 3. カメラデータの結合（これが本命！）
    // もし相手(other)がカメラデータを持っていれば、自分(this)にコピーする
    if (!other.m_cameraAnim.keyframes.empty()) {
        m_cameraAnim = other.m_cameraAnim;

        // ログを出しておくと安心です
   		DebugPrintW(L"[VMDLoader] カメラデータをマージしました。キー数: %zu\n", m_cameraAnim.keyframes.size());
    }
}