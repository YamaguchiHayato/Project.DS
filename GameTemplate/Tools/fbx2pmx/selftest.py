# -*- coding: utf-8 -*-
"""fbx2pmx のセルフテスト。

テスト用の立方体 FBX (バイナリ / ASCII) をその場で生成し、PMX へ変換して
結果を検証する。外部のモデルファイルは不要。

    python selftest.py
"""

from __future__ import annotations

import os
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import converter   # noqa: E402
import pmxfile     # noqa: E402

# 立方体の制御点と面(右手系 Y-up、表から見て反時計回り)
_CUBE_POINTS = [
    (-1, -1, -1), (1, -1, -1), (1, -1, 1), (-1, -1, 1),
    (-1, 1, -1), (1, 1, -1), (1, 1, 1), (-1, 1, 1),
]
_CUBE_FACES = [
    ((0, 1, 2, 3), (0, -1, 0)),
    ((4, 7, 6, 5), (0, 1, 0)),
    ((3, 2, 6, 7), (0, 0, 1)),
    ((0, 4, 5, 1), (0, 0, -1)),
    ((0, 3, 7, 4), (-1, 0, 0)),
    ((1, 5, 6, 2), (1, 0, 0)),
]
_CUBE_UVS = [0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0]


def _cube_arrays():
    """PolygonVertexIndex / Normals / UVIndex を作る。"""
    polygon_index = []
    normals = []
    uv_index = []
    for corners, normal in _CUBE_FACES:
        for position, point in enumerate(corners):
            polygon_index.append(point if position < 3 else -point - 1)
            normals.extend(normal)
            uv_index.append(position)
    return polygon_index, normals, uv_index


# --------------------------------------------------------------------------
# テスト用 FBX の生成
# --------------------------------------------------------------------------

class _Node:
    def __init__(self, name, props=(), children=()):
        self.name = name
        self.props = list(props)
        self.children = list(children)


def _cube_nodes():
    """立方体シーンのノードツリー(FBX の構造そのまま)を作る。"""
    polygon_index, normals, uv_index = _cube_arrays()
    points = [float(c) for point in _CUBE_POINTS for c in point]

    def prop70(name, type_name, *values):
        return _Node("P", [("S", name), ("S", type_name), ("S", ""), ("S", "A")]
                     + [("D", float(v)) if isinstance(v, float) else ("I", int(v))
                        for v in values])

    settings = _Node("GlobalSettings", [], [
        _Node("Version", [("I", 1000)]),
        _Node("Properties70", [], [
            prop70("UpAxis", "int", 1), prop70("UpAxisSign", "int", 1),
            prop70("FrontAxis", "int", 2), prop70("FrontAxisSign", "int", 1),
            prop70("CoordAxis", "int", 0), prop70("CoordAxisSign", "int", 1),
            prop70("UnitScaleFactor", "double", 100.0),
        ]),
    ])

    geometry = _Node("Geometry", [("L", 100), ("S", "Geometry::Cube"), ("S", "Mesh")], [
        _Node("Vertices", [("d", points)]),
        _Node("PolygonVertexIndex", [("i", polygon_index)]),
        _Node("LayerElementNormal", [("I", 0)], [
            _Node("Version", [("I", 102)]),
            _Node("Name", [("S", "")]),
            _Node("MappingInformationType", [("S", "ByPolygonVertex")]),
            _Node("ReferenceInformationType", [("S", "Direct")]),
            _Node("Normals", [("d", [float(v) for v in normals])]),
        ]),
        _Node("LayerElementUV", [("I", 0)], [
            _Node("Version", [("I", 101)]),
            _Node("Name", [("S", "map1")]),
            _Node("MappingInformationType", [("S", "ByPolygonVertex")]),
            _Node("ReferenceInformationType", [("S", "IndexToDirect")]),
            _Node("UV", [("d", _CUBE_UVS)]),
            _Node("UVIndex", [("i", uv_index)]),
        ]),
        _Node("LayerElementMaterial", [("I", 0)], [
            _Node("Version", [("I", 101)]),
            _Node("Name", [("S", "")]),
            _Node("MappingInformationType", [("S", "AllSame")]),
            _Node("ReferenceInformationType", [("S", "IndexToDirect")]),
            _Node("Materials", [("i", [0])]),
        ]),
    ])

    model = _Node("Model", [("L", 200), ("S", "Model::Cube"), ("S", "Mesh")], [
        _Node("Version", [("I", 232)]),
        _Node("Properties70", [], [
            prop70("Lcl Translation", "Lcl Translation", 0.0, 100.0, 0.0),
            prop70("Lcl Scaling", "Lcl Scaling", 10.0, 10.0, 10.0),
        ]),
    ])

    material = _Node("Material", [("L", 300), ("S", "Material::RedBox"), ("S", "")], [
        _Node("Version", [("I", 102)]),
        _Node("ShadingModel", [("S", "phong")]),
        _Node("Properties70", [], [
            prop70("DiffuseColor", "Color", 0.8, 0.2, 0.2),
            prop70("SpecularColor", "Color", 1.0, 1.0, 1.0),
            prop70("ShininessExponent", "Number", 20.0),
            prop70("TransparencyFactor", "Number", 0.25),
        ]),
    ])

    connections = _Node("Connections", [], [
        _Node("C", [("S", "OO"), ("L", 200), ("L", 0)]),
        _Node("C", [("S", "OO"), ("L", 100), ("L", 200)]),
        _Node("C", [("S", "OO"), ("L", 300), ("L", 200)]),
    ])

    header = _Node("FBXHeaderExtension", [], [
        _Node("FBXHeaderVersion", [("I", 1003)]),
        _Node("FBXVersion", [("I", 7400)]),
    ])
    objects = _Node("Objects", [], [geometry, model, material])
    return [header, settings, objects, connections]


def _pack_property(kind, value):
    if kind == "I":
        return b"I" + struct.pack("<i", value)
    if kind == "L":
        return b"L" + struct.pack("<q", value)
    if kind == "D":
        return b"D" + struct.pack("<d", value)
    if kind == "S":
        data = value.encode("utf-8")
        return b"S" + struct.pack("<I", len(data)) + data
    if kind == "d":
        data = struct.pack("<%dd" % len(value), *value)
        return b"d" + struct.pack("<III", len(value), 0, len(data)) + data
    if kind == "i":
        data = struct.pack("<%di" % len(value), *value)
        return b"i" + struct.pack("<III", len(value), 0, len(data)) + data
    raise ValueError("未対応のプロパティ型: %s" % kind)


def _serialize_binary(nodes):
    """ノードツリーをバイナリ FBX (7400) にする。"""
    def emit(node, offset):
        properties = b"".join(_pack_property(kind, value) for kind, value in node.props)
        name = node.name.encode("utf-8")
        header = 13 + len(name)
        child_offset = offset + header + len(properties)
        children = b""
        for child in node.children:
            chunk, child_offset = emit(child, child_offset)
            children += chunk
        if node.children:
            children += b"\x00" * 13
            child_offset += 13
        end = child_offset
        return (struct.pack("<III", end, len(node.props), len(properties))
                + bytes([len(name)]) + name + properties + children), end

    data = b"Kaydara FBX Binary  \x00\x1a\x00" + struct.pack("<I", 7400)
    offset = len(data)
    for node in nodes:
        chunk, offset = emit(node, offset)
        data += chunk
    data += b"\x00" * 13          # トップレベルの終端
    data += b"\x00" * 16          # フッタ代わりの余白
    return data


def _serialize_ascii(nodes):
    """ノードツリーを ASCII FBX にする。"""
    lines = ["; FBX 7.4.0 project file", "; " + "-" * 40, ""]

    def value_text(kind, value):
        if kind == "S":
            return '"%s"' % value
        if kind == "D":
            return "%g" % value
        return "%d" % value

    def emit(node, depth):
        indent = "\t" * depth
        scalars = [(kind, value) for kind, value in node.props if kind not in ("d", "i")]
        arrays = [(kind, value) for kind, value in node.props if kind in ("d", "i")]
        head = "%s%s:" % (indent, node.name)
        if scalars:
            head += " " + ", ".join(value_text(k, v) for k, v in scalars)
        if arrays:
            kind, values = arrays[0]
            body = ",".join(("%g" if kind == "d" else "%d") % v for v in values)
            lines.append("%s *%d {" % (head, len(values)))
            lines.append("%s\ta: %s" % (indent, body))
            lines.append("%s}" % indent)
        elif node.children:
            lines.append(head + " {")
            for child in node.children:
                emit(child, depth + 1)
            lines.append("%s}" % indent)
        else:
            lines.append(head)

    for node in nodes:
        emit(node, 0)
    return "\n".join(lines) + "\n"


# --------------------------------------------------------------------------
# 検証
# --------------------------------------------------------------------------

def _winding_agreement(model):
    """巻き順から求めた面法線と頂点法線が一致する割合。"""
    agree = 0
    total = 0
    for face in model.faces:
        p0, p1, p2 = (model.vertices[i].position for i in face)
        u = (p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
        v = (p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])
        cross = (u[1] * v[2] - u[2] * v[1],
                 u[2] * v[0] - u[0] * v[2],
                 u[0] * v[1] - u[1] * v[0])
        normal = [sum(model.vertices[i].normal[axis] for i in face) / 3.0
                  for axis in range(3)]
        dot = sum(cross[axis] * normal[axis] for axis in range(3))
        if abs(dot) < 1e-9:
            continue
        total += 1
        if dot > 0.0:
            agree += 1
    return agree, total


def _check_cube(model, label, failures):
    def check(condition, message):
        if not condition:
            failures.append("%s: %s" % (label, message))

    check(len(model.vertices) == 24, "頂点数が 24 ではない (%d)" % len(model.vertices))
    check(len(model.faces) == 12, "面数が 12 ではない (%d)" % len(model.faces))
    check(len(model.materials) == 1, "材質数が 1 ではない (%d)" % len(model.materials))
    check(len(model.bones) == 1, "ボーンが自動生成されていない (%d)" % len(model.bones))

    agree, total = _winding_agreement(model)
    check(agree == total and total == 12,
          "面の向きが法線と一致しない (%d/%d)" % (agree, total))

    xs = [v.position[0] for v in model.vertices]
    ys = [v.position[1] for v in model.vertices]
    zs = [v.position[2] for v in model.vertices]
    # 1 単位 = 1m、MMD スケール 12.5 倍、立方体は 10 倍スケールで Y+100 に配置
    check(abs(min(xs) + 125.0) < 1e-3 and abs(max(xs) - 125.0) < 1e-3,
          "X の範囲が想定と違う (%.3f〜%.3f)" % (min(xs), max(xs)))
    check(abs(min(ys) - 1125.0) < 1e-3 and abs(max(ys) - 1375.0) < 1e-3,
          "Y の範囲が想定と違う (%.3f〜%.3f)" % (min(ys), max(ys)))
    check(abs(min(zs) + 125.0) < 1e-3 and abs(max(zs) - 125.0) < 1e-3,
          "Z の範囲が想定と違う (%.3f〜%.3f)" % (min(zs), max(zs)))

    material = model.materials[0] if model.materials else None
    if material is not None:
        check(material.name == "RedBox", "材質名が違う (%s)" % material.name)
        check(abs(material.diffuse[3] - 0.75) < 1e-6,
              "不透明度が TransparencyFactor から求められていない (%.3f)"
              % material.diffuse[3])
        check(material.surface_count == 36,
              "材質の面数が違う (%d)" % material.surface_count)
    check(all(abs(sum(v.weights[:len(v.bones)]) - 1.0) < 1e-6 for v in model.vertices),
          "ウェイト合計が 1 でない頂点がある")
    check(any(abs(v.uv[1] - 1.0) < 1e-6 for v in model.vertices),
          "UV の上下反転が行われていない")


def _dds_header(width, height, pf_flags, fourcc, bit_count, masks, data_size):
    """テスト用 DDS の 128 バイトヘッダを作る。"""
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000  # CAPS/HEIGHT/WIDTH/PIXELFORMAT/LINEARSIZE
    header = b"DDS " + struct.pack("<7I", 124, flags, height, width, data_size, 0, 1)
    header += b"\x00" * 44
    header += struct.pack("<II4sI4I", 32, pf_flags, fourcc, bit_count, *masks)
    header += struct.pack("<5I", 0x1000, 0, 0, 0, 0)
    return header


def _test_textures(failures):
    """DDS デコーダと PNG 書き出しを検証する。"""
    import texconv

    # 1) 無圧縮 BGRA の DDS
    pixels = [(10, 20, 30, 40), (200, 100, 50, 255),
              (0, 0, 0, 0), (255, 255, 255, 255)]
    body = b"".join(struct.pack("<4B", blue, green, red, alpha)
                    for red, green, blue, alpha in pixels)
    dds = _dds_header(2, 2, 0x40 | 0x1, b"\x00" * 4, 32,
                      (0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000),
                      len(body)) + body

    # 2) DXT1 の 1 ブロック(赤と青の補間)
    indices = sum(value << (2 * i) for i, value in
                  enumerate([0] * 4 + [1] * 4 + [2] * 4 + [3] * 4))
    block = struct.pack("<HHI", 0xF800, 0x001F, indices)
    dxt1 = _dds_header(4, 4, 0x4, b"DXT1", 0, (0, 0, 0, 0), len(block)) + block

    with tempfile.TemporaryDirectory(prefix="fbx2pmx_tex_") as directory:
        plain_path = os.path.join(directory, "plain.dds")
        with open(plain_path, "wb") as stream:
            stream.write(dds)
        width, height, rgba = texconv.read_dds(plain_path)
        expected = b"".join(bytes(p) for p in pixels)
        if (width, height) != (2, 2) or rgba != expected:
            failures.append("無圧縮 DDS のデコード結果が想定と違う: %r" % (rgba,))

        dxt_path = os.path.join(directory, "dxt1.dds")
        with open(dxt_path, "wb") as stream:
            stream.write(dxt1)
        width, height, rgba = texconv.read_dds(dxt_path)
        rows = [tuple(rgba[y * 16:y * 16 + 4]) for y in range(4)]
        expected_rows = [(255, 0, 0, 255), (0, 0, 255, 255),
                         (170, 0, 85, 255), (85, 0, 170, 255)]
        if rows != expected_rows:
            failures.append("DXT1 のデコード結果が想定と違う: %r" % (rows,))

        # 3) PNG を書いて読み直す
        png_path = os.path.join(directory, "out.png")
        texconv.write_png(png_path, width, height, rgba)
        with open(png_path, "rb") as stream:
            png = stream.read()
        if not png.startswith(b"\x89PNG\r\n\x1a\x0a"):
            failures.append("PNG のシグネチャが不正")
        start = png.index(b"IDAT") + 4
        length = struct.unpack_from(">I", png, start - 8)[0]
        raw = zlib.decompress(png[start:start + length])
        restored = b"".join(raw[y * (width * 4 + 1) + 1:(y + 1) * (width * 4 + 1)]
                            for y in range(height))
        if restored != rgba:
            failures.append("PNG に書いた画素が読み直した内容と一致しない")

        # 4) MMD が読める形式かの判定
        if texconv.needs_conversion("a.png") or not texconv.needs_conversion("a.DDS"):
            failures.append("MMD が読める形式の判定が正しくない")
    print("テクスチャ変換: OK (無圧縮 DDS / DXT1 / PNG 書き出し)")


def main():
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass

    nodes = _cube_nodes()
    failures = []
    with tempfile.TemporaryDirectory(prefix="fbx2pmx_test_") as directory:
        cases = (
            ("バイナリ FBX", "cube_bin.fbx", _serialize_binary(nodes), "wb"),
            ("ASCII FBX", "cube_ascii.fbx", _serialize_ascii(nodes), "w"),
        )
        for label, filename, data, mode in cases:
            fbx_path = os.path.join(directory, filename)
            if mode == "wb":
                with open(fbx_path, "wb") as stream:
                    stream.write(data)
            else:
                with open(fbx_path, "w", encoding="utf-8") as stream:
                    stream.write(data)

            pmx_path = os.path.join(directory, filename.replace(".fbx", ".pmx"))
            result = converter.convert(fbx_path, pmx_path)
            pmxfile.write_pmx(result.model, pmx_path)
            _check_cube(result.model, label + "(変換直後)", failures)
            _check_cube(pmxfile.read_pmx(pmx_path), label + "(読み直し)", failures)
            print("%s: 変換 OK (%d 頂点 / %d 面)"
                  % (label, len(result.model.vertices), len(result.model.faces)))

    _test_textures(failures)

    if failures:
        print("\n失敗:")
        for message in failures:
            print("  - " + message)
        return 1
    print("\nすべてのチェックに合格しました。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
