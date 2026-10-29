# -*- coding: utf-8 -*-
"""FBX のアニメーション(Mixamo などのダンス・モーション)を MMD の VMD へ変換する。

使い方:
    python fbx2vmd.py dance.fbx
    python fbx2vmd.py dance.fbx -o out\\dance.vmd --take "mixamo.com"
    python fbx2vmd.py dance.fbx --list-takes

骨格は fbx2pmx と同じ手順で MMD 標準ボーン構造(足ＩＫ・準標準ボーン付き)に
整えてから変換するので、変換した PMX や一般的な MMD モデルにそのまま使える。
"""

from __future__ import annotations

import argparse
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import converter   # noqa: E402
import fbxscene    # noqa: E402
import motion      # noqa: E402
import tkconvert   # noqa: E402
import vmdfile     # noqa: E402


def _build_parser():
    parser = argparse.ArgumentParser(
        prog="fbx2vmd", description="FBX のアニメーションを MMD の VMD へ変換します。")
    parser.add_argument("inputs", nargs="+", metavar="FBX/TKA",
                        help="アニメーションを含む .fbx、またはゲームの .tka")
    parser.add_argument("--skeleton", metavar="TKS",
                        help=".tka の骨格(既定: ボーン数が合う .tks を近くのフォルダから探す)")
    parser.add_argument("-o", "--output", metavar="PATH", help="出力する .vmd、または出力フォルダ")
    parser.add_argument("--take", help="変換する Take 名(既定: 動きのある最初の Take)")
    parser.add_argument("--list-takes", action="store_true", help="Take の一覧を表示して終わる")
    parser.add_argument("--fps", type=int, default=motion.FPS, help="サンプリングのフレームレート (既定 30)")
    parser.add_argument("--no-reduce", action="store_true", help="毎フレームのキーを間引かない")
    group = parser.add_argument_group("骨格の整え方(モデル変換と同じ指定にする)")
    group.add_argument("--scale", type=float, help="座標に掛ける倍率")
    group.add_argument("--unit", choices=("mmd", "meter", "raw"), default="mmd")
    group.add_argument("--yaw", default="auto", help="Y 軸まわりの回転(度) / auto")
    group.add_argument("--arm-angle", default="auto", help="腕の角度 auto / off / 度")
    group.add_argument("--fit", choices=("auto", "none"), default="auto",
                       help="脚の長さを標準体格に合わせる (既定 auto)")
    group.add_argument("--leg-length", type=float, default=converter.STANDARD_LEG_LENGTH)
    group.add_argument("--bone-map", metavar="JSON", help="ボーン名の対応表")
    group.add_argument("--no-semi-standard", action="store_true", help="準標準ボーンを使わない")
    group.add_argument("--tk-up", choices=("z", "y"), default="z", help=".tka の上方向の軸")
    parser.add_argument("-q", "--quiet", action="store_true")
    return parser


def output_path_for(fbx_path, output, multiple):
    if output and not multiple and output.lower().endswith(".vmd"):
        directory = os.path.dirname(os.path.abspath(output))
        os.makedirs(directory, exist_ok=True)
        return output
    directory = output or os.path.dirname(os.path.abspath(fbx_path))
    os.makedirs(directory, exist_ok=True)
    return os.path.join(directory, os.path.splitext(os.path.basename(fbx_path))[0] + ".vmd")


def make_options(args):
    options = motion.MotionOptions()
    options.take = args.take
    options.fps = args.fps
    options.reduce = not args.no_reduce
    model = options.model
    model.scale = args.scale
    model.unit = args.unit
    model.yaw = args.yaw if args.yaw == "auto" else float(args.yaw)
    model.arm_angle = args.arm_angle if args.arm_angle in ("auto", "off") else float(args.arm_angle)
    model.fit = args.fit
    model.leg_length = args.leg_length
    model.bone_map_path = args.bone_map
    model.semi_standard = not args.no_semi_standard
    model.tk_up = args.tk_up
    return options


def report_motion(report, path, size, log):
    log("  出力: %s (%.1f KB)" % (path, size / 1024.0))
    log("  Take「%s」 %d フレーム / ボーン %d 本 / キー %d → %d / 表情 %d"
        % (report.take_name, report.frames, report.keyed_bones, report.keys_before,
           report.keys_after, report.morph_tracks))
    if report.skipped_names:
        log("  [警告] 名前が長すぎて VMD に入らないボーン: %s" % "、".join(report.skipped_names))
    for message in report.warnings:
        log("  [警告] %s" % message)


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = _build_parser().parse_args(argv)
    log = (lambda message: None) if args.quiet else print

    if args.list_takes:
        for fbx_path in args.inputs:
            scene = fbxscene.FbxScene(fbx_path)
            print(fbx_path)
            for take in scene.takes():
                print("  %-30s %.2f 秒 %s" % (take.name, take.duration_seconds,
                                             "" if take.has_motion() else "(動きなし)"))
        return 0

    options = make_options(args)
    failed = 0
    skeletons = [p for p in args.inputs if p.lower().endswith(".tks")]   # 一緒に渡された骨格
    inputs = [p for p in args.inputs if not p.lower().endswith(".tks")]
    for fbx_path in inputs:
        log("変換: %s" % fbx_path)
        path = output_path_for(fbx_path, args.output, len(inputs) > 1)
        try:
            if fbx_path.lower().endswith(".tka"):
                data, report, _result = tkconvert.convert_tka(fbx_path, options, log, args.skeleton,
                                                              skeletons)
            else:
                data, report, _result = motion.convert_motion(fbx_path, options)
            size = vmdfile.write_vmd(data, path)
        except Exception as error:
            failed += 1
            print("  失敗: %s" % error, file=sys.stderr)
            if os.environ.get("FBX2PMX_DEBUG"):
                traceback.print_exc()
            continue
        report_motion(report, path, size, log)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
