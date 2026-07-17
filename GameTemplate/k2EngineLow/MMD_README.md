# k2EngineLow MMD描画システム 引き継ぎガイド

PMX（MMDモデル）＋VMD（モーション）をDirectX 12で描画するためのモジュール群の解説です。
**次の作品でMMDキャラを出したい人は、このファイルとサンプルコードだけ読めば使えます。**

---

## 1. クラス構成（登場人物）

```
[ファイル]           [クラス]        [役割]
pmxLoader.h/.cpp     PMXLoader      .pmxファイルを読み込んで PMXModel を作る（純粋なファイルI/O）
                     PMXModel       モデルデータ本体（頂点・ボーン・材質・モーフ・剛体）
VMDLoader.h/.cpp     VMDLoader      .vmdファイルを読み込む（ボーン/モーフ/カメラ/IKキーフレーム）
VMDAnimPlayer.h/.cpp VMDAnimPlayer  VMDを時間で評価してボーン姿勢・モーフ値を作る（PMXRenderが内部で持つ）
PMXPhysics.h/.cpp    PMXPhysics     スカート・髪などの物理（Bullet。モデルごとに独立したワールドを持つ）
PMXRender.h/.cpp     PMXRender      ★中心クラス。上記を束ねてスキニング・描画まで行う
RenderingEngine      AddPMXObject   登録されたPMXRenderを毎フレーム更新・描画する
```

データの流れ（毎フレーム）:

```
VMDAnimPlayer(時間→姿勢) → ボーンFK → IK → 付与親 → 物理(Bullet) → スキニング行列
                         → モーフ値 → 頂点モーフ/マテリアルモーフ
→ GPUへ転送（ボーン行列StructuredBuffer・頂点バッファ・定数バッファ）→ 描画
```

---

## 2. 最小の使い方（これだけで表示できる）

```cpp
// --- メンバとして持つもの ---
PMXLoader  m_loader;
PMXModel   m_model;
PMXRender  m_render;
VMDLoader  m_vmd;

// --- 初期化 ---
m_loader.LoadFromFile("Assets/MMD/Miku/Miku.pmx", m_model);   // 1) PMX読み込み
auto& rt = g_renderingEngine->GetMainRenderTarget();
m_render.Init(m_model, rt.GetColorBufferFormat(), rt.GetDepthBufferFormat()); // 2) レンダー初期化
while (m_render.UpdateLoader()) {}                             // 3) テクスチャを読み切る
m_vmd.Load("Assets/animData/dance.vmd");                       // 4) VMD読み込み
m_render.AttachVMD(&m_vmd);                                    // 5) モーション接続
m_render.SetRootTransform({0,0,0}, {0,0,0,1});                 // 6) 位置と向き
g_renderingEngine->AddPMXObject(&m_render);                    // 7) 描画登録 ←これで映る

// --- 毎フレーム（音楽に合わせる場合） ---
m_render.SyncToMusicTime(musicSec);  // VMDは30fps基準。秒で渡せばOK

// --- 画面から消すとき ---
g_renderingEngine->RemovePMXObject(&m_render); // 登録解除（コストも消える）
```

よく使うAPI（PMXRender）:

| 関数 | 用途 |
|---|---|
| `SyncToMusicTime(sec)` | 曲の再生位置に同期（dt積算より正確。ズレない） |
| `SetAnimPlaySpeed(s)` | 一時停止(0)や再開(1) |
| `ResetPhysics()` | ポーズを飛ばした直後に呼ぶ（スカート暴れ防止） |
| `SetPhysicsEnabled(b)` | 物理のON/OFF（遠くのキャラはOFFで軽量化） |
| `SetEnableCameraVMD(b)` | カメラVMDでg_camera3Dを動かすか |
| `SetRimStrength(f)` | リムライト強度（キャラ1.0 / ステージ・地面0） |
| `SetFilter(brightness, blur)` | 明るさフィルター（AC演出の暗転などに使用） |
| `SetMotionBlurEnable(b)` | モーションブラーの対象にするか |

---

## 3. パフォーマンス設計（モデル数が増えても大丈夫な理由）

### 3-1. CPU計算とGPU転送の分離＋並列化
- `UpdateAnimation()` … VMD・ボーン・IK・物理・モーフの**CPU計算だけ**を行う。GPUに触らない
- `Draw()` … CPUが用意した結果を**GPUへ転送して描画コマンド発行**（メインスレッド専用）
- RenderingEngineは全PMXオブジェクトの `UpdateAnimation()` を
  `std::for_each(std::execution::par, ...)` で**並列実行**してから、順番にDrawする
- → 一番重い「ボーン更新＋物理」がコア数でスケールする。**モデルを増やしてもFPSが落ちにくい**

```
[並列]  モデルA: VMD→ボーン→物理→モーフ   ┐
        モデルB: VMD→ボーン→物理→モーフ   ├ ワーカースレッドに分散
        モデルC: ...                       ┘
[直列]  A転送→A描画 → B転送→B描画 → ...     （GPUはメインスレッドのみ）
```

**並列実行の前提（重要）**: `PMXModel` と `VMDLoader` は**PMXRender間で共有しない**こと。
UpdateAnimationはモデルデータ（ボーン姿勢・モーフ値）に書き込むため、共有すると競合します。
「1キャラ＝PMXModel1個＋VMDLoader1個＋PMXRender1個」のセットで持つのがルール。

### 3-2. その他の軽量化（実装済み・自動で効く）
- **PSO/RootSignature/シェーダー**: 全インスタンスでstatic共有（1回だけ生成）
- **テクスチャ**: パスで共有キャッシュ（同モデル9体でも読み込みは1回）
- **頂点モーフ**: 値が変わったフレームだけ再計算。しかも**影響する頂点（顔まわり）だけ**を
  復元・再計算し、GPU転送も影響範囲のみ（全3万頂点は触らない）
- **静的モデル**: VMDも物理剛体も無いモデル（地面など）は、初回転送後は更新を丸ごとスキップ
- **スクラッチバッファ**: 毎フレームのヒープ確保ゼロ（ボーン行列・モーフ・材質の作業領域を使い回し）
- **物理**: 固定1/60ステップ（フレーム落ち時にサブステップが増えて余計重くなるのを防止）、
  リセット時のウォームアップは1フレーム10ステップに分散
- **IK**: 反復回数を8回に制限（PMX指定が大きくても打ち切り）

### 3-3. さらに軽くしたいときのノブ
- 後列キャラに `SetPhysicsEnabled(false)`（見た目の影響はスカートの揺れのみ）
- 画面に映らない期間は `RemovePMXObject()`（登録している限り毎フレームコストを払う）
- モデル自体の頂点数・材質数・剛体数を減らす（材質数＝ドローコール数）

---

## 4. ロードの作法（スレッドとGPUの境界）

ロードは「**ファイルI/Oは裏スレッドOK・GPU操作はメインスレッドのみ**」が絶対ルール。

| 処理 | 裏スレッド |
|---|---|
| `PMXLoader::LoadFromFile` / `VMDLoader::Load` | ✅ OK（純粋なファイルI/O） |
| `PMXRender::PreloadTextureBlobs` | ✅ OK（テクスチャの先読み。内部でロックする） |
| `PMXRender::Init` / `UpdateLoader` / `Destroy` | ❌ メインスレッドのみ（GPUリソース生成） |
| `NewGO` / `DeleteGO` / サウンド登録 | ❌ メインスレッドのみ |

実戦パターン（RhythmGame.cpp が実例）:
1. 裏スレッド: 全モデルのPMX/VMDを`LoadFromFile`（モデルごとにさらに並列化も可）
   ＋ `PMXRender::PreloadTextureBlobs(model)` でテクスチャ(.dds)もメモリへ先読み
2. 完了フラグ（`std::atomic<bool>`）を立てる
3. メインスレッド: フラグを見てから `Init` → 毎フレーム `UpdateLoader()` を
   **時間予算つき**で回す（1フレーム約6msまで。RhythmGame::PumpModelLoaders参照）。
   先読み済みテクスチャは「メモリ→GPU転送」だけになるのでロード画面が止まらない
4. `AddPMXObject` して表示開始。全部終わったら `ClearTextureBlobCache()` で後始末

モデル差し替え（曲変更など）は「`Destroy()` → 裏で新PMXを読み → `Init()`」。
`Destroy()` はボーンバッファ・物理・キャッシュ類をすべてリセットするので再Initは安全。

---

## 5. ハマりどころ集（先輩たちの屍）

1. **裏スレッドでVMDLoaderを読み直すときは、先に `AttachVMD(nullptr)` で参照を切る**
   描画中のレンダーが旧VMDのモーフトラックを参照したままロードし直すと use-after-free で稀に落ちる。
2. **ポーズを一気に飛ばしたら `ResetPhysics()`**
   リトライ・リザルトポーズ切替などでボーンが瞬間移動すると、物理剛体が引きずられて
   スカート・髪が爆発する。リセット→ウォームアップで落ち着かせる。
3. **`SetRootTransform` はレンダー再Init後に必ず再適用**（Initで消える）
4. **同じPMXModelを2つのPMXRenderで共有しない**（3-1参照。姿勢書き込みが競合する）
5. **カメラVMDは1体だけに有効化する**（全員に有効化するとカメラを取り合う）
6. **リソースの二重解放に注意**
   Texture等のラッパーは遅延解放キュー（`ReleaseD3D12Object`）へ渡した時点でポインタを
   nullptr化する実装になっている。新しいリソースクラスを作るときも必ずこのパターンに従うこと
   （守らないと再Init時に二重解放→アクセス違反）。
7. **VMDは30fps基準**。`SyncToMusicTime(秒)` を使えば意識しなくてよい。
   dt積算で自前更新するとフレーム落ちで音とズレるので、音楽モノは必ず時刻同期を使う。

---

## 6. ファイル対応表（改造したくなったら）

| やりたいこと | 見るファイル |
|---|---|
| 描画の見た目（ライティング・リム・ブラー） | PMXRender.cpp の Draw / Assets/shader/pmxModel.fx |
| ボーン・IKの挙動 | PMXRender.cpp の BuildGlobals / SolveIK / ApplyGrant |
| 物理の挙動（重力・減衰） | PMXPhysics.cpp |
| モーフ（表情・材質色） | PMXRender.cpp の UpdateVertices / UpdateMaterialMorphs |
| VMDの補間・カメラ | VMDAnimPlayer.cpp / VMDLoader.cpp |
| 対応していないPMX機能を足す | pmxLoader.cpp（読み込み）＋PMXRender.cpp（利用側） |
