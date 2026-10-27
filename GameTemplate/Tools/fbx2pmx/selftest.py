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
    """揺れものの剛体・ジョイント生成とウィンク生成、PMX への書き出しを検証する。"""
    import mmdize
    import physics

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
    skirt = add_bone("Skirt_00", (0.0, 10.0, -2.5), lower)
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
        vertex((0.5, 10.0 - t * 0.5, -2.5), skirt)
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
        print("物理・ウィンク: OK (剛体 %d / ジョイント %d / 向き / 衝突設定 / 読み直し)"
              % (len(model.rigid_bodies), len(model.joints)))


def _test_rig(all_failures):
    """MMD 標準ボーン化(センター周り・足ＩＫ・両目・腕の角度・モーフ名)を検証する。"""
    import math

    import mmdize

    failures = []
    try:
        _check_rig(failures, math, mmdize)
    finally:
        all_failures.extend(failures)
    if not failures:
        print("MMD 標準ボーン化: OK (足ＩＫ / センター周り / 両目 / A スタンス / モーフ名)")


def _check_rig(failures, math, mmdize):
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
        arm = add_bone(side + "腕", (1.5 * x, 14.5, 0.0), spine)
        add_bone(side + "手首", (5.5 * x, 14.5, 0.0), arm)
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

    mmdize.rename_morphs(model)
    report = mmdize.standardize(model)

    bones = model.bones
    index = {bone.name: i for i, bone in enumerate(bones)}

    def check(condition, message):
        if not condition:
            failures.append("MMD 化: " + message)

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
    check(parent("下半身") == "グルーブ", "下半身の親がグルーブでない")
    check(parent("上半身") == "グルーブ", "上半身が下半身から切り離されていない")
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
    check([id(bones[v.bones[0]]) for v in model.vertices] == owners,
          "ボーンの並べ替え後に頂点のウェイト先がずれている")
    check([m.name for m in model.morphs] == ["あ", "まばたき"],
          "モーフ名が変換されていない %r" % [m.name for m in model.morphs])
    check(not report.warnings, "警告が出ている %r" % report.warnings)


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

    if failures:
        print("\n失敗:")
        for message in failures:
            print("  - " + message)
        return 1
    print("\nすべてのチェックに合格しました。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
