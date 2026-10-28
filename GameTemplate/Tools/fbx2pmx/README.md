# fbx2pmx — FBX モデルを MMD の PMX 形式へ変換するツール

FBX ファイルを MikuMikuDance (MMD) / PMXEditor で読める **PMX 2.0** へ変換します。
Python だけで動き、Autodesk FBX SDK も Blender も不要です。

- 対応入力: バイナリ FBX (7.x) / ASCII FBX
- 対応出力: PMX 2.0 (文字コード UTF-16LE)
- 必要環境: Python 3.8 以降 (追加ライブラリ不要)
- MMD が読めない DDS テクスチャは PNG / TGA へ変換できます(後述)

## 使い方

```bat
python fbx2pmx.py model.fbx
```

`model.fbx` と同じ場所に `model.pmx` ができます。
Windows では `fbx2pmx.bat` に **FBX をドラッグ＆ドロップ**しても変換できます。

```bat
rem 出力先を指定
python fbx2pmx.py model.fbx -o out\model.pmx

rem テクスチャも出力先へコピーする
python fbx2pmx.py model.fbx -o out --copy-textures

rem フォルダ内の .fbx をまとめて変換
python fbx2pmx.py ..\..\Game\Assets\modelData -o out
```

### 主なオプション

| オプション | 説明 |
| --- | --- |
| `-o, --output PATH` | 出力する `.pmx` のパス、または出力フォルダ |
| `--scale N` | 頂点座標に掛ける倍率。省略時は FBX の単位から自動計算 |
| `--unit {mmd,meter,raw}` | `--scale` 省略時の基準。既定は `mmd` (1 単位 ≒ 8cm) |
| `--yaw {auto,角度}` | Y 軸まわりの回転。既定の `auto` はつま先ボーンから正面を推定 |
| `--max-weights {1,2,4}` | 1 頂点あたりの最大ボーン数 (既定 4) |
| `--keep-bone-names` | ボーン名を MMD 標準名へ変換せず FBX のまま出力 |
| `--bone-map JSON` | `{"FBXボーン名": "MMDボーン名"}` の JSON で名前を上書き |
| `--name` / `--comment` | PMX のモデル名・コメント |
| `--copy-textures` | 参照テクスチャを出力先へそのままコピー |
| `--convert-textures` | DDS など MMD が読めないテクスチャを PNG へ変換して出力先へ置く |
| `--texture-format {png,tga}` | `--convert-textures` の出力形式 (既定 `png`) |
| `--texture-max-size N` | テクスチャの長辺が N を超える場合に縮小 |
| `--no-morph` | ブレンドシェイプをモーフへ変換しない |
| `--no-edge` / `--double-sided` | マテリアルの輪郭線を切る / 両面描画にする |
| `--flip-faces` | 面の表裏が逆になったときに巻き順を反転 |

変換後の中身を確認するツールも付いています。

```bat
python pmxinfo.py model.pmx --materials --bones --morphs
```

## 変換される内容

| FBX | PMX | 備考 |
| --- | --- | --- |
| メッシュ(複数可) | 1 つのモデルへ統合 | 多角形は三角形に分割 |
| 法線 / UV | 頂点法線 / UV | UV は上下反転(PMX は原点左上) |
| マテリアル | 材質 | 拡散色・不透明度・反射色・反射強度・環境色 |
| テクスチャ | テクスチャ(拡散のみ) | スフィア・トゥーンは未設定 |
| スキン(Cluster) | BDEF1 / BDEF2 / BDEF4 | 影響度上位 N 本を正規化 |
| ボーン階層 | ボーン | バインドポーズ(TransformLink)基準 |
| ブレンドシェイプ | 頂点モーフ | 名前から表示パネルを推定 |
| — | 表示枠 (Root / 表情 / ボーン) | MMD 用に自動生成 |

### 座標系・単位

FBX は右手座標系で、上方向や正面方向はファイル内の `GlobalSettings`
(`UpAxis` / `FrontAxis` / `CoordAxis`) で決まります。本ツールはその指定を読んで
**PMX の左手座標系 (Y 上・正面 -Z)** へ変換し、鏡像変換になる分だけ面の巻き順を
戻します。単位は `UnitScaleFactor` から実寸(メートル)を求め、MMD の縮尺
(1 単位 ≒ 8cm、身長 158cm で約 20 単位) に合わせます。

### ボーン名の MMD 標準名への変換

Mixamo (`mixamorig:Hips`)、Autodesk の `Character1_*`、3ds Max Biped (`Bip001 *`)、
Unity Humanoid 風 (`LeftUpperArm`、`UpperArm.L` など) の名前を
`センター` `上半身` `首` `頭` `左腕` `左ひじ` `左手首` `左足` `左ひざ` `左足首`
`左親指１` … といった MMD 標準名へ読み替えます(元の名前は英語名欄に残します)。
表にない名前はそのまま出力されます。`--bone-map` で個別に上書きでき、
`--keep-bone-names` で変換自体を止められます。

## テクスチャの変換 (DDS → PNG / TGA)

MMD が読めるのは BMP / PNG / JPEG / TGA だけで、**DDS は読めません**。
ゲーム用アセットが DDS の場合は `--convert-textures` を付けてください。
PMX と同じフォルダへ PNG を書き出し、PMX からはその PNG を参照します。

```bat
python fbx2pmx.py model.fbx -o out --convert-textures

rem 4K テクスチャを 2048 まで落としつつ変換
python fbx2pmx.py model.fbx -o out --convert-textures --texture-max-size 2048
```

テクスチャだけを変換したいときは単体でも使えます。

```bat
python texconv.py utc_all2.DDS                       rem 同じ場所に .png を作る
python texconv.py ..\..\Game\Assets\modelData -o tex rem フォルダ内の DDS をまとめて
python texconv.py normal.DDS --format tga --max-size 1024
```

対応している DDS は次のとおりです。

| 形式 | 内蔵デコーダ | Pillow |
| --- | --- | --- |
| DXT1 / DXT3 / DXT5 (BC1/BC2/BC3) | ○ | ○ |
| BC4 / BC5 (ATI1 / ATI2) | ○ | ○ |
| 無圧縮 RGB / RGBA / BGRA (マスク指定・DX10 含む) | ○ | 一部不可 |
| BC7 / BC6H | × | ○ |

Pillow が入っていればまず Pillow で読み、読めない形式(`B8G8R8A8_UNORM_SRGB` など
Pillow が未対応の DX10 形式)は内蔵デコーダで読み直します。Pillow が無くても
BC7 以外は変換できます。BC7 のみ `pip install Pillow` が必要です。

## 制限事項

- アニメーション(Take / AnimationCurve)は変換しません。モデルのみです。
- 剛体・ジョイント・物理、IK ボーン(`足ＩＫ` など)、SDEF は生成しません。
  必要なら PMXEditor で追加してください。
- スフィアマップ・トゥーンテクスチャ、追加 UV、頂点カラーは扱いません。
- マテリアルは拡散テクスチャ 1 枚のみを参照します(法線・スペキュラは使いません)。
- BC7 / BC6H の DDS を変換するには Pillow が必要です。

## うまくいかないとき

| 症状 | 対処 |
| --- | --- |
| MMD で後ろを向いている | `--yaw 180` |
| 面が裏返って見える | `--flip-faces` |
| 大きすぎる / 小さすぎる | `--scale 0.1` のように倍率を直接指定 |
| テクスチャが出ない | `--convert-textures` を付ける(DDS を PNG へ変換して同梱) |
| ボーン名を変えたくない | `--keep-bone-names` |

## ファイル構成

| ファイル | 役割 |
| --- | --- |
| `fbx2pmx.py` | コマンドライン入口 |
| `fbx2pmx.bat` | ドラッグ＆ドロップ用のバッチ |
| `fbxfile.py` | FBX(バイナリ / ASCII)をノードツリーとして読み込む |
| `fbxscene.py` | ノードツリーからメッシュ・スキン・マテリアルを取り出す |
| `converter.py` | 座標系・単位・ウェイト・モーフを PMX 向けに変換 |
| `pmxfile.py` | PMX 2.0 の読み書き |
| `bonenames.py` | MMD 標準ボーン名への変換表 |
| `texconv.py` | DDS → PNG / TGA 変換(単体ツールとしても使える) |
| `pmxinfo.py` | PMX の内容表示と整合性チェック |
| `selftest.py` | テスト用の立方体 FBX を生成して往復検証 |
| `mat4.py` | 4x4 行列ユーティリティ |

## 動作確認

```bat
python selftest.py
```

テスト用の立方体 FBX(バイナリ / ASCII)と DDS をその場で作って変換し、頂点数・
面の向き・座標・材質・ウェイトと、DDS デコード・PNG 書き出しを検証します。
