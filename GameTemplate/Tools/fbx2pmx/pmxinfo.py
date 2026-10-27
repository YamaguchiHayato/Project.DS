# -*- coding: utf-8 -*-
"""PMX ファイルの中身を表示して整合性を確認するツール。

使い方:
    python pmxinfo.py model.pmx
    python pmxinfo.py model.pmx --materials --bones
"""

from __future__ import annotations

import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pmxfile  # noqa: E402


def _bounding_box(vertices):
    if not vertices:
        return None
    low = [min(v.position[axis] for v in vertices) for axis in range(3)]
    high = [max(v.position[axis] for v in vertices) for axis in range(3)]
    return low, high


def _check(model):
    """よくある壊れ方を調べて警告の一覧を返す。"""
    warnings = []
    vertex_count = len(model.vertices)
    bone_count = len(model.bones)

    for face in model.faces:
        if any(not 0 <= index < vertex_count for index in face):
            warnings.append("面が範囲外の頂点を参照しています")
            break

    total = sum(material.surface_count for material in model.materials)
    if total != len(model.faces) * 3:
        warnings.append("マテリアルの面数合計(%d)が総インデックス数(%d)と違います"
                        % (total, len(model.faces) * 3))

    for index, vertex in enumerate(model.vertices):
        if any(not 0 <= bone < bone_count for bone in vertex.bones):
            warnings.append("頂点 %d が範囲外のボーンを参照しています" % index)
            break
        if abs(sum(vertex.weights[:len(vertex.bones)]) - 1.0) > 1e-3:
            warnings.append("頂点 %d のウェイト合計が 1 ではありません" % index)
            break

    for index, bone in enumerate(model.bones):
        if bone.parent_index >= bone_count or bone.parent_index == index:
            warnings.append("ボーン %d(%s) の親番号が不正です" % (index, bone.name))
            break

    for material in model.materials:
        if material.texture_index >= len(model.textures):
            warnings.append("マテリアル %s のテクスチャ番号が範囲外です" % material.name)
            break

    if not any(frame.special for frame in model.display_frames):
        warnings.append("特殊表示枠(Root / 表情)がありません")

    degenerate = sum(1 for face in model.faces
                     if face[0] == face[1] or face[1] == face[2] or face[0] == face[2])
    if degenerate:
        warnings.append("面が潰れている三角形が %d 枚あります" % degenerate)
    return warnings


def _print_summary(model, path):
    print("ファイル: %s (%.1f KB)" % (path, os.path.getsize(path) / 1024.0))
    print("モデル名: %s / %s" % (model.name, model.name_en))
    print("頂点 %d / 面 %d / 材質 %d / ボーン %d / モーフ %d / 表示枠 %d / テクスチャ %d"
          % (len(model.vertices), len(model.faces), len(model.materials),
             len(model.bones), len(model.morphs), len(model.display_frames),
             len(model.textures)))
    box = _bounding_box(model.vertices)
    if box:
        low, high = box
        print("サイズ: X %.2f〜%.2f / Y %.2f〜%.2f / Z %.2f〜%.2f (高さ %.2f)"
              % (low[0], high[0], low[1], high[1], low[2], high[2], high[1] - low[1]))


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="pmxinfo", description="PMX ファイルの内容を表示します。")
    parser.add_argument("path", metavar="PMX")
    parser.add_argument("--materials", action="store_true", help="マテリアル一覧を表示")
    parser.add_argument("--bones", action="store_true", help="ボーン一覧を表示")
    parser.add_argument("--morphs", action="store_true", help="モーフ一覧を表示")
    parser.add_argument("--frames", action="store_true", help="表示枠を表示")
    args = parser.parse_args(argv)

    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass

    model = pmxfile.read_pmx(args.path)
    _print_summary(model, args.path)

    if args.materials:
        print("\n[マテリアル]")
        for index, material in enumerate(model.materials):
            texture = model.textures[material.texture_index] \
                if 0 <= material.texture_index < len(model.textures) else "(なし)"
            print("  %3d %-24s 面 %6d  拡散 %.2f,%.2f,%.2f,%.2f  tex %s"
                  % (index, material.name, material.surface_count // 3,
                     material.diffuse[0], material.diffuse[1], material.diffuse[2],
                     material.diffuse[3], texture))

    if args.bones:
        print("\n[ボーン]")
        for index, bone in enumerate(model.bones):
            parent = model.bones[bone.parent_index].name \
                if 0 <= bone.parent_index < len(model.bones) else "-"
            print("  %3d %-16s 親 %-16s 位置 %7.2f %7.2f %7.2f  英 %s"
                  % (index, bone.name, parent, bone.position[0], bone.position[1],
                     bone.position[2], bone.name_en))

    if args.morphs:
        print("\n[モーフ]")
        for index, morph in enumerate(model.morphs):
            print("  %3d %-24s 種別 %d  パネル %d  オフセット %d"
                  % (index, morph.name, morph.morph_type, morph.panel,
                     len(morph.offsets)))

    if args.frames:
        print("\n[表示枠]")
        for frame in model.display_frames:
            print("  %-12s 特殊 %s  要素 %d"
                  % (frame.name, "はい" if frame.special else "いいえ",
                     len(frame.elements)))

    warnings = _check(model)
    if warnings:
        print("\n[警告]")
        for message in warnings:
            print("  " + message)
        return 1
    print("\n整合性チェック: 問題は見つかりませんでした。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
