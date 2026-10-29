# -*- coding: utf-8 -*-
"""fbx2pmx のセルフテスト。

テスト用の立方体 FBX (バイナリ / ASCII) をその場で生成し、PMX へ変換して
結果を検証する。外部のモデルファイルは不要。

    python selftest.py
"""

from __future__ import annotations

import math
import os
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import converter   # noqa: E402
import mat4        # noqa: E402
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
    if kind == "R":
        return b"R" + struct.pack("<I", len(value)) + value
    if kind == "l":
        data = struct.pack("<%dq" % len(value), *value)
        return b"l" + struct.pack("<III", len(value), 0, len(data)) + data
    if kind == "f":
        data = struct.pack("<%df" % len(value), *value)
        return b"f" + struct.pack("<III", len(value), 0, len(data)) + data
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


def _texture_scene_nodes(embedded_png):
    """材質ごとにテクスチャの崩れ方が違う、6 材質の立方体シーンを作る。

    Body  : 作者 PC の絶対パス Body_D.tga を参照(候補には body_d.png がある)
    Face  : テクスチャ未接続(候補には Face_BaseColor.png と Face_Normal.png)
    Hair  : hair.png を参照するが実体は FBX に埋め込み
    Eye_L : テクスチャ未接続(候補には eye_l.png)
    Skin  : テクスチャ未接続で、似た名前の画像も無い
    Cloth : cloth.dds を参照(DDS なので PNG へ変換されるはず)
    """
    nodes = _cube_nodes()
    objects = [n for n in nodes if n.name == "Objects"][0]
    connections = [n for n in nodes if n.name == "Connections"][0]
    geometry = [n for n in objects.children if n.name == "Geometry"][0]
    layer = [n for n in geometry.children if n.name == "LayerElementMaterial"][0]
    layer.children = [
        _Node("Version", [("I", 101)]),
        _Node("Name", [("S", "")]),
        _Node("MappingInformationType", [("S", "ByPolygon")]),
        _Node("ReferenceInformationType", [("S", "IndexToDirect")]),
        _Node("Materials", [("i", [0, 1, 2, 3, 4, 5])]),
    ]
    objects.children = [n for n in objects.children if n.name != "Material"]
    connections.children = [c for c in connections.children if c.props[1][1] != 300]

    def texture_node(obj_id, name, path):
        return _Node("Texture", [("L", obj_id), ("S", "Texture::" + name), ("S", "")], [
            _Node("Type", [("S", "TextureVideoClip")]),
            _Node("FileName", [("S", path)]),
            _Node("RelativeFilename", [("S", path)]),
        ])

    materials = ("Body", "Face", "Hair", "Eye_L", "Skin", "Cloth")
    for number, name in enumerate(materials):
        material_id = 310 + number
        objects.children.append(_Node(
            "Material", [("L", material_id), ("S", "Material::" + name), ("S", "")],
            [_Node("Version", [("I", 102)])]))
        connections.children.append(_Node("C", [("S", "OO"), ("L", material_id), ("L", 200)]))

    textures = (
        (401, "Body", "C:/Users/someone/Desktop/model/Body_D.tga", 310),
        (402, "HairTex", "hair.png", 312),
        (403, "ClothTex", "cloth.dds", 315),
    )
    for texture_id, name, path, material_id in textures:
        objects.children.append(texture_node(texture_id, name, path))
        connections.children.append(_Node(
            "C", [("S", "OP"), ("L", texture_id), ("L", material_id), ("S", "DiffuseColor")]))
    objects.children.append(_Node(
        "Video", [("L", 502), ("S", "Video::HairTex"), ("S", "Clip")], [
            _Node("Type", [("S", "Clip")]),
            _Node("Filename", [("S", "hair.png")]),
            _Node("RelativeFilename", [("S", "hair.png")]),
            _Node("Content", [("R", embedded_png)]),
        ]))
    connections.children.append(_Node("C", [("S", "OO"), ("L", 502), ("L", 402)]))
    return nodes


def _test_texture_matching(all_failures):
    """FBX とテクスチャを一緒に渡したときの自動割り当てを検証する。"""
    import json

    import texconv

    failures = []
    pixel = bytes([200, 120, 80, 255]) * 4
    with tempfile.TemporaryDirectory(prefix="fbx2pmx_texmatch_") as directory:
        model_dir = os.path.join(directory, "model")
        source_dir = os.path.join(directory, "textures_from_somewhere")
        out_dir = os.path.join(directory, "out")
        for path in (model_dir, source_dir, out_dir):
            os.makedirs(path)

        embedded_path = os.path.join(directory, "embedded.png")
        texconv.write_png(embedded_path, 2, 2, pixel)
        with open(embedded_path, "rb") as stream:
            embedded = stream.read()
        for name in ("body_d.PNG", "Face_BaseColor.png", "Face_Normal.png", "eye_l.png",
                     "unrelated_rock.png"):
            texconv.write_png(os.path.join(source_dir, name), 2, 2, pixel)
        dds = _dds_header(1, 1, 0x40 | 0x1, b"\x00" * 4, 32,
                          (0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000), 4)
        with open(os.path.join(source_dir, "cloth.dds"), "wb") as stream:
            stream.write(dds + bytes([10, 20, 30, 255]))

        fbx_path = os.path.join(model_dir, "chara.fbx")
        with open(fbx_path, "wb") as stream:
            stream.write(_serialize_binary(_texture_scene_nodes(embedded)))

        def run(texture_map=None):
            options = converter.ConvertOptions()
            options.texture_dirs = [source_dir]
            options.texture_map_path = texture_map
            result = converter.convert(fbx_path, os.path.join(out_dir, "chara.pmx"), options)
            model = result.model
            table = {}
            for material in model.materials:
                index = material.texture_index
                table[material.name] = model.textures[index] if index >= 0 else None
            methods = {name: method for name, _e, method, _s in result.texture_assignments}
            return table, methods

        table, methods = run()
        expected = {
            "Body": ("body_d.PNG", "拡張子違い"),
            "Face": ("Face_BaseColor.png", "名前から推定"),
            "Hair": ("hair.png", "埋め込み"),
            "Eye_L": ("eye_l.png", "名前から推定"),
            "Skin": (None, "なし"),
            "Cloth": ("cloth.png", "同名ファイル"),
        }
        for name, (texture, method) in expected.items():
            if table.get(name) != texture or methods.get(name) != method:
                failures.append("テクスチャ割り当て: %s が %r/%r (期待 %r/%r)"
                                % (name, table.get(name), methods.get(name), texture, method))
        for name in ("body_d.PNG", "Face_BaseColor.png", "hair.png", "eye_l.png", "cloth.png"):
            if not os.path.isfile(os.path.join(out_dir, name)):
                failures.append("テクスチャ割り当て: 出力先に %s が置かれていない" % name)
        with open(os.path.join(out_dir, "hair.png"), "rb") as stream:
            if stream.read() != embedded:
                failures.append("テクスチャ割り当て: 埋め込み画像の中身が元と違う")

        map_path = os.path.join(directory, "map.json")
        with open(map_path, "w", encoding="utf-8") as stream:
            json.dump({"Skin": os.path.join(source_dir, "Face_BaseColor.png"), "Face": None},
                      stream)
        table, methods = run(map_path)
        if table.get("Skin") != "Face_BaseColor.png" or methods.get("Skin") != "割り当て表":
            failures.append("テクスチャ割り当て: 割り当て表の指定が効いていない (Skin=%r)"
                            % table.get("Skin"))
        if table.get("Face") is not None:
            failures.append("テクスチャ割り当て: 割り当て表の null が効いていない")

    all_failures.extend(failures)
    if not failures:
        print("テクスチャ自動割り当て: OK (拡張子違い / 名前推定 / 埋め込み / DDS 変換 / 割り当て表)")


def _test_physics(all_failures):
    """揺れものの剛体・ジョイント生成とウィンク生成、PMX への書き出し、
    プレビュー用の簡易物理演算を検証する。"""
    import mmdize
    import physics
    import physsim
    import posing

    failures = []

    def check(condition, message):
        if not condition:
            failures.append("物理: " + message)

    model = pmxfile.PmxModel()

    def add_bone(name, position, parent):
        model.bones.append(pmxfile.PmxBone(name, name, position, parent))
        return len(model.bones) - 1

    root = add_bone("全ての親", (0.0, 0.0, 0.0), -1)
    lower = add_bone("下半身", (0.0, 10.0, 0.0), root)
    upper = add_bone("上半身", (0.0, 10.5, 0.0), root)
    neck = add_bone("首", (0.0, 15.0, 0.0), upper)
    head = add_bone("頭", (0.0, 16.0, 0.0), neck)
    hair0 = add_bone("Hair_00", (0.0, 18.0, 2.0), head)
    hair1 = add_bone("Hair_01", (0.0, 14.0, 3.0), hair0)
    skirt = add_bone("Skirt_00", (0.0, 10.0, -1.2), lower)
    for side, x in (("左", 1.0), ("右", -1.0)):
        leg = add_bone(side + "足", (x, 9.5, 0.0), lower)
        knee = add_bone(side + "ひざ", (x, 5.0, 0.0), leg)
        add_bone(side + "足首", (x, 1.0, 0.0), knee)

    def vertex(position, bone):
        model.vertices.append(pmxfile.PmxVertex(position, (0.0, 1.0, 0.0), (0.0, 0.0),
                                                [bone], [1.0]))
        return len(model.vertices) - 1

    for t in range(5):  # 頭・髪・スカートに頂点を乗せる(半径の推定用)
        vertex((0.8, 16.0 + t * 0.4, 0.0), head)
        vertex((0.4, 18.0 - t, 2.0 + t * 0.25), hair0)
        vertex((0.4, 14.0 - t, 3.0), hair1)
        vertex((0.9, 10.0 - t * 0.5, -1.2), skirt)
        vertex((-0.9, 10.0 - t * 0.5, -1.2), skirt)
    for index in (lower, upper):
        vertex(model.bones[index].position, index)
    left_eye = vertex((1.0, 17.0, -1.0), head)
    right_eye = vertex((-1.0, 17.0, -1.0), head)
    blink = pmxfile.PmxMorph("まばたき", "blink", 2)
    blink.offsets = [(left_eye, (0.0, -0.1, 0.0)), (right_eye, (0.0, -0.1, 0.0))]
    model.morphs.append(blink)

    created = mmdize.add_winks(model)
    winks = {m.name: m for m in model.morphs}
    check(created == 2, "ウィンクが 2 個作られていない (%d)" % created)
    check([i for i, _ in winks.get("ウィンク", pmxfile.PmxMorph("")).offsets] == [left_eye],
          "ウィンクがモデルの左目(+X)だけになっていない")
    check([i for i, _ in winks.get("ウィンク右", pmxfile.PmxMorph("")).offsets] == [right_eye],
          "ウィンク右がモデルの右目(-X)だけになっていない")

    report = physics.generate(model)
    bodies = {body.name: (i, body) for i, body in enumerate(model.rigid_bodies)}
    for name in ("頭", "下半身", "左足", "Hair_00", "Hair_01", "Skirt_00"):
        check(name in bodies, "%s の剛体が無い" % name)
    if not failures:
        check(bodies["頭"][1].mode == pmxfile.BODY_FOLLOW_BONE, "頭がボーン追従になっていない")
        for name in ("Hair_00", "Hair_01", "Skirt_00"):
            check(bodies[name][1].mode != pmxfile.BODY_FOLLOW_BONE, "%s が物理演算になっていない" % name)
        pairs = {(model.rigid_bodies[j.body_a].name, model.rigid_bodies[j.body_b].name)
                 for j in model.joints}
        check(pairs == {("頭", "Hair_00"), ("Hair_00", "Hair_01"), ("下半身", "Skirt_00")},
              "ジョイントのつながりが違う %r" % sorted(pairs))
        hair = bodies["Hair_00"][1]
        axis = physics.rotation_axis_y(hair.rotation)
        direction = mat4.normalize(tuple(model.bones[hair1].position[k]
                                         - model.bones[hair0].position[k] for k in range(3)))
        check(sum(axis[k] * direction[k] for k in range(3)) > 0.9999,
              "髪のカプセルがボーンの向きに沿っていない")
        check(not hair.collision_mask & (1 << hair.group), "髪が同じグループ同士でぶつかる設定")
        check(not hair.collision_mask & (1 << bodies["頭"][1].group),
              "髪の付け根が頭の当たり判定とぶつかる設定")
        skirt_body = bodies["Skirt_00"][1]
        check(skirt_body.collision_mask & (1 << bodies["左足"][1].group),
              "スカートが脚の当たり判定とぶつからない設定")
        check(report.chains == {"髪": 1, "スカート": 1}, "連鎖の数え方が違う %r" % report.chains)

        # 脚のすぐ前に垂れるスカートは、脚との衝突を外さず細くして隙間を作る
        check(report.relaxed == 0, "当たり判定を外した組がある (%d)" % report.relaxed)
        for i, body in enumerate(model.rigid_bodies):
            if body.mode == pmxfile.BODY_FOLLOW_BONE:
                continue
            a, b = physsim._endpoints(body)
            for other in model.rigid_bodies:
                if other.mode != pmxfile.BODY_FOLLOW_BONE \
                        or not body.collision_mask & (1 << other.group) \
                        or not other.collision_mask & (1 << body.group):
                    continue
                c, d = physsim._endpoints(other)
                gap = physics.segment_distance(a, b, c, d) - body.size[0] - other.size[0]
                check(gap > -1e-6, "%s が静止状態で %s に食い込んでいる (%.3f)"
                      % (body.name, other.name, gap))

        # プレビューの簡易物理: 重力で垂れても、脚を突き抜けず回転の制限の内側に収まる
        pose = posing.Pose(model)
        pose.solve()
        rest = {}
        for body in model.rigid_bodies:
            if body.mode != pmxfile.BODY_FOLLOW_BONE:
                rest[body.bone_index] = physsim._endpoints(body)
        physsim.simulate(model, [pose] * 45)
        colliders = physsim._posed_colliders(model, pose)
        for body in model.rigid_bodies:
            if body.mode == pmxfile.BODY_FOLLOW_BONE:
                continue
            origin = pose.global_position[body.bone_index]
            rotation = pose.global_rotation[body.bone_index]
            pivot = model.bones[body.bone_index].position
            a, b = (tuple(origin[k] + v[k] for k in range(3)) for v in
                    (posing.q_rotate(rotation, tuple(p[k] - pivot[k] for k in range(3)))
                     for p in rest[body.bone_index]))
            for index, (c, d, radius) in colliders.items():
                other = model.rigid_bodies[index]
                if not body.collision_mask & (1 << other.group) \
                        or not other.collision_mask & (1 << body.group):
                    continue
                gap = physics.segment_distance(a, b, c, d) - body.size[0] - radius
                check(gap > -0.05 * (body.size[0] + radius),
                      "簡易物理で %s が %s を突き抜けた (%.3f)" % (body.name, other.name, gap))
        swing = pose.global_rotation[skirt]
        check(math.degrees(posing.q_angle(swing, pose.global_rotation[lower])) <= 25.5,
              "簡易物理でスカートが回転の制限を超えた")

        with tempfile.TemporaryDirectory(prefix="fbx2pmx_phys_") as directory:
            path = os.path.join(directory, "phys.pmx")
            pmxfile.write_pmx(model, path)
            loaded = pmxfile.read_pmx(path)
        check(len(loaded.rigid_bodies) == len(model.rigid_bodies)
              and len(loaded.joints) == len(model.joints), "剛体・ジョイントの読み直しで数が違う")
        if loaded.rigid_bodies:
            a, b = loaded.rigid_bodies[-1], model.rigid_bodies[-1]
            check(a.name == b.name and a.bone_index == b.bone_index and a.mode == b.mode
                  and a.collision_mask == b.collision_mask
                  and all(abs(x - y) < 1e-5 for x, y in zip(a.rotation, b.rotation)),
                  "剛体の読み直しで値が変わった")

    all_failures.extend(failures)
    if not failures:
        print("物理・ウィンク: OK (剛体 %d / ジョイント %d / 向き / 衝突設定 / 隙間 / 簡易物理 / 読み直し)"
              % (len(model.rigid_bodies), len(model.joints)))


def _test_face(all_failures):
    """表情モーフの自動生成(まばたき・ウィンク・あ・舌)を板の顔で検証する。"""
    import facial
    import mmdize

    failures = []

    def check(condition, message):
        if not condition:
            failures.append("表情: " + message)

    model = pmxfile.PmxModel()
    model.bones.append(pmxfile.PmxBone("頭", "head", (0.0, 0.0, 0.0), -1))
    model.bones.append(pmxfile.PmxBone("左目", "eye_L", (0.4, 0.3, -1.0), 0))
    model.bones.append(pmxfile.PmxBone("右目", "eye_R", (-0.4, 0.3, -1.0), 0))

    def vertex(position, bone=0):
        model.vertices.append(pmxfile.PmxVertex(position, (0.0, 0.0, -1.0), (0.0, 0.0),
                                                [bone], [1.0]))
        return len(model.vertices) - 1

    # 正面(-Z)を向いた顔の板。y = -0.5 の |x| <= 0.3 に唇の合わせ目(上下で別の頂点。
    # 位置は同じ)があり、面のつながりはそこで切れている
    steps = [i / 10.0 for i in range(-10, 11)]
    upper = {}
    lower = {}
    for row, y in enumerate(steps):
        for col, x in enumerate(steps):
            upper[row, col] = vertex((x, y, -1.0))
            lower[row, col] = vertex((x, y, -1.0)) if abs(y + 0.5) < 1e-9 and abs(x) < 0.3 + 1e-9 \
                else upper[row, col]
    for row in range(len(steps) - 1):
        for col in range(len(steps) - 1):
            a, b = upper[row, col], upper[row, col + 1]      # 合わせ目の上の面は上唇側の頂点
            c, d = lower[row + 1, col], lower[row + 1, col + 1]  # 下の面は下唇側の頂点
            model.faces.extend([(a, c, b), (b, c, d)])
    for x in (0.4, -0.4):                                    # 瞳の板(目のボーンに乗る)
        bone = 1 if x > 0 else 2
        corners = [vertex((x + dx, 0.3 + dy, -1.02), bone)
                   for dx, dy in ((-0.12, 0.15), (0.12, 0.15), (0.12, -0.15), (-0.12, -0.15))]
        model.faces.extend([(corners[0], corners[1], corners[2]), (corners[0], corners[2], corners[3])])
    tongue = [vertex(p) for p in ((-0.2, -0.45, -1.15), (0.2, -0.45, -1.15), (0.0, -0.55, -1.2))]
    model.faces.append(tuple(tongue))
    material = pmxfile.PmxMaterial("顔")
    material.surface_count = len(model.faces) * 3
    model.materials.append(material)

    report = facial.generate(model)
    mmdize.add_winks(model)
    morphs = {morph.name: dict(morph.offsets) for morph in model.morphs}
    for name in ("まばたき", "笑い", "あ", "い", "う", "え", "お", "ぺろっ", "ウィンク", "ウィンク右"):
        check(name in morphs, "%s が作られていない (%r)" % (name, report.created))
    if not failures:
        blink = morphs["まばたき"]
        lid = upper[steps.index(0.5), steps.index(0.4)]      # 左目の上
        brow = upper[steps.index(1.0), steps.index(0.4)]
        check(blink.get(lid, (0.0, 0.0, 0.0))[1] < -0.05, "まばたきで上まぶたが下がらない")
        check(brow not in blink, "まばたきで額まで動いている")
        check(set(morphs["ウィンク"]) <= {i for i in blink if model.vertices[i].position[0] > 0},
              "ウィンクに右目(-X)の頂点が入っている")
        mouth = morphs["あ"]
        top = upper[steps.index(-0.5), steps.index(0.0)]
        bottom = lower[steps.index(-0.5), steps.index(0.0)]
        check(abs(mouth.get(top, (0.0, 0.0, 0.0))[1]) < 1e-6, "あ で上唇まで動いている")
        check(mouth.get(bottom, (0.0, 0.0, 0.0))[1] < -0.01, "あ で下唇が下がらない")
        check(report.tongue_tucked and all(model.vertices[i].position[2] > -1.0 for i in tongue),
              "飛び出した舌が口の中へしまわれていない")
        check(all(morphs["ぺろっ"][i][2] < 0.0 for i in tongue), "ぺろっ で舌が前へ出ない")

    all_failures.extend(failures)
    if not failures:
        print("表情モーフの自動生成: OK (まばたき / 笑い / ウィンク / あいうえお / 舌)")


# ---- モーション変換用: T スタンスの人型 + スキン + 30 フレームのアニメーション ----

# (名前, 親, 位置 cm)。FBX は Y 上・正面 +Z・右手系で、モデルの左は +X
_HUMANOID = (
    ("mixamorig:Hips", None, (0.0, 100.0, 0.0)),
    ("mixamorig:Spine", "mixamorig:Hips", (0.0, 110.0, 0.0)),
    ("mixamorig:Neck", "mixamorig:Spine", (0.0, 150.0, 0.0)),
    ("mixamorig:Head", "mixamorig:Neck", (0.0, 160.0, 0.0)),
    ("mixamorig:LeftShoulder", "mixamorig:Spine", (5.0, 145.0, 0.0)),
    ("mixamorig:LeftArm", "mixamorig:LeftShoulder", (15.0, 145.0, 0.0)),
    ("mixamorig:LeftForeArm", "mixamorig:LeftArm", (40.0, 145.0, 0.0)),
    ("mixamorig:LeftHand", "mixamorig:LeftForeArm", (65.0, 145.0, 0.0)),
    ("mixamorig:RightShoulder", "mixamorig:Spine", (-5.0, 145.0, 0.0)),
    ("mixamorig:RightArm", "mixamorig:RightShoulder", (-15.0, 145.0, 0.0)),
    ("mixamorig:RightForeArm", "mixamorig:RightArm", (-40.0, 145.0, 0.0)),
    ("mixamorig:RightHand", "mixamorig:RightForeArm", (-65.0, 145.0, 0.0)),
    ("mixamorig:LeftUpLeg", "mixamorig:Hips", (10.0, 95.0, 0.0)),
    ("mixamorig:LeftLeg", "mixamorig:LeftUpLeg", (10.0, 52.0, 2.0)),
    ("mixamorig:LeftFoot", "mixamorig:LeftLeg", (10.0, 8.0, 0.0)),
    ("mixamorig:LeftToeBase", "mixamorig:LeftFoot", (10.0, 2.0, 12.0)),
    ("mixamorig:RightUpLeg", "mixamorig:Hips", (-10.0, 95.0, 0.0)),
    ("mixamorig:RightLeg", "mixamorig:RightUpLeg", (-10.0, 52.0, 2.0)),
    ("mixamorig:RightFoot", "mixamorig:RightLeg", (-10.0, 8.0, 0.0)),
    ("mixamorig:RightToeBase", "mixamorig:RightFoot", (-10.0, 2.0, 12.0)),
)
MOTION_FRAMES = 30


def _humanoid_curves(frame):
    """フレームごとのアニメーション値 {(ボーン, プロパティ): (x, y, z)}。"""
    import math
    t = frame / float(MOTION_FRAMES - 1)
    wave = math.sin(math.pi * t)
    return {
        ("mixamorig:Hips", "Lcl Translation"): (0.0, 100.0 - 10.0 * wave, 20.0 * t),
        ("mixamorig:Hips", "Lcl Rotation"): (0.0, 30.0 * t, 0.0),
        ("mixamorig:Spine", "Lcl Rotation"): (20.0 * wave, 0.0, 0.0),
        ("mixamorig:LeftArm", "Lcl Rotation"): (15.0 * t, 0.0, -60.0 * t),
        ("mixamorig:LeftForeArm", "Lcl Rotation"): (0.0, 40.0 * t, 0.0),
        ("mixamorig:LeftUpLeg", "Lcl Rotation"): (-40.0 * t, 0.0, 0.0),
        ("mixamorig:LeftLeg", "Lcl Rotation"): (50.0 * t, 0.0, 0.0),
        ("mixamorig:Head", "Lcl Rotation"): (0.0, 20.0 * wave, 0.0),
    }


def _humanoid_nodes():
    """スキン付きメッシュとアニメーションを持つ人型 FBX のノードツリーを作る。"""
    positions = {name: position for name, _parent, position in _HUMANOID}
    ids = {name: 1000 + i for i, (name, _p, _pos) in enumerate(_HUMANOID)}
    children = {}
    for name, parent, _pos in _HUMANOID:
        children.setdefault(parent, []).append(name)

    # 各ボーンの区間に、制御点を共有しない小さな三角形を並べる(頂点番号 = 制御点番号)
    points, weights = [], {}

    def triangle(center, bone, other=None, ratio=1.0):
        for offset in ((1.5, 0.0, 1.0), (-1.5, 0.5, 1.0), (0.0, -0.5, -1.5)):
            weights.setdefault(bone, []).append((len(points), ratio))
            if other is not None:
                weights.setdefault(other, []).append((len(points), 1.0 - ratio))
            points.append(tuple(center[k] + offset[k] for k in range(3)))

    for name, _parent, position in _HUMANOID:
        kids = children.get(name, [])
        tip = positions[kids[0]] if kids else tuple(position[k] + (0.0, 6.0, 0.0)[k] for k in range(3))
        for t in (0.25, 0.5, 0.75):
            triangle(tuple(position[k] + (tip[k] - position[k]) * t for k in range(3)), name)
        if kids:
            triangle(tuple(position[k] + (tip[k] - position[k]) * 0.95 for k in range(3)),
                     name, kids[0], 0.5)
    polygon_index = []
    for i in range(0, len(points), 3):
        polygon_index.extend([i, i + 1, -(i + 2) - 1])

    def prop70(name, type_name, *values):
        return _Node("P", [("S", name), ("S", type_name), ("S", ""), ("S", "A")]
                     + [("D", float(v)) for v in values])

    settings = _Node("GlobalSettings", [], [_Node("Properties70", [], [
        _Node("P", [("S", "UpAxis"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 1)]),
        _Node("P", [("S", "UpAxisSign"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 1)]),
        _Node("P", [("S", "FrontAxis"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 2)]),
        _Node("P", [("S", "FrontAxisSign"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 1)]),
        _Node("P", [("S", "CoordAxis"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 0)]),
        _Node("P", [("S", "CoordAxisSign"), ("S", "int"), ("S", "Integer"), ("S", ""), ("I", 1)]),
        prop70("UnitScaleFactor", "double", 1.0),
    ])])
    objects = []
    connections = []
    geometry = _Node("Geometry", [("L", 100), ("S", "Geometry::Body"), ("S", "Mesh")], [
        _Node("Vertices", [("d", [c for p in points for c in p])]),
        _Node("PolygonVertexIndex", [("i", polygon_index)]),
    ])
    objects.append(geometry)
    objects.append(_Node("Model", [("L", 200), ("S", "Model::Body"), ("S", "Mesh")], [
        _Node("Version", [("I", 232)])]))
    connections += [_Node("C", [("S", "OO"), ("L", 200), ("L", 0)]),
                    _Node("C", [("S", "OO"), ("L", 100), ("L", 200)])]

    skin_id = 300
    objects.append(_Node("Deformer", [("L", skin_id), ("S", "Deformer::Skin"), ("S", "Skin")]))
    connections.append(_Node("C", [("S", "OO"), ("L", skin_id), ("L", 100)]))
    for number, (name, parent, position) in enumerate(_HUMANOID):
        local = position if parent is None else tuple(
            position[k] - positions[parent][k] for k in range(3))
        objects.append(_Node("Model", [("L", ids[name]), ("S", "Model::" + name),
                                       ("S", "LimbNode")], [
            _Node("Version", [("I", 232)]),
            _Node("Properties70", [], [prop70("Lcl Translation", "Lcl Translation", *local)]),
        ]))
        connections.append(_Node("C", [("S", "OO"), ("L", ids[name]),
                                       ("L", ids[parent] if parent else 0)]))
        x, y, z = position
        link = [1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, x, y, z, 1.0]   # 行ベクトル規約
        inverse = [1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, -x, -y, -z, 1.0]
        entries = weights.get(name, [])
        cluster_id = 400 + number
        objects.append(_Node("Deformer", [("L", cluster_id), ("S", "SubDeformer::" + name),
                                          ("S", "Cluster")], [
            _Node("Indexes", [("i", [i for i, _w in entries])]),
            _Node("Weights", [("d", [w for _i, w in entries])]),
            _Node("Transform", [("d", inverse)]),
            _Node("TransformLink", [("d", link)]),
        ]))
        connections += [_Node("C", [("S", "OO"), ("L", cluster_id), ("L", skin_id)]),
                        _Node("C", [("S", "OO"), ("L", ids[name]), ("L", cluster_id)])]

    # アニメーション(毎フレームにキーを打ったベイク済みの形)
    stop = 46186158000 // 30 * (MOTION_FRAMES - 1)
    objects.append(_Node("AnimationStack", [("L", 9000), ("S", "AnimStack::Take 001"), ("S", "")],
                         [_Node("Properties70", [], [
                             _Node("P", [("S", "LocalStart"), ("S", "KTime"), ("S", "Time"),
                                         ("S", ""), ("L", 0)]),
                             _Node("P", [("S", "LocalStop"), ("S", "KTime"), ("S", "Time"),
                                         ("S", ""), ("L", stop)])])]))
    objects.append(_Node("AnimationLayer", [("L", 9001), ("S", "AnimLayer::BaseLayer"), ("S", "")]))
    connections.append(_Node("C", [("S", "OO"), ("L", 9001), ("L", 9000)]))
    samples = [_humanoid_curves(f) for f in range(MOTION_FRAMES)]
    times = [46186158000 // 30 * f for f in range(MOTION_FRAMES)]
    next_id = 10000
    for (name, prop_name) in samples[0]:
        node_id = next_id
        next_id += 1
        first = samples[0][(name, prop_name)]
        objects.append(_Node("AnimationCurveNode", [("L", node_id), ("S", "AnimCurveNode::"
                                                                    + prop_name[-1]), ("S", "")],
                             [_Node("Properties70", [], [prop70("d|X", "Number", first[0]),
                                                        prop70("d|Y", "Number", first[1]),
                                                        prop70("d|Z", "Number", first[2])])]))
        connections += [_Node("C", [("S", "OO"), ("L", node_id), ("L", 9001)]),
                        _Node("C", [("S", "OP"), ("L", node_id), ("L", ids[name]),
                                    ("S", prop_name)])]
        for axis, channel in enumerate(("d|X", "d|Y", "d|Z")):
            curve_id = next_id
            next_id += 1
            objects.append(_Node("AnimationCurve", [("L", curve_id), ("S", "AnimCurve::"),
                                                    ("S", "")], [
                _Node("KeyTime", [("l", times)]),
                _Node("KeyValueFloat", [("f", [s[(name, prop_name)][axis] for s in samples])]),
                _Node("KeyAttrFlags", [("i", [4])]),
                _Node("KeyAttrRefCount", [("i", [MOTION_FRAMES])]),
            ]))
            connections.append(_Node("C", [("S", "OP"), ("L", curve_id), ("L", node_id),
                                           ("S", channel)]))

    header = _Node("FBXHeaderExtension", [], [_Node("FBXVersion", [("I", 7400)])])
    return ([header, settings, _Node("Objects", [], objects), _Node("Connections", [], connections)],
            points, weights, ids)


def _test_motion(all_failures):
    """FBX のアニメーション → VMD を、FBX 側で求めた正解の頂点位置と突き合わせて検証する。"""
    import math

    import motion
    import posing
    import vmdfile

    failures = []
    nodes, points, weights, ids = _humanoid_nodes()

    def run(directory, arm_angle, reduce):
        """モデルとモーションを変換し、VMD を当てた頂点と FBX の正解の最大差を返す。"""
        fbx_path = os.path.join(directory, "dance.fbx")
        if not os.path.isfile(fbx_path):
            with open(fbx_path, "wb") as stream:
                stream.write(_serialize_binary(nodes))
        pmx_path = os.path.join(directory, "dance.pmx")
        vmd_path = os.path.join(directory, "dance.vmd")
        model_options = converter.ConvertOptions()
        model_options.arm_angle = arm_angle
        model_result = converter.convert(fbx_path, pmx_path, model_options)
        pmxfile.write_pmx(model_result.model, pmx_path)
        motion_options = motion.MotionOptions()
        motion_options.model.arm_angle = arm_angle
        motion_options.reduce = reduce
        motion_data, report, _result = motion.convert_motion(fbx_path, motion_options)
        vmdfile.write_vmd(motion_data, vmd_path)
        model = pmxfile.read_pmx(pmx_path)
        loaded = vmdfile.read_vmd(vmd_path)

        # 正解: FBX のスキニングで求めた頂点位置(PMX 空間)
        scene = model_result.scene
        take = scene.takes()[0]
        worst = 0.0
        for frame in (0, 9, 18, 29):
            time = 46186158000 // 30 * frame
            cache = {}
            moved = {}
            for name, bone_id in ids.items():
                animated = scene.global_matrix_at(scene.objects[bone_id], take, time, cache)
                moved[name] = mat4.multiply(animated,
                                            mat4.inverse(model_result.bind_globals[bone_id]))
            expected = [[0.0, 0.0, 0.0] for _ in points]
            for name, entries in weights.items():
                for index, weight in entries:
                    p = mat4.transform_point(moved[name], points[index])
                    for k in range(3):
                        expected[index][k] += weight * p[k]
            expected = [mat4.transform_point(model_result.matrix, p) for p in expected]
            pose, morphs, missing = posing.pose_from_motion(model, loaded, frame)
            if missing:
                failures.append("モーション: PMX に無い名前がある %r" % missing)
            actual, _normals = pose.skinned_vertices(morphs)
            worst = max(worst, max(math.dist(a, e) for a, e in zip(actual, expected)))
        return worst, report, loaded, model_result

    with tempfile.TemporaryDirectory(prefix="fbx2pmx_motion_") as directory:
        # 1) A スタンス化も間引きも無し: 変換の計算そのものが正確か
        exact, _report, _loaded, _result = run(directory, "off", False)
        # 2) 既定(T スタンスを A スタンス化 + キーを間引く)
        worst, report, loaded, model_result = run(directory, "auto", True)

    names = {frame.name for frame in loaded.bone_frames}
    for name in ("センター", "下半身", "上半身", "左腕", "左ひじ", "左足ＩＫ", "右足ＩＫ", "頭"):
        if name not in names:
            failures.append("モーション: %s のキーが無い" % name)
    if report.keys_after >= report.keys_before:
        failures.append("モーション: キーが間引かれていない (%d → %d)"
                        % (report.keys_before, report.keys_after))
    if model_result.rig is None or not model_result.rig.arm_rotations:
        failures.append("モーション: T スタンスの腕が A スタンス化されていない(打ち消しの検証にならない)")
    if exact > 1e-4:
        failures.append("モーション: A スタンス化・間引き無しでも FBX の正解とずれる (最大 %.6f)" % exact)
    # A スタンス化した肩と腕の境目のブレンド頂点はわずかに戻りきらない(LBS の性質)。
    # 間引きの許容誤差(0.3 度)と合わせても脚 1 本 9.5 単位に対して 0.06 未満に収まる
    if worst > 0.06:
        failures.append("モーション: VMD を当てた頂点が FBX の正解とずれている (最大 %.4f)" % worst)

    all_failures.extend(failures)
    if not failures:
        print("モーション変換: OK (%d フレーム / キー %d → %d / 正解との差 %.6f、既定設定で %.4f)"
              % (report.frames, report.keys_before, report.keys_after, exact, worst))


def _test_tk(all_failures):
    """ゲーム形式(tkm / tks / tka)を作って PMX / VMD へ変換し、tka 側の正解と突き合わせる。"""
    import math

    import motion
    import posing
    import tkconvert

    failures = []
    _nodes, points, weights, _ids = _humanoid_nodes()
    names = [name for name, _parent, _position in _HUMANOID]
    index_of = {name: i for i, name in enumerate(names)}
    # FBX(Y 上・正面 +Z)の座標を 3ds Max(Z 上・正面 -Y)へ: (x, y, z) -> (x, -z, y)
    to_max = (1.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0)
    from_max = mat4.transpose(to_max)
    positions = {name: mat4.transform_point(to_max, p) for name, _parent, p in _HUMANOID}
    parents = {name: parent for name, parent, _p in _HUMANOID}

    def row4x3(m):     # 列ベクトル規約の 4x4 -> tk の 4x3(行ベクトル規約)
        return [m[0], m[4], m[8], m[1], m[5], m[9], m[2], m[6], m[10], m[3], m[7], m[11]]

    def local_matrix(name, frame):
        values = _humanoid_curves(frame)
        parent = parents[name]
        rest = _HUMANOID[index_of[name]][2]
        offset = rest if parent is None else tuple(
            rest[k] - _HUMANOID[index_of[parent]][2][k] for k in range(3))
        offset = values.get((name, "Lcl Translation"), offset)
        rotation = values.get((name, "Lcl Rotation"), (0.0, 0.0, 0.0))
        local = mat4.multiply(mat4.translation(*offset), mat4.euler_degrees(*rotation))
        return mat4.multiply_all(to_max, local, from_max)

    with tempfile.TemporaryDirectory(prefix="fbx2pmx_tk_") as directory:
        tks = bytearray(struct.pack("<i", len(names)))
        for name in names:
            raw = name.encode("utf-8")
            parent = parents[name]
            bind = mat4.translation(*positions[name])
            tks += bytes([len(raw)]) + raw + b"\x00" + struct.pack(
                "<i", index_of[parent] if parent else -1)
            tks += struct.pack("<12f", *row4x3(bind)) + struct.pack("<12f", *row4x3(mat4.inverse(bind)))
        tkm = bytearray(struct.pack("<BBH", 100, 0, 1))
        tkm += struct.pack("<IIB3x", 1, len(points), 4)
        tkm += struct.pack("<I", 8) + b"body.png\x00" + struct.pack("<4I", 0, 0, 0, 0)
        influence = {}
        for name, entries in weights.items():
            for index, weight in entries:
                influence.setdefault(index, []).append((index_of[name], weight))
        for index, point in enumerate(points):
            bones = (influence[index] + [(-1, 0.0)] * 4)[:4]
            tkm += struct.pack("<3f3f2f4f4h", *mat4.transform_point(to_max, point), 0.0, 0.0, 1.0,
                               0.25, 0.125, *[w for _b, w in bones], *[b for b, _w in bones])
        tkm += struct.pack("<i", len(points) // 3)
        tkm += struct.pack("<%dI" % len(points), *[i + 1 for i in range(len(points))])
        tka = bytearray(struct.pack("<II", MOTION_FRAMES * len(names), 0))
        for frame in range(MOTION_FRAMES):
            for name in names:
                tka += struct.pack("<If", index_of[name], frame / 30.0)
                tka += struct.pack("<12f", *row4x3(local_matrix(name, frame)))
        paths = {}
        for extension, data in (("tks", tks), ("tkm", tkm), ("tka", tka)):
            paths[extension] = os.path.join(directory, "hero." + extension)
            with open(paths[extension], "wb") as stream:
                stream.write(bytes(data))

        model_options = converter.ConvertOptions()
        model_options.arm_angle = "off"
        model_options.skip_textures = True
        result = tkconvert.convert_tkm(paths["tkm"], os.path.join(directory, "hero.pmx"),
                                       model_options)
        motion_options = motion.MotionOptions()
        motion_options.model.arm_angle = "off"
        motion_options.reduce = False
        data, report, _skeleton = tkconvert.convert_tka(paths["tka"], motion_options)

        # 骨格(tks)が横に無い tkm: 黙ってボーン 1 本の置物にせず、はっきり止める。
        # 一緒に渡された tks があればそれを使う
        alone = os.path.join(directory, "x", "y", "z")
        os.makedirs(alone)
        lonely = os.path.join(alone, "hero.tkm")
        with open(lonely, "wb") as stream:
            stream.write(bytes(tkm))
        try:
            tkconvert.convert_tkm(lonely, os.path.join(alone, "hero.pmx"), model_options)
            failures.append("tk: 骨格の無い tkm がエラーにならず、ボーンなしで変換された")
        except ValueError as error:
            if ".tks" not in str(error):
                failures.append("tk: 骨格が無いときのエラーに .tks の案内が無い (%s)" % error)
        given = tkconvert.convert_tkm(lonely, os.path.join(alone, "hero.pmx"), model_options,
                                      given=[paths["tks"]])
        if len(given.model.bones) < len(names):
            failures.append("tk: 一緒に渡した tks が骨格として使われていない")

    model = result.model
    bone_names = {bone.name for bone in model.bones}
    for name in ("センター", "下半身", "上半身", "左腕", "左ひざ", "左足ＩＫ"):
        if name not in bone_names:
            failures.append("tk: %s が無い" % name)
    if len(model.vertices) != len(points) or len(model.faces) != len(points) // 3:
        failures.append("tk: 頂点・面の数が違う (%d / %d)" % (len(model.vertices), len(model.faces)))
    if any(abs(v.uv[0] - 0.25) > 1e-6 or abs(v.uv[1] - 0.125) > 1e-6 for v in model.vertices):
        failures.append("tk: UV が tkm のまま入っていない (上下反転など)")
    worst = 0.0
    inverse_bind = {name: mat4.inverse(mat4.translation(*positions[name])) for name in names}
    for frame in (0, 15, 29):
        world = {}
        for name in names:
            local = local_matrix(name, frame)
            world[name] = local if parents[name] is None else mat4.multiply(world[parents[name]], local)
        expected = [[0.0, 0.0, 0.0] for _ in points]
        for name, entries in weights.items():
            skin = mat4.multiply(world[name], inverse_bind[name])
            for index, weight in entries:
                p = mat4.transform_point(skin, mat4.transform_point(to_max, points[index]))
                for k in range(3):
                    expected[index][k] += weight * p[k]
        expected = [mat4.transform_point(result.matrix, p) for p in expected]
        pose, morphs, missing = posing.pose_from_motion(model, data, frame)
        if missing:
            failures.append("tk: PMX に無い名前がある %r" % missing)
        actual, _normals = pose.skinned_vertices(morphs)
        worst = max(worst, max(math.dist(a, e) for a, e in zip(actual, expected)))
    if worst > 1e-3:
        failures.append("tk: tka から作った VMD を当てた頂点が正解とずれる (最大 %.5f)" % worst)

    all_failures.extend(failures)
    if not failures:
        print("ゲーム形式 tkm / tks / tka: OK (頂点 %d / %d フレーム / 正解との差 %.6f)"
              % (len(model.vertices), report.frames, worst))


def _test_rig(all_failures):
    """MMD 標準ボーン化(センター周り・足ＩＫ・両目・腕の角度・モーフ名)を検証する。"""
    import math

    import mmdize

    failures = []
    try:
        _check_rig(failures, math, mmdize, semi_standard=False)
        _check_rig(failures, math, mmdize, semi_standard=True)
    finally:
        all_failures.extend(failures)
    if not failures:
        print("MMD 標準ボーン化: OK (足ＩＫ / センター周り / 両目 / A スタンス / モーフ名 / "
              "準標準: 腕捩・手捩・肩P・腰・足IK親・足D)")


def _check_rig(failures, math, mmdize, semi_standard):
    model = pmxfile.PmxModel()

    def add_bone(name, position, parent):
        model.bones.append(pmxfile.PmxBone(name, name, position, parent))
        return len(model.bones) - 1

    # Mixamo 型の骨格: Hips の下に背骨と脚がぶら下がり、腕は水平(T スタンス)
    armature = add_bone("Armature", (0.0, 0.0, 0.0), -1)
    hips = add_bone("センター", (0.0, 10.0, 0.0), armature)
    spine = add_bone("上半身", (0.0, 11.0, 0.0), hips)
    neck = add_bone("首", (0.0, 15.0, 0.0), spine)
    head = add_bone("頭", (0.0, 16.0, 0.0), neck)
    add_bone("左目", (0.5, 17.0, -0.5), head)
    add_bone("右目", (-0.5, 17.0, -0.5), head)
    for side, x in (("左", 1.0), ("右", -1.0)):
        shoulder = add_bone(side + "肩", (0.5 * x, 14.5, 0.0), spine)
        arm = add_bone(side + "腕", (1.5 * x, 14.5, 0.0), shoulder)
        elbow = add_bone(side + "ひじ", (3.5 * x, 14.5, 0.0), arm)
        add_bone(side + "手首", (5.5 * x, 14.5, 0.0), elbow)
        leg = add_bone(side + "足", (x, 9.5, 0.0), hips)
        knee = add_bone(side + "ひざ", (x, 5.0, 0.0), leg)
        add_bone(side + "足首", (x, 1.0, 0.0), knee)

    # 各ボーンに 1 頂点ずつ + つま先位置の推定用に足先の頂点
    for index, bone in enumerate(model.bones):
        model.vertices.append(pmxfile.PmxVertex(bone.position, (0.0, 1.0, 0.0), (0.0, 0.0),
                                                [index], [1.0]))
    for side, x in (("左", 1.0), ("右", -1.0)):
        ankle = [i for i, b in enumerate(model.bones) if b.name == side + "足首"][0]
        model.vertices.append(pmxfile.PmxVertex((x, 0.0, -2.0), (0.0, 1.0, 0.0), (0.0, 0.0),
                                                [ankle], [1.0]))
    for name in ("MTH_aShape", "Blink"):
        morph = pmxfile.PmxMorph(name, name)
        morph.offsets = [(0, (0.0, 0.1, 0.0))]
        model.morphs.append(morph)
    owners = [id(model.bones[v.bones[0]]) for v in model.vertices]
    owner_names = [model.bones[v.bones[0]].name for v in model.vertices]

    mmdize.rename_morphs(model)
    report = mmdize.standardize(model, semi_standard=semi_standard)

    bones = model.bones
    index = {bone.name: i for i, bone in enumerate(bones)}
    label = "MMD 化(準標準あり): " if semi_standard else "MMD 化: "

    def check(condition, message):
        if not condition:
            failures.append(label + message)

    def parent(name):
        i = index.get(name)
        if i is None or bones[i].parent_index < 0:
            return None
        return bones[bones[i].parent_index].name

    for name in ("全ての親", "センター", "グルーブ", "下半身", "上半身", "両目",
                 "左足ＩＫ", "左つま先ＩＫ", "右足ＩＫ", "右つま先ＩＫ", "左つま先", "右つま先"):
        check(name in index, "%s が作られていない" % name)
    if failures:
        return
    check(bones[0].name == "全ての親" and bones[0].parent_index < 0, "全ての親が根になっていない")
    check(parent("グルーブ") == "センター", "グルーブの親がセンターでない")
    trunk = "腰" if semi_standard else "グルーブ"
    check(parent("下半身") == trunk, "下半身の親が%sでない (%s)" % (trunk, parent("下半身")))
    check(parent("上半身") == trunk, "上半身が下半身から切り離されていない (%s)" % parent("上半身"))
    check(not bones[index["下半身"]].flags & pmxfile.BONE_TRANSLATABLE, "下半身が移動可になっている")

    for side in ("左", "右"):
        ik = bones[index[side + "足ＩＫ"]]
        chain = [ik.ik.target_index] + [link.bone_index for link in ik.ik.links]
        check([bones[i].name for i in chain] == [side + "足首", side + "ひざ", side + "足"],
              "%s足ＩＫの対象とリンクが違う" % side)
        check(all(i < index[side + "足ＩＫ"] for i in chain),
              "%s足ＩＫが連鎖より前に並んでいる" % side)
        check(ik.ik.links[0].has_limit and ik.ik.links[0].limit_max[0] < 0.0,
              "%sひざの角度制限が無い" % side)
        check(parent(side + "つま先ＩＫ") == side + "足ＩＫ", "%sつま先ＩＫの親が違う" % side)
        toe = bones[index[side + "つま先"]].position
        check(abs(toe[2] + 2.0) < 1e-6 and abs(toe[1]) < 1e-6,
              "%sつま先が足先の頂点位置に置かれていない %r" % (side, toe))
        arm = bones[index[side + "腕"]].position
        hand = bones[index[side + "手首"]].position
        angle = math.degrees(math.atan2(-(hand[1] - arm[1]), abs(hand[0] - arm[0])))
        check(abs(angle - mmdize.DEFAULT_ARM_ANGLE) < 0.01,
              "%s腕が A スタンスになっていない (%.2f 度)" % (side, angle))

    check(index["両目"] < index["左目"] and index["両目"] < index["右目"],
          "両目が左目・右目より後ろに並んでいる")
    left_eye = bones[index["左目"]]
    check(left_eye.flags & pmxfile.BONE_INHERIT_ROTATION
          and left_eye.inherit_parent_index == index["両目"], "左目が両目から回転付与されていない")
    check([m.name for m in model.morphs] == ["あ", "まばたき"],
          "モーフ名が変換されていない %r" % [m.name for m in model.morphs])
    check(not report.warnings, "警告が出ている %r" % report.warnings)
    check(all(abs(sum(v.weights[:len(v.bones)]) - 1.0) < 1e-6 for v in model.vertices),
          "ウェイトの合計が 1 でない頂点がある")
    if not semi_standard:
        check([id(bones[v.bones[0]]) for v in model.vertices] == owners,
              "ボーンの並べ替え後に頂点のウェイト先がずれている")
        return

    # ---- 準標準ボーン ----
    for name in ("腰", "腰キャンセル左", "左肩P", "左肩C", "左腕捩", "左腕捩1", "左腕捩2", "左腕捩3",
                 "左手捩", "左足IK親", "左足D", "左ひざD", "左足首D", "左足先EX"):
        check(name in index, "%s が作られていない" % name)
    if failures:
        return
    check(parent("左足") == "腰キャンセル左", "左足の親が腰キャンセル左でない")
    cancel = bones[index["腰キャンセル左"]]
    check(cancel.inherit_parent_index == index["腰"] and cancel.inherit_rate == -1.0,
          "腰キャンセルが腰の回転を打ち消していない")
    check(parent("左肩") == "左肩P" and parent("左腕") == "左肩C", "肩P / 肩C の親子が違う")
    check(parent("左ひじ") == "左腕捩" and parent("左手首") == "左手捩", "腕捩 / 手捩 が連鎖に入っていない")
    twist = bones[index["左腕捩"]]
    arm_direction = mat4.normalize(tuple(bones[index["左ひじ"]].position[k]
                                         - bones[index["左腕"]].position[k] for k in range(3)))
    check(twist.flags & pmxfile.BONE_FIXED_AXIS
          and all(abs(twist.fixed_axis[k] - arm_direction[k]) < 1e-6 for k in range(3)),
          "左腕捩の軸が腕の向き(A スタンス後)に固定されていない %r" % (twist.fixed_axis,))
    for number, rate in ((1, 0.25), (2, 0.5), (3, 0.75)):
        helper = bones[index["左腕捩%d" % number]]
        check(helper.inherit_parent_index == index["左腕捩"] and abs(helper.inherit_rate - rate) < 1e-9,
              "左腕捩%d の付与が違う" % number)
    check(parent("左足ＩＫ") == "左足IK親", "足ＩＫの親が足IK親でない")
    leg_d = bones[index["左足D"]]
    check(leg_d.inherit_parent_index == index["左足"] and index["左足D"] > index["左足ＩＫ"],
          "左足D が IK の後ろで左足の回転を受け取っていない")
    for i, vertex in enumerate(model.vertices):
        if owner_names[i] in ("左足", "左ひざ"):
            check(bones[vertex.bones[0]].name == owner_names[i] + "D",
                  "%s のウェイトが D 系へ移っていない" % owner_names[i])


def _test_preview(all_failures):
    """pmxpreview の描画(画素ごとのテクスチャ・裏面の省略・前後関係)を小さな板で確かめる。"""
    try:
        from PIL import Image
    except ImportError:
        print("プレビュー描画: Pillow が無いので省略")
        return
    import pmxpreview

    failures = []
    model = pmxfile.PmxModel()
    model.bones.append(pmxfile.PmxBone("センター", "center"))
    # 正面(-Z 側)を向いた板。テクスチャは左半分が赤・右半分が青
    for position, uv in (((-1, 1, 0), (0, 0)), ((1, 1, 0), (1, 0)),
                         ((1, -1, 0), (1, 1)), ((-1, -1, 0), (0, 1))):
        model.vertices.append(pmxfile.PmxVertex(position, (0, 0, -1), uv))
    # 板より手前にある裏向きの緑の三角形(片面描画なので見えてはいけない)
    for position in ((0.2, 0.3, -0.5), (0.8, 0.3, -0.5), (0.8, -0.3, -0.5)):
        model.vertices.append(pmxfile.PmxVertex(position, (0, 0, 1), (0, 0)))
    model.faces = [(0, 1, 2), (0, 2, 3), (4, 6, 5)]
    model.textures = ["plate.png"]
    plate = pmxfile.PmxMaterial("板")
    plate.texture_index = 0
    plate.surface_count = 6
    back = pmxfile.PmxMaterial("裏")
    back.diffuse = (0.0, 1.0, 0.0, 1.0)
    back.surface_count = 3
    model.materials = [plate, back]

    with tempfile.TemporaryDirectory(prefix="fbx2pmx_preview_") as directory:
        texture = Image.new("RGBA", (2, 2), (255, 0, 0, 255))
        texture.putpixel((1, 0), (0, 0, 255, 255))
        texture.putpixel((1, 1), (0, 0, 255, 255))
        texture.save(os.path.join(directory, "plate.png"))
        textures, missing = pmxpreview.load_textures(model, directory)
        if missing:
            failures.append("preview: テクスチャが読めない %r" % missing)
        path = os.path.join(directory, "preview.png")
        positions = [v.position for v in model.vertices]
        normals = [v.normal for v in model.vertices]
        pmxpreview.render(model, positions, normals, path, size=64, textures=textures)
        image = Image.open(path).convert("RGB")
        left, right = image.getpixel((16, 32)), image.getpixel((48, 32))

    if not (left[0] > 180 and left[2] < 60):
        failures.append("preview: 板の左半分が赤く塗られない %r" % (left,))
    if not (right[2] > 180 and right[0] < 60):
        failures.append("preview: 板の右半分が青く塗られない(裏向きの面が描かれた?) %r" % (right,))

    all_failures.extend(failures)
    if not failures:
        print("プレビュー描画: OK (テクスチャ / 裏面の省略)")


def _test_uvcheck(all_failures):
    """UV の重なりから、材質とテクスチャの取り違えを見つけて直せるか。"""
    try:
        from PIL import Image
    except ImportError:
        print("テクスチャの取り違え検出: Pillow が無いので省略")
        return
    import random
    import uvcheck

    failures = []
    model = pmxfile.PmxModel()
    model.bones.append(pmxfile.PmxBone("センター", "center"))

    def quad(u0, v0, u1, v1):
        base = len(model.vertices)
        for u, v in ((u0, v0), (u1, v0), (u1, v1), (u0, v1)):
            model.vertices.append(pmxfile.PmxVertex((u, v, 0.0), (0, 0, -1), (u, v)))
        model.faces.extend([(base, base + 1, base + 2), (base, base + 2, base + 3)])

    model.textures = ["a.png", "b.png"]
    for name, texture, rect in (("体", 0, (0.0, 0.0, 1.0, 1.0)),      # a 全面を使う
                                ("スカート", 0, (0.1, 0.1, 0.4, 0.4)),  # 本当は b の部品
                                ("飾り", 1, (0.6, 0.6, 0.9, 0.9))):
        material = pmxfile.PmxMaterial(name)
        material.texture_index = texture
        material.surface_count = 6
        model.materials.append(material)
        quad(*rect)
    with tempfile.TemporaryDirectory(prefix="fbx2pmx_uv_") as directory:
        random.seed(1)
        for name in ("a.png", "b.png"):
            image = Image.new("RGBA", (64, 64))
            image.putdata([(random.randrange(256), random.randrange(256), random.randrange(256), 255)
                           for _ in range(64 * 64)])
            image.save(os.path.join(directory, name))
        fixed = uvcheck.fix_swapped_textures(model, directory)
    if [m.texture_index for m in model.materials] != [0, 1, 1]:
        failures.append("uv: 取り違えた材質が付け替えられていない %r" % fixed)

    all_failures.extend(failures)
    if not failures:
        print("テクスチャの取り違え検出: OK")


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
    _test_texture_matching(failures)
    _test_rig(failures)
    _test_physics(failures)
    _test_motion(failures)
    _test_tk(failures)
    _test_preview(failures)
    _test_face(failures)
    _test_uvcheck(failures)

    if failures:
        print("\n失敗:")
        for message in failures:
            print("  - " + message)
        return 1
    print("\nすべてのチェックに合格しました。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
