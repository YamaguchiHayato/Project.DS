# -*- coding: utf-8 -*-
"""FBX モデルを MMD の PMX 形式へ変換するコマンドラインツール。

使い方:
    python fbx2pmx.py model.fbx
    python fbx2pmx.py model.fbx -o out/model.pmx --copy-textures
    python fbx2pmx.py assets/modelData --scale 0.1

外部ライブラリ・FBX SDK・Blender のいずれも不要(Python 3.8 以降)。
"""

from __future__ import annotations

import argparse
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import converter   # noqa: E402
import pmxfile     # noqa: E402
import texmatch    # noqa: E402


def _build_parser():
    parser = argparse.ArgumentParser(
        prog="fbx2pmx",
        description="FBX モデルを MMD の PMX 形式へ変換します。",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("inputs", nargs="+", metavar="FBX/画像/フォルダ",
                        help="変換する .fbx。テクスチャ画像やフォルダを一緒に渡すと、"
                             "それも割り当ての候補にする(フォルダ内の .fbx はまとめて変換)")
    parser.add_argument("-o", "--output", metavar="PATH",
                        help="出力先の .pmx パス、または出力フォルダ")
    parser.add_argument("--scale", type=float,
                        help="頂点座標に掛ける倍率(省略時は FBX の単位から自動計算)")
    parser.add_argument("--unit", choices=("mmd", "meter", "raw"), default="mmd",
                        help="--scale 省略時の基準 (既定: mmd = 1 単位約 8cm)")
    parser.add_argument("--yaw", default="auto",
                        help="Y 軸まわりの回転角(度)。auto でつま先の向きから自動判定 (既定: auto)")
    parser.add_argument("--max-weights", type=int, default=4, choices=(1, 2, 4),
                        help="1 頂点あたりの最大ボーン数 (既定: 4)")
    parser.add_argument("--keep-bone-names", action="store_true",
                        help="ボーン名を MMD 標準名へ変換せず FBX のまま出力する")
    parser.add_argument("--bone-map", metavar="JSON",
                        help='{"FBXボーン名": "MMDボーン名"} 形式の JSON で名前を上書きする')
    parser.add_argument("--name", help="PMX のモデル名 (既定: ファイル名)")
    parser.add_argument("--comment", help="PMX のコメント欄に書く文字列")
    parser.add_argument("--copy-textures", action="store_true",
                        help="参照テクスチャを出力先へコピーする")
    parser.add_argument("--convert-textures", action="store_true",
                        help="(既定で有効) DDS など MMD が読めないテクスチャを PNG へ変換する")
    parser.add_argument("--no-convert-textures", action="store_true",
                        help="MMD が読めないテクスチャも変換せずそのまま参照する")
    parser.add_argument("--texture-dir", action="append", default=[], metavar="DIR",
                        help="テクスチャを探すフォルダを追加する(複数指定可)")
    parser.add_argument("--texture-map", metavar="JSON",
                        help='{"材質名": "画像"} 形式の JSON でテクスチャの割り当てを指定する')
    parser.add_argument("--texture-format", choices=("png", "tga"), default="png",
                        help="変換したテクスチャの出力形式 (既定: png)")
    parser.add_argument("--texture-max-size", type=int, default=0, metavar="N",
                        help="テクスチャの長辺が N ピクセルを超える場合に縮小する")
    parser.add_argument("--no-morph", action="store_true",
                        help="ブレンドシェイプをモーフへ変換しない")
    parser.add_argument("--no-edge", action="store_true",
                        help="マテリアルの輪郭線(エッジ)を無効にする")
    parser.add_argument("--double-sided", action="store_true",
                        help="マテリアルを両面描画にする")
    parser.add_argument("--flip-faces", action="store_true",
                        help="面の表裏が逆になったときに巻き順を反転する")
    parser.add_argument("--rig", choices=("mmd", "none"), default="mmd",
                        help="mmd: 配布モーションが流せるよう標準ボーン構造と足ＩＫを整える "
                             "(既定) / none: FBX の骨格のまま")
    parser.add_argument("--arm-angle", default="auto", metavar="{auto,off,角度}",
                        help="腕の下がり角(度)。auto は T スタンスのときだけ 35 度へ直す (既定: auto)")
    parser.add_argument("--keep-morph-names", action="store_true",
                        help="表情モーフ名を あ・い・う・まばたき 等へ変換しない")
    parser.add_argument("--morph-map", metavar="JSON",
                        help='{"FBXのモーフ名": "MMDのモーフ名"} 形式の JSON で名前を上書きする')
    parser.add_argument("--no-wink", action="store_true",
                        help="まばたき・笑いを左右に分けたウィンク系モーフを作らない")
    parser.add_argument("--physics", choices=("auto", "none"), default="auto",
                        help="auto: 髪・スカート・袖などに剛体とジョイントを自動で付ける (既定) "
                             "/ none: 付けない")
    parser.add_argument("--keep-material", action="store_true",
                        help="材質の色・トゥーン・輪郭線・両面描画を MMD 向けに調整しない")
    parser.add_argument("-q", "--quiet", action="store_true", help="経過表示を抑える")
    return parser


def _collect_inputs(paths):
    """指定パスを (FBX の一覧, テクスチャ画像の一覧, フォルダの一覧) に振り分ける。

    エクスプローラから FBX とテクスチャをまとめて bat へドロップした場合も、
    画像は変換対象ではなく割り当ての候補として扱う。
    """
    fbx_files = []
    images = []
    directories = []
    for path in paths:
        if os.path.isdir(path):
            directories.append(path)
            for name in sorted(os.listdir(path)):
                if name.lower().endswith(".fbx"):
                    fbx_files.append(os.path.join(path, name))
        elif os.path.splitext(path)[1].lower() in texmatch.IMAGE_EXTENSIONS:
            images.append(path)
        else:
            fbx_files.append(path)
    return fbx_files, images, directories


def _resolve_output(fbx_path, output, multiple):
    """出力先の .pmx パスを決める。"""
    if output and not multiple and not os.path.isdir(output) \
            and output.lower().endswith(".pmx"):
        directory = os.path.dirname(os.path.abspath(output))
        if directory and not os.path.isdir(directory):
            os.makedirs(directory, exist_ok=True)
        return output
    directory = output if output else os.path.dirname(os.path.abspath(fbx_path))
    if directory and not os.path.isdir(directory):
        os.makedirs(directory, exist_ok=True)
    name = os.path.splitext(os.path.basename(fbx_path))[0] + ".pmx"
    return os.path.join(directory, name)


def _make_options(args):
    options = converter.ConvertOptions()
    options.scale = args.scale
    options.unit = args.unit
    options.yaw = args.yaw if args.yaw == "auto" else float(args.yaw)
    options.max_weights = args.max_weights
    options.use_mmd_bone_names = not args.keep_bone_names
    options.bone_map_path = args.bone_map
    options.model_name = args.name
    options.comment = args.comment
    options.copy_textures = args.copy_textures
    options.convert_textures = not args.no_convert_textures
    options.texture_map_path = args.texture_map
    options.texture_format = args.texture_format
    options.texture_max_size = args.texture_max_size
    options.include_morphs = not args.no_morph
    options.edge = not args.no_edge
    options.double_sided = args.double_sided
    options.flip_faces = args.flip_faces
    options.rig = args.rig
    options.arm_angle = args.arm_angle if args.arm_angle in ("auto", "off") \
        else float(args.arm_angle)
    options.rename_morphs = not args.keep_morph_names
    options.morph_map_path = args.morph_map
    options.winks = not args.no_wink
    options.physics = args.physics
    options.mmd_look = not args.keep_material
    return options


def _report(result, output_path, size, log):
    model = result.model
    log("  出力: %s (%.1f KB)" % (output_path, size / 1024.0))
    log("  頂点 %d / 面 %d / 材質 %d / ボーン %d / モーフ %d / テクスチャ %d"
        % (len(model.vertices), len(model.faces), len(model.materials),
           len(model.bones), len(model.morphs), len(model.textures)))
    log("  スケール %.6g 倍 / Y 回転 %.0f 度 / メッシュ %d 個を統合"
        % (result.scale, result.yaw, result.source_meshes))
    if result.renamed_bones:
        log("  MMD 標準名へ変換したボーン: %d 本" % result.renamed_bones)
    if result.rig is not None:
        if result.rig.added_bones:
            log("  追加したボーン: %s" % "、".join(result.rig.added_bones))
        log("  IK: %d 本" % result.rig.ik_chains)
    if result.renamed_morphs:
        log("  MMD 標準名へ変換したモーフ: %d 個" % result.renamed_morphs)
    if result.winks:
        log("  ウィンク系モーフを追加: %d 個" % result.winks)
    report = result.physics
    if report is not None and report.swing_bodies:
        chains = "、".join("%s %d" % (name, count) for name, count in report.chains.items())
        log("  物理: 揺れもの剛体 %d / 当たり判定 %d / ジョイント %d (連鎖: %s)"
            % (report.swing_bodies, report.collision_bodies, report.joints, chains))
    _report_textures(result, output_path, log)
    for message in result.warnings:
        log("  [警告] %s" % message)


# 推定や未発見を含む割り当て(人の確認が必要なもの)
_UNSURE_METHODS = ("名前から推定", "唯一の候補", "見つからない")


def _report_textures(result, output_path, log):
    """材質ごとのテクスチャ割り当てを表示し、要確認なら割り当て表を書き出す。"""
    assignments = result.texture_assignments
    if not assignments:
        return
    converted = dict(result.converted_textures)
    width = min(24, max(len(name) for name, _entry, _method, _source in assignments))
    log("  テクスチャの割り当て:")
    seen = set()
    for name, entry, method, source in assignments:
        if name in seen:
            continue
        seen.add(name)
        if entry is None:
            log("    %-*s  (%s)" % (width, name, method))
            continue
        detail = method
        if source is not None and os.path.basename(source) != os.path.basename(entry):
            detail += ": %s" % os.path.basename(source)
            if os.path.basename(source) in converted:
                detail += " を変換"
        log("    %-*s  ← %s  [%s]" % (width, name, entry, detail))

    if any(method.startswith(_UNSURE_METHODS)
           for _name, _entry, method, _source in assignments):
        map_path = os.path.splitext(output_path)[0] + "_textures.json"
        unique = []
        names = set()
        for name, _entry, _method, source in assignments:
            if name not in names:
                names.add(name)
                unique.append((name, source))
        texmatch.save_texture_map(map_path, unique)
        log("  推定や未発見の割り当てがあります。違っていたら %s を書き換えて"
            % os.path.basename(map_path))
        log("  --texture-map %s を付けて変換し直してください。" % os.path.basename(map_path))


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):  # コンソールが出せない文字で落ちないように
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = _build_parser().parse_args(argv)
    log = (lambda message: None) if args.quiet else print

    try:
        if args.yaw != "auto":
            float(args.yaw)
    except ValueError:
        print("--yaw には auto か角度(度)を指定してください: %s" % args.yaw, file=sys.stderr)
        return 2
    try:
        if args.arm_angle not in ("auto", "off"):
            float(args.arm_angle)
    except ValueError:
        print("--arm-angle には auto / off / 角度(度)を指定してください: %s" % args.arm_angle,
              file=sys.stderr)
        return 2

    files, images, directories = _collect_inputs(args.inputs)
    if not files:
        print("変換対象の FBX が見つかりません。", file=sys.stderr)
        return 2

    options = _make_options(args)
    options.texture_files = images
    options.texture_dirs = directories + list(args.texture_dir)
    if images or options.texture_dirs:
        log("テクスチャの候補: 画像 %d 枚 / フォルダ %d 個" % (len(images), len(options.texture_dirs)))
    failed = 0
    for fbx_path in files:
        if not os.path.isfile(fbx_path):
            print("ファイルがありません: %s" % fbx_path, file=sys.stderr)
            failed += 1
            continue
        log("変換: %s" % fbx_path)
        output_path = _resolve_output(fbx_path, args.output, len(files) > 1)
        try:
            result = converter.convert(fbx_path, output_path, options, log)
            size = pmxfile.write_pmx(result.model, output_path)
        except Exception as error:  # 1 件失敗しても残りは続ける
            failed += 1
            print("  失敗: %s" % error, file=sys.stderr)
            if os.environ.get("FBX2PMX_DEBUG"):
                traceback.print_exc()
            continue
        _report(result, output_path, size, log)

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
