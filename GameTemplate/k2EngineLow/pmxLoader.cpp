#include "k2EngineLowPreCompile.h"
#include "PMXLoader.h"
#include <iostream>
#include <cstdarg>
#include <algorithm>
#include <functional>

#undef min
#undef max

/// <summary>
/// 下限上限を決めて値を特定の範囲内に収める（挟み込む）関数
/// </summary>
/// <typeparam name="T"></typeparam>
/// <param name="value"></param>
/// <param name="minVal"></param>
/// <param name="maxVal"></param>
/// <returns></returns>
template <typename T>
T clamp(const T& value, const T& minVal, const T& maxVal)
{
    return (value < minVal) ? minVal : (value > maxVal ? maxVal : value);
}
using namespace nsK2EngineLow;
using namespace DirectX;

// 先頭付近（using namespace の後あたり）に追加
static void DumpBytes(std::ifstream& ifs, size_t count, const char* title = nullptr)
{
#ifdef _DEBUG
    std::streampos oldPos = ifs.tellg();
    if (!ifs.good() || oldPos < 0) {
        return;
    }

    std::vector<unsigned char> buf(count);
    ifs.read(reinterpret_cast<char*>(buf.data()), count);
    std::streamsize readBytes = ifs.gcount();

    // 必ず元の位置に戻す
    ifs.clear();
    ifs.seekg(oldPos);

    if (title) {
        OutputDebugStringA(title);
        OutputDebugStringA("\n");
    }
    else {
        OutputDebugStringA("HexDump:\n");
    }

    char line[256];
    for (std::streamsize i = 0; i < readBytes; i += 16) {
        int offset = (int)i;
        sprintf_s(line, "%04X: ", offset);
        OutputDebugStringA(line);

        // 16進
        for (int j = 0; j < 16 && (i + j) < readBytes; ++j) {
            sprintf_s(line, "%02X ", buf[(size_t)i + j]);
            OutputDebugStringA(line);
        }
        OutputDebugStringA(" | ");

        // ASCII
        for (int j = 0; j < 16 && (i + j) < readBytes; ++j) {
            unsigned char c = buf[(size_t)i + j];
            char ch[2] = { (c >= 32 && c <= 126) ? (char)c : '.', '\0' };
            OutputDebugStringA(ch);
        }
        OutputDebugStringA("\n");
    }
    OutputDebugStringA("\n");
#endif
}


//// デバッグ出力用のヘルパー関数
////これ呼び出すとデバックコンソールに出力される。
//void DebugPrint(const char* format, ...) {
//    char buf[512];
//    va_list args;
//    va_start(args, format);
//    vsprintf_s(buf, format, args);
//    va_end(args);
//    OutputDebugStringA(buf);
//}
////ワイド文字版デバッグ出力ヘルパー関数。
//void DebugPrintW(const wchar_t* format, ...) {
//    wchar_t buf[512];
//    va_list args;
//    va_start(args, format);
//    vswprintf_s(buf, format, args);
//    va_end(args);
//    OutputDebugStringW(buf);
//}

bool PMXLoader::LoadFromFile(const std::string& path, PMXModel& outModel)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        DebugPrint("PMX file open failed: %s\n", path.c_str());
        return false;
    }
    // --- 追加：基準ディレクトリを取り出す ---
    std::string baseDir = path.substr(0, path.find_last_of("/\\") + 1);

    PMXHeader header;
    if (!LoadPMXHeader(ifs, header)) {
        DebugPrint("PMX header load failed.\n");
        return false;
    }
	outModel.header = header;

    DebugPrint("PMX Version: %.2f | DataSize: %d | Encoding: %d | AddUV: %d | VertexIndexSize: %d | TextureIndexSize: %d |materialIndexSize:%d  | boneIndexSize: %d  |  morphIndexSize: %d\n",
        header.version,
        header.dataSize,
        header.global[0],
        header.global[1],
        header.global[2],
        header.global[3],
        header.global[4],
        header.global[5],
        header.global[6]);

    int textEncoding = header.global[0];  // 0=UTF16, 1=UTF8
    int addUVCount = header.global[1];
    int vertexIndexSize = header.global[2];
    int textureIndexSize = header.global[3];
    int materialIndexSize = header.global[4];
    int boneIndexSize = header.global[5];
    int morphIndexSize = header.global[6];
    int rigidIndexSize = header.global[7];

    // モデル名とコメント
    DebugPrint("File position before model name: %lld\n", static_cast<long long>(ifs.tellg()));
    outModel.modelName = readString(ifs, textEncoding);
    DebugPrint("File position after model name: %lld\n", static_cast<long long>(ifs.tellg()));

    DebugPrint("File position before model nameEng: %lld\n", static_cast<long long>(ifs.tellg()));
    outModel.modelNameEnglish = readString(ifs, textEncoding);
    DebugPrint("File position after model nameEng: %lld\n", static_cast<long long>(ifs.tellg()));

    DebugPrint("File position before comment: %lld\n", static_cast<long long>(ifs.tellg()));
    outModel.comment = readString(ifs, textEncoding);
    DebugPrint("File position after comment: %lld\n", static_cast<long long>(ifs.tellg()));

    DebugPrint("File position before commentEng: %lld\n", static_cast<long long>(ifs.tellg()));
    outModel.commentEnglish = readString(ifs, textEncoding);
    DebugPrint("File position after commentEng: %lld\n", static_cast<long long>(ifs.tellg()));

    // ファイルの次の部分を16進数でダンプ（デバッグ用）
    DumpBytes(ifs, 128, "Next 128 bytes after comment");


    // ★修正: Utf8ToWstring で変換してから、どちらも DebugPrintW で出力する！
    DebugPrintW(L"ModelName: %s\n", Utf8ToWstring(outModel.modelName).c_str());
    DebugPrintW(L"Comment: %s\n", Utf8ToWstring(outModel.comment).c_str());

	///モデルの名前とコメントをワイド文字で出力する。
    //// 読み込んだ文字列を wchar_t に変換して表示したい場合こちらを使用
    //std::wstring wModelName = Utf8ToWstring(outModel.modelName);
    //std::wstring wComment = Utf8ToWstring(outModel.comment);
    //// Wide 出力（必要なら Visual Studio の Output ウィンドウで確認）
    //OutputDebugStringW(L"Wide ModelName: ");
    //OutputDebugStringW(wModelName.c_str());
    //OutputDebugStringW(L"\nWide Comment: ");
    //OutputDebugStringW(wComment.c_str());
    //OutputDebugStringW(L"\n");

    // 頂点数を読み込む前に、実際のデータを確認
    std::streampos vertexPos = ifs.tellg();
    DebugPrint("File position before vertexCount: %lld\n", static_cast<long long>(vertexPos));

    // 次の32バイトを16進数で確認（位置を変えない）
    DumpBytes(ifs, 32, "Next 32 bytes before vertexCount");

    // コメント読み込み直後あたりに
    std::streampos pos = ifs.tellg();
    DebugPrint("File position before vertexCount: %lld\n", static_cast<long long>(pos));

    // 16バイトくらい覗いてみる（位置を変えない）
    DumpBytes(ifs, 16, "Next 16 bytes before vertexCount");

    // 頂点数
    uint32_t vertexCount = 0;
    ifs.read(reinterpret_cast<char*>(&vertexCount), sizeof(uint32_t));

    if (!ifs.good()) {
        DebugPrint("Failed to read vertexCount.\n");
        return false;
    }

    DebugPrint("Raw vertexCount: %u (0x%08X)\n", vertexCount, vertexCount);

    // 常識的な上限チェック（必要に応じて調整）
    constexpr uint32_t MAX_VERTEX_COUNT = 5000000;

    if (vertexCount == 0 || vertexCount > MAX_VERTEX_COUNT) {
        DebugPrint("Invalid vertexCount detected: %u\n", vertexCount);
        return false;
    }

    DebugPrint("Final VertexCount: %u\n", vertexCount);

    outModel.vertices.reserve(vertexCount);


    // 頂点データ
    for (uint32_t i = 0; i < vertexCount; ++i) {
        PMXVertex v{};
        // --- デバッグ: 頂点 i の直前のファイル位置と生データをダンプ ---
        std::streampos vPos = ifs.tellg();
        DebugPrint("Vertex %u start filepos: %lld\n", i, static_cast<long long>(vPos));

        // 先頭から 32 バイトを覗く（位置は戻される）
        //DumpBytes(ifs, 32, "Vertex peek");
        // --- デバッグここまで ---
        // 基本情報
        ifs.read(reinterpret_cast<char*>(v.pos), sizeof(float) * 3);
        ifs.read(reinterpret_cast<char*>(v.normal), sizeof(float) * 3);
        ifs.read(reinterpret_cast<char*>(v.uv), sizeof(float) * 2);

        // 追加UV（最大4個まで）
        int safeAddUVCount = addUVCount;
        if (safeAddUVCount < 0) safeAddUVCount = 0;
        if (safeAddUVCount > 4) safeAddUVCount = 4;

        v.addUVs.resize(safeAddUVCount);
        for (int j = 0; j < safeAddUVCount; ++j) {
            float temp[4];
            ifs.read(reinterpret_cast<char*>(temp), sizeof(float) * 4);
            v.addUVs[j] = { temp[0], temp[1], temp[2], temp[3] };
        }

        // スキニングタイプ
        uint8_t weightType = 0;
        ifs.read(reinterpret_cast<char*>(&weightType), 1);
        v.skinningType = weightType;

        switch (weightType) {
        case 0: { // BDEF1
            v.boneIndices.resize(1);
            v.boneIndices[0] = readIndex(ifs, boneIndexSize);
            v.boneWeights = { 1.0f };
            break;
        }
        case 1: { // BDEF2
            v.boneIndices.resize(2);
            for (int j = 0; j < 2; ++j)
                v.boneIndices[j] = readIndex(ifs, boneIndexSize);
            float w;
            ifs.read(reinterpret_cast<char*>(&w), sizeof(float));
            v.boneWeights = { w, 1.0f - w };
            break;
        }
        case 2: { // BDEF4
            v.boneIndices.resize(4);
            for (int j = 0; j < 4; ++j)
                v.boneIndices[j] = readIndex(ifs, boneIndexSize);
            v.boneWeights.resize(4);
            ifs.read(reinterpret_cast<char*>(v.boneWeights.data()), sizeof(float) * 4);
            break;
        }
        case 3: { // SDEF
            v.boneIndices.resize(2);
            for (int j = 0; j < 2; ++j)
                v.boneIndices[j] = readIndex(ifs, boneIndexSize);

            // ★ SDEF は BDEF2 と同様に weight (float) が来る
            float w = 0.5f;
            ifs.read(reinterpret_cast<char*>(&w), sizeof(float));
            // C, R0, R1 (each 3 floats)
            float sdefC[3], sdefR0[3], sdefR1[3];
            ifs.read(reinterpret_cast<char*>(sdefC), sizeof(float) * 3);
            ifs.read(reinterpret_cast<char*>(sdefR0), sizeof(float) * 3);
            ifs.read(reinterpret_cast<char*>(sdefR1), sizeof(float) * 3);

            v.boneWeights = { w, 1.0f - w, 0.0f, 0.0f };

            // 必要なら sdefC/R0/R1 を PMXVertex に格納するフィールドを追加して保存
            break;
        }
        case 4: { // QDEF
            v.boneIndices.resize(4);
            for (int j = 0; j < 4; ++j)
                v.boneIndices[j] = readIndex(ifs, boneIndexSize);
            v.boneWeights.resize(4);
            ifs.read(reinterpret_cast<char*>(v.boneWeights.data()), sizeof(float) * 4);
            break;
        }
        default: {
            DebugPrint("Unknown weightType encountered (%d) at vertex %d\n", weightType, i);
            v.boneIndices.resize(1);
            v.boneIndices[0] = 0;
            v.boneWeights = { 1.0f, 0, 0, 0 };
            break;
        }
        }

        // EdgeScale 読み飛ばし
        float edgeScale;
        ifs.read(reinterpret_cast<char*>(&edgeScale), sizeof(float));

        outModel.vertices.push_back(std::move(v));

        //// デバッグ出力
        //char buf[256];
        //sprintf_s(buf, "Vertex[%d]: weightType=%d Bone0=%d Weight=%.2f\n",
        //    i, v.skinningType, v.boneIndices[0], v.boneWeights[0]);
        //OutputDebugStringA(buf);

      // ==== 修正後：安全なデバッグ出力 ====
        // ボーン情報が1つ以上入っている時だけ、デバッグ出力をする
        if (v.boneIndices.size() > 0 && v.boneWeights.size() > 0) {
            DebugPrint("Vertex[%d]: weightType=%d Bone0=%d Weight=%.2f\n",
                i, v.skinningType, v.boneIndices[0], v.boneWeights[0]);
        }
        else {
            // ボーンが無い場合（ステージ等）はこちらを出力する
            DebugPrint("Vertex[%d]: weightType=%d No Bone Data\n",
                i, v.skinningType);
        }
    }


   // インデックスデータの読み込み
    uint32_t indexCount = 0;
    std::streampos indexPos = ifs.tellg();
    ifs.read(reinterpret_cast<char*>(&indexCount), sizeof(uint32_t));

    if (!ifs.good()) {
        DebugPrint("Failed to read indexCount.\n");
        return false;
    }

    DebugPrint("File position before indexCount: %lld\n",
        static_cast<long long>(indexPos));
    DebugPrint("Raw indexCount: %u (0x%08X)\n", indexCount, indexCount);

    // インデックス数の常識的な範囲チェック
    constexpr uint32_t MAX_INDEX_COUNT = 15000000; // 必要なら調整

    if (indexCount == 0 || indexCount > MAX_INDEX_COUNT) {
        DebugPrint("Invalid indexCount detected: %u\n", indexCount);
        return false;
    }

    DebugPrint("Final IndexCount: %u\n", indexCount);

    outModel.indices.reserve(indexCount);
    for (uint32_t i = 0; i < indexCount; ++i) {
        outModel.indices.push_back(readIndex(ifs, vertexIndexSize));
    }


    // ファイル状態をチェック
    if (!ifs.good()) {
        DebugPrint("File stream is in error state, attempting to recover...\n");
        ifs.clear();
    }

    DebugPrint("File position before materialCount: %lld\n", static_cast<long long>(ifs.tellg()));
    //------------------------------------------------------
    // テクスチャリストの読み込み（マテリアルの前！）
   //------------------------------------------------------
       // --- テクスチャパス ---
    int32_t textureCount = 0;
    ifs.read(reinterpret_cast<char*>(&textureCount), sizeof(int32_t));
    outModel.textures.reserve(textureCount);
    for (int i = 0; i < textureCount; ++i) {
        std::string tex = readString(ifs, header.global[0]);
        // ---- ここで絶対パス補完する ----
        if (!tex.empty()) {
            outModel.textures.push_back(baseDir + tex);
        }
        else {
            outModel.textures.push_back("");
        }
    }
    //
    DebugPrint("IndexSizes: Vertex=%d, Tex=%d, Mat=%d, Bone=%d, Morph=%d, Rigid=%d\n",
        header.global[2], header.global[3], header.global[4],
        header.global[5], header.global[6], header.global[7]);
    // ---- 安全なマテリアル読み込みブロック（これで置き換えて） ----
    int32_t materialCount = 0;
    ifs.read(reinterpret_cast<char*>(&materialCount), sizeof(int32_t));
    if (!ifs.good()) {
        DebugPrint("ERROR: failed reading materialCount. fail=%d bad=%d eof=%d\n", ifs.fail(), ifs.bad(), ifs.eof());
        return false;
    }
    DebugPrint("Raw materialCount bytes: %08X (decimal: %d)\n", materialCount, materialCount);

    outModel.materials.clear();
    outModel.materials.reserve(materialCount); // ★ reserveにする

    for (int32_t mi = 0; mi < materialCount; ++mi) {
        std::streampos startPos = ifs.tellg();
        DebugPrint("Material[%d] start at filepos: %lld\n", mi, static_cast<long long>(startPos));

        PMXMaterial mat;
        // 名前
        DebugPrint("about to read material name at pos: %lld\n", static_cast<long long>(ifs.tellg()));
        std::string name = readString(ifs, header.global[0]);
        DebugPrint("after readString(name) pos: %lld, nameLen=%zu\n", static_cast<long long>(ifs.tellg()), name.size());

        mat.name = name; // ← ここを修正

        if (!ifs.good()) { DebugPrint("ERROR: readString(name) failed at material %d\n", mi); return false; }
        mat.nameEnglish = readString(ifs, header.global[0]);
        if (!ifs.good()) { DebugPrint("ERROR: readString(nameEng) failed at material %d\n", mi); return false; }

        // 基本色系
        ifs.read(reinterpret_cast<char*>(mat.diffuse), sizeof(float) * 4);
        if (!ifs.good()) { DebugPrint("ERROR: read diffuse failed at material %d\n", mi); return false; }
        ifs.read(reinterpret_cast<char*>(mat.specular), sizeof(float) * 3);
        if (!ifs.good()) { DebugPrint("ERROR: read specular failed at material %d\n", mi); return false; }
        ifs.read(reinterpret_cast<char*>(&mat.shininess), sizeof(float));
        if (!ifs.good()) { DebugPrint("ERROR: read shininess failed at material %d\n", mi); return false; }
        ifs.read(reinterpret_cast<char*>(mat.ambient), sizeof(float) * 3);
        if (!ifs.good()) { DebugPrint("ERROR: read ambient failed at material %d\n", mi); return false; }

        // 描画フラグ（1byte）
        ifs.read(reinterpret_cast<char*>(&mat.flags), 1);
        if (!ifs.good()) { DebugPrint("ERROR: read flags failed at material %d\n", mi); return false; }

        // EdgeColor, EdgeSize
        ifs.read(reinterpret_cast<char*>(mat.edgeColor), sizeof(float) * 4);
        if (!ifs.good()) { DebugPrint("ERROR: read edgeColor failed at material %d\n", mi); return false; }
        ifs.read(reinterpret_cast<char*>(&mat.edgeSize), sizeof(float));
        if (!ifs.good()) { DebugPrint("ERROR: read edgeSize failed at material %d\n", mi); return false; }

        // テクスチャインデックス（可変サイズ）
        int32_t texIndex = readIndex(ifs, header.global[3]);
        if (!ifs.good()) { DebugPrint("ERROR: read texIndex failed at material %d\n", mi); return false; }
        int32_t sphereIndex = readIndex(ifs, header.global[3]);
        if (!ifs.good()) { DebugPrint("ERROR: read sphereIndex failed at material %d\n", mi); return false; }

        // スフィアモード・トゥーンフラグ
        uint8_t sphereMode = 0;
        ifs.read(reinterpret_cast<char*>(&sphereMode), 1);
        if (!ifs.good()) { DebugPrint("ERROR: read sphereMode failed at material %d\n", mi); return false; }
        uint8_t toonFlag = 0;
        ifs.read(reinterpret_cast<char*>(&toonFlag), 1);
        if (!ifs.good()) { DebugPrint("ERROR: read toonFlag failed at material %d\n", mi); return false; }

        int32_t toonIndex = -1;
        if (toonFlag == 0) {
            // 共通Toon は 1 byte ID
            uint8_t toonShared = 0;
            ifs.read(reinterpret_cast<char*>(&toonShared), 1);
            if (!ifs.good()) { DebugPrint("ERROR: read toonShared failed at material %d\n", mi); return false; }
            toonIndex = static_cast<int32_t>(toonShared); // - but treated separately
        }
        else {
            // 個別Toon は index
            toonIndex = readIndex(ifs, header.global[3]);
            if (!ifs.good()) { DebugPrint("ERROR: read toonIndex failed at material %d\n", mi); return false; }
        }

        // memo（文字列）
        std::string memo = readString(ifs, header.global[0]);
        if (!ifs.good()) { DebugPrint("ERROR: read memo failed at material %d\n", mi); return false; }

        // faceCount
        int32_t faceCount = 0;
        ifs.read(reinterpret_cast<char*>(&faceCount), sizeof(int32_t));
        if (!ifs.good()) { DebugPrint("ERROR: read faceCount failed at material %d\n", mi); return false; }
        mat.faceCount = faceCount;

        // 範囲チェックして安全に格納（-1は「なし」）
        if (texIndex >= 0 && texIndex < static_cast<int>(outModel.textures.size()))
            mat.textureFile = outModel.textures[texIndex];
        else
            mat.textureFile.clear();

        if (sphereIndex >= 0 && sphereIndex < static_cast<int>(outModel.textures.size()))
            mat.sphereTextureFile = outModel.textures[sphereIndex];
        else
            mat.sphereTextureFile.clear();

        // toonIndex: 共通Toon（0..n）を扱うか個別Indexかはアプリ側で解釈
        if (toonFlag == 0) {
            // 共通Toon -> store as toonTextureFile empty and maybe store ID elsewhere.
            mat.toonTextureFile.clear();
        }
        else {
            if (toonIndex >= 0 && toonIndex < static_cast<int>(outModel.textures.size()))
                mat.toonTextureFile = outModel.textures[toonIndex];
            else
                mat.toonTextureFile.clear();
        }
        // ★修正: DebugPrintW と L"..." に変更し、mat.name を Utf8ToWstring で包む！
        DebugPrintW(L"Material[%d]: name='%s' texIdx=%d sphereIdx=%d toonFlag=%d toonIdx=%d faceCount=%d filepos_after=%lld\n",
            mi, Utf8ToWstring(mat.name).c_str(), texIndex, sphereIndex, toonFlag, toonIndex, faceCount, static_cast<long long>(ifs.tellg()));

        outModel.materials.push_back(std::move(mat)); // ★ push_backにする

    } // end materials loop

    // 最後に状態確認
    if (!ifs.good()) {
        DebugPrint("Stream not good after materials: fail=%d bad=%d eof=%d\n", ifs.fail(), ifs.bad(), ifs.eof());
        // ここで false にして上位で対処する方が安全
        return false;
    }

    //// ファイル状態をチェック
    //if (!ifs.good()) {
    //    DebugPrint("File stream is in error state before boneCount, attempting to recover...\n");
    //    ifs.clear();
    //}

    //DebugPrint("File position before boneCount: %lld\n", static_cast<long long>(ifs.tellg()));

    // =================================================================
   // ボーン読み込み 
   // =================================================================
    try {
        int32_t boneCount = 0;
        ifs.read(reinterpret_cast<char*>(&boneCount), sizeof(int32_t));

        if (!ifs.good() || boneCount < 0 || boneCount > 10000) {
            throw std::runtime_error("Invalid boneCount detected.");
        }

        outModel.bones.reserve(boneCount); // ★ reserve を使う
        DebugPrint("BoneCount: %d\n", boneCount);

        for (int i = 0; i < boneCount; ++i) {
            PMXBone b; // ★ 参照ではなく新規作成
            std::streampos boneStartPos = ifs.tellg();
            //DebugPrint("Bone[%d] start pos: %lld\n", i, static_cast<long long>(boneStartPos));

            b.name = readString(ifs, textEncoding);
            b.enName = readString(ifs, textEncoding);
            // ★★★ ここから追加：全角の「ＩＫ」を半角「IK」に強制統一！ ★★★
            auto replaceAll = [](std::string& str, const std::string& from, const std::string& to) {
                size_t start_pos = 0;
                while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                    str.replace(start_pos, from.length(), to);
                    start_pos += to.length();
                }
                };

            // 悪さをする全角文字を徹底的に半角に書き換える
            replaceAll(b.name, u8"ＩＫ", "IK");
            replaceAll(b.name, u8"Ｉ", "I"); // 念のため単体でも
            replaceAll(b.name, u8"Ｋ", "K"); // 念のため単体でも

            replaceAll(b.enName, u8"ＩＫ", "IK");
            // ★★★ 追加ここまで ★★★
			DebugPrintW(L"Bone[%d] name: %s\n", i, Utf8ToWstring(b.name).c_str());

            ifs.read(reinterpret_cast<char*>(&b.position), sizeof(Vector3));
            b.parentIndex = readIndex(ifs, boneIndexSize);
            ifs.read(reinterpret_cast<char*>(&b.deformLayer), sizeof(int32_t));
            ifs.read(reinterpret_cast<char*>(&b.boneFlag), sizeof(uint16_t));

            // フラグに基づく可変データ
            if (b.boneFlag & PMXBoneFlags::Connection) { // 接続先ボーン
                b.connectIndex = readIndex(ifs, boneIndexSize);
            }
            else { // オフセット
                ifs.read(reinterpret_cast<char*>(&b.offset), sizeof(Vector3));
            }

            if (b.boneFlag & PMXBoneFlags::RotGrant || b.boneFlag & PMXBoneFlags::MoveGrant) { // 付与親
                b.grantParentIndex = readIndex(ifs, boneIndexSize);
                ifs.read(reinterpret_cast<char*>(&b.grantWeight), sizeof(float));

                b.isLocalGrant = (b.boneFlag & PMXBoneFlags::RotGrant) != 0;   // ざっくり：回転付与フラグをローカル付与扱い
                b.isTranslationGrant = (b.boneFlag & PMXBoneFlags::MoveGrant) != 0;   // true なら移動付与、false なら回転付与
            }

            if (b.boneFlag & PMXBoneFlags::AxisFixed) { // 軸固定
                ifs.read(reinterpret_cast<char*>(&b.axisDirection), sizeof(Vector3));
            }

            if (b.boneFlag & PMXBoneFlags::LocalAxis) { // ローカル軸
                ifs.read(reinterpret_cast<char*>(&b.localX), sizeof(Vector3));
                ifs.read(reinterpret_cast<char*>(&b.localZ), sizeof(Vector3));
            }

            if (b.boneFlag & PMXBoneFlags::ExternalParent) { // 外部親
                ifs.read(reinterpret_cast<char*>(&b.externalKey), sizeof(int32_t));
            }

            if (b.boneFlag & PMXBoneFlags::IK) { // IK
                b.hasIK = true;
                b.ikTargetIndex = readIndex(ifs, boneIndexSize);
                ifs.read(reinterpret_cast<char*>(&b.ikLoopCount), sizeof(int32_t));
                ifs.read(reinterpret_cast<char*>(&b.ikLimitAngle), sizeof(float));

                uint32_t ikLinkCount = 0;
                ifs.read(reinterpret_cast<char*>(&ikLinkCount), sizeof(uint32_t));

                // ★★★ 異常なIKリンク数を検出してクラッシュを回避する ★★★
                if (ikLinkCount > 256 || !ifs.good()) { // 256は安全マージン。MMDモデルでこれを超えることはまずない
                    DebugPrint("WARN: Suspicious ikLinkCount (%u) at Bone[%d]. Skipping IK links to prevent crash.\n", ikLinkCount, i);
                    // このボーンのIKデータは壊れていると判断し、読み込みをスキップする
                    // ただし、ファイルポインタを次のボーンの開始位置まで進める必要があるが、
                    // 破損データの長さが不明なため、これ以上の読み込みは危険。
                    // ここで読み込みを中断するのが最も安全。
                    throw std::runtime_error("Corrupted IK link data.");
                }

                b.ikLinks.resize(ikLinkCount);
                for (uint32_t j = 0; j < ikLinkCount; ++j) {
                    PMXBone::IKLink& link = b.ikLinks[j];
                    link.linkBoneIndex = readIndex(ifs, boneIndexSize);
                    ifs.read(reinterpret_cast<char*>(&link.hasLimit), 1);
                    if (link.hasLimit) {
                        ifs.read(reinterpret_cast<char*>(&link.limitMin), sizeof(Vector3));
                        ifs.read(reinterpret_cast<char*>(&link.limitMax), sizeof(Vector3));
                    }
                }
            }
            // --- ループの最後で必ず補正する ---
            if (b.parentIndex >= 0) {
                b.hasParent = true;
            }
            else {
                b.hasParent = false;
            }

            // ★★★ ここが超重要！作ったボーン b をリストに格納する ★★★
            outModel.bones.push_back(std::move(b));
           // DebugPrint("==== Bone[%d] section end at %lld ====\n", i, static_cast<long long>(ifs.tellg()));
        }
        for (size_t i = 0; i < outModel.bones.size(); ++i) {
            const auto& b = outModel.bones[i];
            DebugPrintW(L"[Bone %zu] name=%s parent=%d\n", i, Utf8ToWstring(b.name), b.parentIndex);
            
        }
    }
    catch (const std::exception& e) {
        DebugPrintW(L"FATAL ERROR during bone parsing: %s\n", e.what());
        DebugPrint("The model file seems to be corrupted. Aborting load.\n");
        return false;
    }


	// モーフデータの読み込み
    LoadMorphs(ifs,header,outModel.morphs); 

	// 表示枠データの読み込み
    LoadDisplayFrames(ifs, header, outModel.displayFrames);


    LoadRigidBodies(ifs,header,outModel.rigidBodies);

    // ジョイントデータの読み込み
	LoadJoints(ifs, header, outModel.joints);

  

    DebugPrint("PMX Load completed successfully!\n");
    DebugPrint("Total vertices: %d\n", (int)outModel.vertices.size());
    DebugPrint("Total indices: %d\n", (int)outModel.indices.size());
    DebugPrint("Total materials: %d\n", (int)outModel.materials.size());
    DebugPrint("Total bones: %d\n", (int)outModel.bones.size());
	DebugPrint("Total morphs: %d\n", (int)outModel.morphs.size());
	DebugPrint("Total display frames: %d\n", (int)outModel.displayFrames.size());
	DebugPrint("Total rigid bodies: %d\n", (int)outModel.rigidBodies.size());
	DebugPrint("Total joints: %d\n", (int)outModel.joints.size());
    for (size_t i = 0; i < outModel.bones.size(); i++) {
        const auto& b = outModel.bones[i];
        DebugPrintW(L"Bone[%d] pos=(%.3f, %.3f, %.3f) parent=%d name=%s\n",
            (int)i, b.position.x, b.position.y, b.position.z, b.parentIndex, Utf8ToWstring(b.name));
    }
    PostProcess(outModel);

    return true;
}

bool PMXLoader::LoadPMXHeader(std::ifstream& ifs, PMXHeader& header)
{
    ifs.read(header.magic, 4);
    if (strncmp(header.magic, "PMX ", 4) != 0) return false;

    ifs.read(reinterpret_cast<char*>(&header.version), sizeof(float));
    ifs.read(reinterpret_cast<char*>(&header.dataSize), 1);
    std::streampos pos = ifs.tellg();
    DebugPrint("FilePos before global: %d\n", (int)pos);
    ifs.read(reinterpret_cast<char*>(header.global), header.dataSize);
    // デバッグ：global の中身確認
    DebugPrint("Global bytes: ");
    for (int i = 0; i < header.dataSize; i++) {
        DebugPrint("%02X ", header.global[i]);
    }
    DebugPrint("\n");
    DebugPrint("PMX Version: %.2f | DataSize: %d | "
        "VertexIndexSize: %d | TextureIndexSize: %d | MaterialIndexSize: %d | "
        "BoneIndexSize: %d | MorphIndexSize: %d | RigidIndexSize: %d\n",
        header.version, header.dataSize,
        header.global[2], header.global[3], header.global[4],
        header.global[5], header.global[6], header.global[7]);
    return true;
}

/// <summary>
/// PMX形式の文字列を読み込む
/// </summary>
std::string PMXLoader::readString(std::ifstream& ifs, int encoding)
{
    std::streampos startPos = ifs.tellg();
    int32_t len = 0;
    ifs.read(reinterpret_cast<char*>(&len), sizeof(int32_t));
    if (!ifs.good()) return "";

    constexpr int32_t MAX_LEN = 65536;
    if (len < 0 || len > MAX_LEN) { ifs.setstate(std::ios::failbit); return ""; }
    if (len == 0) return "";

    std::vector<char> buf(len);
    ifs.read(buf.data(), len);
    if (!ifs.good()) return "";

    if (encoding == 0) { // UTF-16
        const wchar_t* wdata = reinterpret_cast<const wchar_t*>(buf.data());
        int wcharCount = len / 2;
        int ulen = WideCharToMultiByte(CP_UTF8, 0, wdata, wcharCount, nullptr, 0, nullptr, nullptr);
        if (ulen <= 0) return "";
        std::string utf8(ulen, 0);
        WideCharToMultiByte(CP_UTF8, 0, wdata, wcharCount, &utf8[0], ulen, nullptr, nullptr);
        return utf8;
    }
    else { // UTF-8
        return std::string(buf.data(), len);
    }
}

std::wstring PMXLoader::readStringW(std::ifstream& ifs, int encoding)
{
    std::streampos startPos = ifs.tellg();
    int32_t len = 0;
    ifs.read(reinterpret_cast<char*>(&len), sizeof(int32_t));
    if (!ifs.good()) return L"";

    constexpr int32_t MAX_LEN = 65536;
    if (len < 0 || len > MAX_LEN) { ifs.setstate(std::ios::failbit); return L""; }
    if (len == 0) return L"";

    std::vector<char> buf(len);
    ifs.read(buf.data(), len);
    if (!ifs.good()) return L"";

    if (encoding == 0) { // UTF-16
        std::wstring w;
        w.resize(len / 2);
        memcpy(&w[0], buf.data(), len);
        return w;
    }
    else { // UTF-8
        int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), len, nullptr, 0);
        if (wlen <= 0) return L"";
        std::wstring wide(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, buf.data(), len, &wide[0], wlen);
        return wide;
    }
}
int32_t PMXLoader::readIndex(std::ifstream& ifs, int indexSize)
{
    // 1 byte index → int8_t
    if (indexSize == 1) {
        int8_t v = 0;
        ifs.read(reinterpret_cast<char*>(&v), 1);
        if (!ifs.good()) return -1;
        return static_cast<int32_t>(v);
    }

    // 2 byte index → int16_t (LE)
    else if (indexSize == 2) {
        int16_t v = 0;
        uint8_t raw[2];
        ifs.read(reinterpret_cast<char*>(raw), 2);
        if (!ifs.good()) return -1;

        // 正しく符号付き LE を構築
        v = static_cast<int16_t>((raw[1] << 8) | raw[0]);

        return static_cast<int32_t>(v);
    }

    // 4 byte index → int32_t (LE)
    else if (indexSize == 4) {
        int32_t v = 0;
        uint8_t raw[4];
        ifs.read(reinterpret_cast<char*>(raw), 4);
        if (!ifs.good()) return -1;

        v = static_cast<int32_t>(
            (raw[3] << 24) |
            (raw[2] << 16) |
            (raw[1] << 8) |
            (raw[0])
            );

        return v;
    }

    // 不正サイズ
    DebugPrint("Unsupported index size: %d\n", indexSize);
    return -1;
}


std::wstring PMXLoader::Utf8ToWstring(const std::string& utf8)
{
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wide(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wlen);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}

std::string PMXLoader::WstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string utf8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    if (!utf8.empty() && utf8.back() == '\0') utf8.pop_back();
    return utf8;
}

//10/14追記: ボーン読み込み関数の追加
//bool nsK2EngineLow::PMXLoader::LoadBones(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXBone>& bones)
//{
//    int boneCount;
//    ifs.read(reinterpret_cast<char*>(&boneCount), sizeof(int));
//    DebugPrint("BoneCount: %d\n", boneCount);
//
//    bones.resize(boneCount);
//
//    for (int i = 0; i < boneCount; i++) {
//        // --- ボーン名の読み込み（UTF-16を仮に1バイト文字列扱いで読み飛ばす） ---
//        int nameLength;
//        ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(int));
//        std::vector<char> nameBuf(nameLength);
//        ifs.read(nameBuf.data(), nameLength);
//        bones[i].name = std::string(nameBuf.begin(), nameBuf.end());
//
//        // --- 座標 ---
//        ifs.read(reinterpret_cast<char*>(&bones[i].position), sizeof(Vector3));
//
//        // --- 親インデックス ---
//        int32_t parentIndex;
//        ifs.read(reinterpret_cast<char*>(&parentIndex), sizeof(int32_t));
//        bones[i].parentIndex = parentIndex;
//        bones[i].hasParent = (parentIndex >= 0);
//
//        // --- 変形階層 ---
//        ifs.read(reinterpret_cast<char*>(&bones[i].deformLayer), sizeof(int));
//
//        // --- ボーンフラグ ---
//        uint16_t boneFlag;
//        ifs.read(reinterpret_cast<char*>(&boneFlag), sizeof(uint16_t));
//        bones[i].hasIK = (boneFlag & 0x0020); // IKフラグ
//
//        // ここでは詳細なIKデータは一旦スキップ
//        // 実装するなら、boneFlagをチェックして必要分読み飛ばす
//
//        DebugPrint("Bone[%d]: Name=%s Parent=%d Pos=(%.2f, %.2f, %.2f) IK=%d\n",
//            i,
//            bones[i].name.c_str(),
//            bones[i].parentIndex,
//            bones[i].position.x,
//            bones[i].position.y,
//            bones[i].position.z,
//            bones[i].hasIK);
//    }
//
//    return true;
//}

// モーフデータの読み込み
bool PMXLoader::LoadMorphs(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXMorph>& outMorphs)
{
    int32_t morphCount = 0;
    ifs.read(reinterpret_cast<char*>(&morphCount), sizeof(int32_t));
    if (!ifs.good()) {
        DebugPrint("Failed to read morphCount\n");
        return false;
    }

    DebugPrint("Morph count: %d\n", morphCount);
    if (morphCount < 0 || morphCount > 100000) { // 過大値ガード
        DebugPrint("Suspicious morphCount: %d\n", morphCount);
        return false;
    }

    outMorphs.clear();
    outMorphs.reserve(morphCount);

    for (int32_t i = 0; i < morphCount; ++i)
    {
        std::streampos morphStart = ifs.tellg();
        DebugPrint("Morph[%d] start pos: %lld\n", i, (long long)morphStart);

        PMXMorph morph;
        // 名前（日本語名と英語名）
        morph.name = readStringW(ifs, header.global[0]);
        morph.englishName = readString(ifs, header.global[0]);
        if (!ifs.good()) {
            DebugPrintW(L"Stream bad after reading Morph[%d] names (name=%ls)\n", i, morph.name.c_str());
            return false;
        }

        ifs.read(reinterpret_cast<char*>(&morph.controlPanel), sizeof(uint8_t));
        uint8_t type = 0;
        ifs.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));
        morph.morphType = static_cast<PMXMorphType>(type);

        int32_t elementCount = 0;
        ifs.read(reinterpret_cast<char*>(&elementCount), sizeof(int32_t));
        if (!ifs.good()) {
            DebugPrint("Failed to read elementCount for Morph[%d]\n", i);
            return false;
        }

        if (elementCount < 0 || elementCount > 1000000) {
            DebugPrint("Suspicious elementCount %d for Morph[%d]\n", elementCount, i);
            return false;
        }

        // ★修正: %ls ではなく %s を使う（C++標準の wprintf 仕様に合わせるため）
        DebugPrintW(L"Morph[%d]: name='%s' type=%d elementCount=%d\n",
            i, morph.name.c_str(), (int)morph.morphType, elementCount);

        for (int32_t j = 0; j < elementCount; ++j)
        {
            switch (morph.morphType)
            {
            case PMXMorphType::Position:
            {
                auto& pm = morph.positionMorph.emplace_back();
                pm.vertexIndex = static_cast<unsigned int>(readIndex(ifs, header.global[2]));
                ifs.read(reinterpret_cast<char*>(&pm.position), sizeof(DirectX::XMFLOAT3));
                break;
            }
            case PMXMorphType::UV:
            case PMXMorphType::AddUV1:
            case PMXMorphType::AddUV2:
            case PMXMorphType::AddUV3:
            case PMXMorphType::AddUV4:
            {
                auto& uv = morph.uvMorph.emplace_back();
                uv.vertexIndex = static_cast<unsigned int>(readIndex(ifs, header.global[2]));
                ifs.read(reinterpret_cast<char*>(&uv.uv), sizeof(DirectX::XMFLOAT4));
                break;
            }
            case PMXMorphType::Bone:
            {
                auto& bm = morph.boneMorph.emplace_back();
                bm.boneIndex = static_cast<unsigned int>(readIndex(ifs, header.global[5]));
                ifs.read(reinterpret_cast<char*>(&bm.position), sizeof(DirectX::XMFLOAT3));
                ifs.read(reinterpret_cast<char*>(&bm.quaternion), sizeof(DirectX::XMFLOAT4));
                break;
            }
            case PMXMorphType::Material:
            {
                auto& mm = morph.materialMorph.emplace_back();
                mm.materialIndex = static_cast<unsigned int>(readIndex(ifs, header.global[4]));
                uint8_t op = 0;
                ifs.read(reinterpret_cast<char*>(&op), sizeof(uint8_t));
                mm.opType = static_cast<PMXMorph::MaterialMorph::OpType>(op);
                ifs.read(reinterpret_cast<char*>(&mm.diffuse), sizeof(DirectX::XMFLOAT4));
                ifs.read(reinterpret_cast<char*>(&mm.specular), sizeof(DirectX::XMFLOAT3));
                ifs.read(reinterpret_cast<char*>(&mm.specularPower), sizeof(float));
                ifs.read(reinterpret_cast<char*>(&mm.ambient), sizeof(DirectX::XMFLOAT3));
                ifs.read(reinterpret_cast<char*>(&mm.edgeColor), sizeof(DirectX::XMFLOAT4));
                ifs.read(reinterpret_cast<char*>(&mm.edgeSize), sizeof(float));
                ifs.read(reinterpret_cast<char*>(&mm.textureFactor), sizeof(DirectX::XMFLOAT4));
                ifs.read(reinterpret_cast<char*>(&mm.sphereTextureFactor), sizeof(DirectX::XMFLOAT4));
                ifs.read(reinterpret_cast<char*>(&mm.toonTextureFactor), sizeof(DirectX::XMFLOAT4));
                break;
            }
            case PMXMorphType::Group:
            {
                auto& gm = morph.groupMorph.emplace_back();
                gm.morphIndex = static_cast<unsigned int>(readIndex(ifs, header.global[6]));
                ifs.read(reinterpret_cast<char*>(&gm.weight), sizeof(float));
                break;
            }
            case PMXMorphType::Flip:
            {
                auto& fm = morph.flipMorph.emplace_back();
                fm.morphIndex = static_cast<unsigned int>(readIndex(ifs, header.global[6]));
                ifs.read(reinterpret_cast<char*>(&fm.weight), sizeof(float));
                break;
            }
            case PMXMorphType::Impluse:
            {
                auto& im = morph.impulseMorph.emplace_back();
                im.rigidBodyIndex = static_cast<unsigned int>(readIndex(ifs, header.global[7]));
                ifs.read(reinterpret_cast<char*>(&im.localFlag), sizeof(uint8_t));
                ifs.read(reinterpret_cast<char*>(&im.translateVelocity), sizeof(DirectX::XMFLOAT3));
                ifs.read(reinterpret_cast<char*>(&im.rotateTorque), sizeof(DirectX::XMFLOAT3));
                break;
            }
            default:
                DebugPrint("Unknown morph type: %d at Morph[%d]\n", (int)morph.morphType, i);
                return false;
            }

            if (!ifs.good()) {
                DebugPrint("Stream bad at Morph[%d] element[%d] type=%d\n", i, j, (int)morph.morphType);
                return false;
            }
        }

        DebugPrint("Morph[%d] counts - pos:%zu uv:%zu bone:%zu mat:%zu grp:%zu flip:%zu imp:%zu\n",
            i,
            morph.positionMorph.size(),
            morph.uvMorph.size(),
            morph.boneMorph.size(),
            morph.materialMorph.size(),
            morph.groupMorph.size(),
            morph.flipMorph.size(),
            morph.impulseMorph.size()
        );

        std::streampos morphEnd = ifs.tellg();
        outMorphs.push_back(std::move(morph));
        DebugPrint("==== Morph[%d] section end at %lld ====\n", i, (long long)morphEnd);
    }

    DebugPrint("Morph section end at pos: %lld\n", (long long)ifs.tellg());
    return true;
}

// 表示枠データの読み込み
bool PMXLoader::LoadDisplayFrames(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXDisplayFrame>& frames)
{
    int32_t frameCount = 0;
    ifs.read(reinterpret_cast<char*>(&frameCount), sizeof(int32_t));
    if (!ifs.good()) {
        std::cerr << "Failed to read DisplayFrame count\n";
        return false;
    }

    DebugPrint("FrameCount: %d\n", frameCount);
    frames.clear();
    frames.reserve(frameCount);

    for (int32_t i = 0; i < frameCount; ++i)
    {
        PMXDisplayFrame frame;

        // --- 名前 ---
        frame.name = Utf8ToWstring(readString(ifs, header.global[0]));
        // 英語名
        {
            std::streampos before = ifs.tellg();
            std::string english = readString(ifs, header.global[0]);
            if (english.empty()) {
                DebugPrint("Frame[%d]: English name empty at pos=%lld\n", i, (long long)before);
            }
            frame.englishName = english;
        }
        if (!ifs.good()) {
            DebugPrint("readString failed at DisplayFrame[%d]\n", i);
            return false;
        }

        // --- フラグ ---
        uint8_t flag = 0;
        ifs.read(reinterpret_cast<char*>(&flag), sizeof(uint8_t));
        frame.flag = static_cast<PMXDisplayFrame::FrameType>(flag);

        // --- ターゲット数 ---
        int32_t targetCount = 0;
        ifs.read(reinterpret_cast<char*>(&targetCount), sizeof(int32_t));
        if (!ifs.good()) {
            DebugPrint("Failed to read targetCount at DisplayFrame[%d]\n", i);
            return false;
        }

        frame.targets.reserve(targetCount);

        // --- 各ターゲット ---
        for (int32_t j = 0; j < targetCount; ++j)
        {
            PMXDisplayFrame::Target target;
            uint8_t targetType = 0;
            ifs.read(reinterpret_cast<char*>(&targetType), sizeof(uint8_t));
            target.type = static_cast<PMXDisplayFrame::TargetType>(targetType);

            int indexSize = 0;
            switch (target.type)
            {
            case PMXDisplayFrame::TargetType::BoneIndex:
                indexSize = header.global[5];
                break;
            case PMXDisplayFrame::TargetType::MorphIndex:
                indexSize = header.global[6];
                break;
            default:
                DebugPrint("Unknown targetType=%d at DisplayFrame[%d][%d]\n", targetType, i, j);
                return false;
            }

            target.index = static_cast<unsigned int>(readIndex(ifs, indexSize));
            frame.targets.push_back(target);
        }

        DebugPrintW(
            L"DisplayFrame[%d]: %s (targets=%d, flag=%d)\n",
            i,
            frame.name.c_str(),
            (int)frame.targets.size(),
            (int)frame.flag
        );

        frames.push_back(std::move(frame));
    }
    std::streampos curPos = ifs.tellg();
    DebugPrint("==== DisplayFrame section end at %lld ====\n", static_cast<long long>(curPos));
    return true;
}

// 剛体データの読み込み
bool PMXLoader::LoadRigidBodies(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXRigidBody>& rigidBodies)
{
    int32_t rigidBodyCount = 0;
    ifs.read(reinterpret_cast<char*>(&rigidBodyCount), sizeof(int32_t));
    if (!ifs.good()) {
        DebugPrint("Failed to read RigidBody count\n");
        return false;
    }

    DebugPrint("RigidBodyCount: %d\n", rigidBodyCount);
    rigidBodies.reserve(rigidBodyCount);

    for (int32_t i = 0; i < rigidBodyCount; ++i)
    {
        PMXRigidBody rb;
        std::streampos startPos = ifs.tellg();

        // 名前
        rb.name = Utf8ToWstring(readString(ifs, header.global[0]));
        rb.englishName = readString(ifs, header.global[0]);

        // ボーンインデックス
        rb.boneIndex = readIndex(ifs, header.global[5]); // boneIndexSize

        // 衝突グループ
        ifs.read(reinterpret_cast<char*>(&rb.group), sizeof(uint8_t));

        // 衝突グループマスク
        ifs.read(reinterpret_cast<char*>(&rb.collisionGroup), sizeof(uint16_t));

        // 形状
        uint8_t shapeType;
        ifs.read(reinterpret_cast<char*>(&shapeType), sizeof(uint8_t));
        rb.shape = static_cast<PMXRigidBody::Shape>(shapeType);

        // 形状サイズ（半径 or サイズ or 半径・高さ）
        ifs.read(reinterpret_cast<char*>(&rb.shapeSize), sizeof(DirectX::XMFLOAT3));

        // 位置・回転
        ifs.read(reinterpret_cast<char*>(&rb.translate), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&rb.rotate), sizeof(DirectX::XMFLOAT3));

        // 質量などの物理パラメータ
        ifs.read(reinterpret_cast<char*>(&rb.mass), sizeof(float));
        ifs.read(reinterpret_cast<char*>(&rb.translateDimmer), sizeof(float));
        ifs.read(reinterpret_cast<char*>(&rb.rotateDimmer), sizeof(float));
        ifs.read(reinterpret_cast<char*>(&rb.repulsion), sizeof(float));
        ifs.read(reinterpret_cast<char*>(&rb.friction), sizeof(float));

        // 操作モード（物理タイプ）
        uint8_t operation;
        ifs.read(reinterpret_cast<char*>(&operation), sizeof(uint8_t));
        rb.op = static_cast<PMXRigidBody::Operation>(operation);

        std::streampos endPos = ifs.tellg();

        // デバッグ出力 (ワイド文字版)
        DebugPrintW(
            L"RigidBody[%d]: name='%s' boneIndex=%u shape=%d mass=%.3f op=%d section(%lld→%lld)\n",
            i,
            rb.name.c_str(),
            rb.boneIndex,
            (int)rb.shape,
            rb.mass,
            (int)rb.op,
            (long long)startPos,
            (long long)endPos
        );

        if (!ifs.good()) {
            DebugPrint("Warning: stream error detected at rigid body %d\n", i);
            ifs.clear();
        }
        rigidBodies.push_back(std::move(rb));
    }

    DebugPrint("==== RigidBody section end at %lld ====\n", (long long)ifs.tellg());
    return true;
}

// ジョイントデータの読み込み
void PMXLoader::LoadJoints(std::ifstream& ifs, const PMXHeader& header, std::vector<PMXJoint>& joints)
{
    int jointCount = 0;
    ifs.read(reinterpret_cast<char*>(&jointCount), sizeof(int));
    DebugPrint("JointCount: %d\n", jointCount);

    joints.reserve(jointCount);

    for (int i = 0; i < jointCount; ++i)
    {
        PMXJoint joint;
        // 名前
        joint.name = Utf8ToWstring(readString(ifs, header.global[0]));
        joint.englishName = readString(ifs, header.global[0]);

        // タイプ
        uint8_t typeByte;
        ifs.read(reinterpret_cast<char*>(&typeByte), 1);
        joint.type = static_cast<PMXJoint::JointType>(typeByte);

        // 剛体インデックス
        joint.rigidBodyAIndex = readIndex(ifs, header.global[7]);
        joint.rigidBodyBIndex = readIndex(ifs, header.global[7]);

        // 位置・回転
        ifs.read(reinterpret_cast<char*>(&joint.translate), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&joint.rotate), sizeof(DirectX::XMFLOAT3));

        // 各制限
        ifs.read(reinterpret_cast<char*>(&joint.translateLowerLimit), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&joint.translateUpperLimit), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&joint.rotateLowerLimit), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&joint.rotateUpperLimit), sizeof(DirectX::XMFLOAT3));

        // スプリング係数
        ifs.read(reinterpret_cast<char*>(&joint.springTranslateFactor), sizeof(DirectX::XMFLOAT3));
        ifs.read(reinterpret_cast<char*>(&joint.springRotateFactor), sizeof(DirectX::XMFLOAT3));

        DebugPrintW(L"Joint[%d]: %s (type=%d, rigidA=%d, rigidB=%d)\n",
            i, joint.name.c_str(), (int)joint.type,
            joint.rigidBodyAIndex, joint.rigidBodyBIndex);
        joints.push_back(std::move(joint));
    }

    DebugPrint("==== Joint section end at %lld ====\n", (long long)ifs.tellg());
}



// === ここから追記 ===
#include <DirectXMath.h>
using namespace DirectX;


//============================================================
// ■ PostProcess：PMX全データ読み込み後に呼ぶ後処理
//============================================================
void PMXLoader::PostProcess(PMXModel& model)
{
    auto& bones = model.bones;

    //------------------------------------------------------------
    // 1) ボーンの ローカルオフセット(localOffset) を作成
    //------------------------------------------------------------
    for (size_t i = 0; i < bones.size(); i++) {
        int parent = bones[i].parentIndex;

        if (parent >= 0) {
            bones[i].localOffset.x = bones[i].position.x - bones[parent].position.x;
            bones[i].localOffset.y = bones[i].position.y - bones[parent].position.y;
            bones[i].localOffset.z = bones[i].position.z - bones[parent].position.z;
        }
        else {
            // ルートボーンはそのまま
            bones[i].localOffset.x= bones[i].position.x;
			bones[i].localOffset.y = bones[i].position.y;
			bones[i].localOffset.z = bones[i].position.z;
        }
    }

    //------------------------------------------------------------
    // 2) ボーン名 → インデックス MAP
    //------------------------------------------------------------
    BuildBoneNameMap(model);

    //------------------------------------------------------------
    // 3) ウェイトを正規化（合計1.0）
    //------------------------------------------------------------
    NormalizeAndClampVertexWeights(model);

    //------------------------------------------------------------
    // 4) 最終的に Recalc フラグ ON
    //------------------------------------------------------------
    model.needBoneRecalc = true;

    // ------------------------------------------------------------
    // ★修正：トポロジカルソート（親が必ず子より先に処理されることを保証）
    // ------------------------------------------------------------
    {
        const auto& bones = model.bones;
        int n = (int)bones.size();

        // 各ボーンのツリー深さを再帰で計算（循環参照ガード付き）
        std::vector<int> depth(n, -1);
        std::function<int(int)> computeDepth = [&](int i) -> int {
            if (depth[i] >= 0) return depth[i];
            depth[i] = 0; // 循環検出用の仮値
            if (bones[i].parentIndex >= 0 && bones[i].parentIndex < n)
                depth[i] = computeDepth(bones[i].parentIndex) + 1;
            return depth[i];
        };
        for (int i = 0; i < n; ++i) computeDepth(i);

        model.sortedBoneIndices.resize(n);
        for (int i = 0; i < n; ++i) model.sortedBoneIndices[i] = i;

        // 深さ昇順 → deformLayer昇順 → インデックス昇順
        std::sort(model.sortedBoneIndices.begin(), model.sortedBoneIndices.end(),
            [&](int a, int b) {
                if (depth[a] != depth[b]) return depth[a] < depth[b];
                if (bones[a].deformLayer != bones[b].deformLayer)
                    return bones[a].deformLayer < bones[b].deformLayer;
                return a < b;
            }
        );
    }

    //------------------------------------------------------------
    // 5) BindPose と InverseBindPose を作成（トポロジカルソート後に実行）
    //------------------------------------------------------------
    BuildBindPoseAndInverse(model);
}



void PMXLoader::BuildBoneNameMap(PMXModel& model)
{
    model.boneIndexByName.clear();

    // VMDLoader側と同じ正規化処理を簡易的にラムダで定義
    auto normalizeIK = [](const std::string& s) {
        std::string res = s;
        std::string zenkakuIK = u8"ＩＫ";
        size_t pos = 0;
        while ((pos = res.find(zenkakuIK, pos)) != std::string::npos) {
            res.replace(pos, zenkakuIK.length(), "IK");
            pos += 2;
        }
        return res;
        };

    for (size_t i = 0; i < model.bones.size(); ++i) {
        const auto& b = model.bones[i];

        if (!b.name.empty()) {
            // 正規化した名前で登録する
            std::string normName = normalizeIK(b.name);
            model.boneIndexByName[normName] = (int)i;
        }
        if (!b.enName.empty()) {
            model.boneIndexByName[b.enName] = (int)i;
        }
    }
    DebugPrint("[PMXLoader] BoneNameMap: %zu entries\n", model.boneIndexByName.size());
}

void PMXLoader::NormalizeAndClampVertexWeights(PMXModel& model)
{
    for (auto& v : model.vertices)
    {
        if (v.boneIndices.empty()) {
            v.boneIndices = { 0,0,0,0 };
            v.boneWeights = { 1,0,0,0 };
            continue;
        }
        if (v.boneIndices.size() > 4) v.boneIndices.resize(4);
        if (v.boneWeights.size() > 4) v.boneWeights.resize(4);

        float s = 0.f; for (auto w : v.boneWeights) s += w;
        if (s <= 1e-6f) {
            v.boneWeights.assign(v.boneIndices.size(), 0.0f);
            v.boneWeights[0] = 1.0f;
        }
        else {
            for (auto& w : v.boneWeights) w /= s;
        }
        while (v.boneIndices.size() < 4) v.boneIndices.push_back(0);
        while (v.boneWeights.size() < 4) v.boneWeights.push_back(0.0f);
    }
    DebugPrint("[PMXLoader] Weights normalized\n");
}

void PMXLoader::BuildBindPoseAndInverse(PMXModel& model)
{
    auto& bones = model.bones;
    size_t n = bones.size();

    if (n == 0) return;

    std::vector<XMMATRIX> globals(n);
    // 親ボーンが未処理の場合に備えてIdentityで初期化
    std::fill(globals.begin(), globals.end(), XMMatrixIdentity());

    // 1) BindPoseGlobal を作り直す（sortedBoneIndicesを使い親が必ず先に処理されることを保証）
    const auto& order = model.sortedBoneIndices;
    for (size_t k = 0; k < n; k++)
    {
        int i = order.empty() ? (int)k : order[k];

        XMMATRIX T = XMMatrixTranslation(
            bones[i].localOffset.x,
            bones[i].localOffset.y,
            bones[i].localOffset.z
        );

        // 回転は boneFlag で決まるので bind では基本 identity
        XMMATRIX R = XMMatrixIdentity();

        XMMATRIX local = R * T;

        if (bones[i].parentIndex >= 0)
            globals[i] = local * globals[bones[i].parentIndex];
        else
            globals[i] = local;

        XMStoreFloat4x4(&bones[i].bindPoseGlobal, globals[i]);
    }

    // 2) inverseBindPose = 逆行列
    for (size_t i = 0; i < n; i++) {
        XMMATRIX inv = XMMatrixInverse(nullptr, globals[i]);
        XMStoreFloat4x4(&bones[i].inverseBindPoseMatrix, inv);
    }
}


void PMXLoader::ConvertCoordinateIfNeeded(PMXModel& model, bool rhToLh)
{
    if (!rhToLh) return;

    // 頂点と法線のZ反転
    for (auto& v : model.vertices) {
        v.pos[2] = -v.pos[2];
        v.normal[2] = -v.normal[2];
    }
    // ボーンの絶対座標（position）のZ反転
    for (auto& b : model.bones) {
        b.position.z = -b.position.z;
    }
    // 面の巻き順反転
    for (size_t i = 0; i + 2 < model.indices.size(); i += 3)
        std::swap(model.indices[i + 1], model.indices[i + 2]);

    DebugPrint("[PMXLoader] RH->LH : Z flipped\n");
}
// === 追記ここまで ===

