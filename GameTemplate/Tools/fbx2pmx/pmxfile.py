# -*- coding: utf-8 -*-
"""PMX (MikuMikuDance モデル) 2.0 の読み書き。

PMX は左手座標系・Y 軸上向き・UV 原点左上で、面は表から見て時計回り。
このモジュールは形式の入出力だけを担当し、座標変換は行わない。
"""

from __future__ import annotations

import struct

PMX_SIGNATURE = b"PMX "
PMX_VERSION = 2.0

# ボーンフラグ
BONE_TAIL_IS_INDEX = 0x0001
BONE_ROTATABLE = 0x0002
BONE_TRANSLATABLE = 0x0004
BONE_VISIBLE = 0x0008
BONE_ENABLED = 0x0010
BONE_IK = 0x0020
BONE_INHERIT_ROTATION = 0x0100
BONE_INHERIT_TRANSLATION = 0x0200
BONE_FIXED_AXIS = 0x0400
BONE_LOCAL_AXIS = 0x0800
BONE_AFTER_PHYSICS = 0x1000
BONE_EXTERNAL_PARENT = 0x2000

# マテリアル描画フラグ
MATERIAL_DOUBLE_SIDED = 0x01
MATERIAL_GROUND_SHADOW = 0x02
MATERIAL_SELF_SHADOW_MAP = 0x04
MATERIAL_SELF_SHADOW = 0x08
MATERIAL_EDGE = 0x10

# モーフ種別
MORPH_GROUP = 0
MORPH_VERTEX = 1
MORPH_BONE = 2
MORPH_UV = 3
MORPH_MATERIAL = 8

# 表示枠の要素種別
FRAME_ELEMENT_BONE = 0
FRAME_ELEMENT_MORPH = 1


class PmxVertex:
    __slots__ = ("position", "normal", "uv", "bones", "weights", "edge_scale")

    def __init__(self, position, normal, uv, bones=None, weights=None, edge_scale=1.0):
        self.position = position
        self.normal = normal
        self.uv = uv
        self.bones = bones if bones is not None else [0]
        self.weights = weights if weights is not None else [1.0]
        self.edge_scale = edge_scale


class PmxMaterial:
    def __init__(self, name, name_en=""):
        self.name = name
        self.name_en = name_en
        self.diffuse = (1.0, 1.0, 1.0, 1.0)
        self.specular = (0.0, 0.0, 0.0)
        self.specular_power = 5.0
        self.ambient = (0.5, 0.5, 0.5)
        self.flags = MATERIAL_GROUND_SHADOW | MATERIAL_SELF_SHADOW_MAP | \
            MATERIAL_SELF_SHADOW | MATERIAL_EDGE
        self.edge_color = (0.0, 0.0, 0.0, 1.0)
        self.edge_size = 1.0
        self.texture_index = -1
        self.sphere_index = -1
        self.sphere_mode = 0
        self.toon_shared = 0
        self.toon_index = -1
        self.memo = ""
        self.surface_count = 0


class PmxIkLink:
    __slots__ = ("bone_index", "has_limit", "limit_min", "limit_max")

    def __init__(self, bone_index, has_limit=False,
                 limit_min=(0.0, 0.0, 0.0), limit_max=(0.0, 0.0, 0.0)):
        self.bone_index = bone_index
        self.has_limit = has_limit
        self.limit_min = limit_min
        self.limit_max = limit_max


class PmxIk:
    __slots__ = ("target_index", "loop_count", "limit_radian", "links")

    def __init__(self, target_index, loop_count=10, limit_radian=1.0, links=None):
        self.target_index = target_index
        self.loop_count = loop_count
        self.limit_radian = limit_radian
        self.links = links if links is not None else []


class PmxBone:
    def __init__(self, name, name_en="", position=(0.0, 0.0, 0.0), parent_index=-1):
        self.name = name
        self.name_en = name_en
        self.position = position
        self.parent_index = parent_index
        self.layer = 0
        self.flags = BONE_ROTATABLE | BONE_VISIBLE | BONE_ENABLED
        self.tail_offset = (0.0, 0.0, 0.0)
        self.tail_index = -1
        self.inherit_parent_index = -1
        self.inherit_rate = 1.0
        self.fixed_axis = (0.0, 0.0, 0.0)
        self.local_axis_x = (1.0, 0.0, 0.0)
        self.local_axis_z = (0.0, 0.0, 1.0)
        self.external_key = 0
        self.ik = None


class PmxMorph:
    def __init__(self, name, name_en="", panel=4, morph_type=MORPH_VERTEX):
        self.name = name
        self.name_en = name_en
        self.panel = panel
        self.morph_type = morph_type
        self.offsets = []  # 頂点モーフなら [(頂点インデックス, (dx, dy, dz)), ...]


class PmxDisplayFrame:
    def __init__(self, name, name_en="", special=False):
        self.name = name
        self.name_en = name_en
        self.special = special
        self.elements = []  # [(種別, インデックス), ...]


# 剛体の形状
SHAPE_SPHERE = 0
SHAPE_BOX = 1
SHAPE_CAPSULE = 2

# 剛体の物理演算モード
BODY_FOLLOW_BONE = 0        # ボーン追従(当たり判定・揺れものの付け根)
BODY_PHYSICS = 1            # 物理演算
BODY_PHYSICS_ALIGNED = 2    # 物理演算 + ボーン位置合わせ


class PmxRigidBody:
    """剛体。size は球なら (半径, -, -)、カプセルなら (半径, 高さ, -)。

    rotation は MMD と同じく Y → X → Z の順に掛けるオイラー角(ラジアン)で、
    カプセルはローカル Y 軸方向に伸びる。
    """

    def __init__(self, name, name_en="", bone_index=-1):
        self.name = name
        self.name_en = name_en
        self.bone_index = bone_index
        self.group = 0
        self.collision_mask = 0xFFFF   # 衝突する相手グループのビット(1 = 衝突する)
        self.shape = SHAPE_SPHERE
        self.size = (1.0, 0.0, 0.0)
        self.position = (0.0, 0.0, 0.0)
        self.rotation = (0.0, 0.0, 0.0)
        self.mass = 1.0
        self.linear_damping = 0.5
        self.angular_damping = 0.5
        self.restitution = 0.0
        self.friction = 0.5
        self.mode = BODY_FOLLOW_BONE


class PmxJoint:
    """ばね付き 6 自由度ジョイント。"""

    def __init__(self, name, name_en="", body_a=-1, body_b=-1):
        self.name = name
        self.name_en = name_en
        self.body_a = body_a
        self.body_b = body_b
        self.position = (0.0, 0.0, 0.0)
        self.rotation = (0.0, 0.0, 0.0)
        self.translation_min = (0.0, 0.0, 0.0)
        self.translation_max = (0.0, 0.0, 0.0)
        self.rotation_min = (0.0, 0.0, 0.0)
        self.rotation_max = (0.0, 0.0, 0.0)
        self.spring_translation = (0.0, 0.0, 0.0)
        self.spring_rotation = (0.0, 0.0, 0.0)


class PmxModel:
    def __init__(self, name="model", name_en="model"):
        self.name = name
        self.name_en = name_en
        self.comment = ""
        self.comment_en = ""
        self.vertices = []
        self.faces = []          # [(a, b, c), ...]
        self.textures = []       # [相対パス, ...]
        self.materials = []
        self.bones = []
        self.morphs = []
        self.display_frames = []
        self.rigid_bodies = []
        self.joints = []


# --------------------------------------------------------------------------
# 書き出し
# --------------------------------------------------------------------------

def _unsigned_index_size(count):
    if count < 0x80:
        return 1
    if count < 0x8000:
        return 2
    return 4


def _signed_index_size(count):
    if count < 0x80:
        return 1
    if count < 0x8000:
        return 2
    return 4


_UNSIGNED_FORMATS = {1: "<B", 2: "<H", 4: "<i"}
_SIGNED_FORMATS = {1: "<b", 2: "<h", 4: "<i"}


class _Writer:
    def __init__(self, model, encoding=0):
        self.model = model
        self.encoding = encoding
        self.buffer = bytearray()
        self.vertex_index_size = _unsigned_index_size(len(model.vertices))
        self.texture_index_size = _signed_index_size(len(model.textures))
        self.material_index_size = _signed_index_size(len(model.materials))
        self.bone_index_size = _signed_index_size(len(model.bones))
        self.morph_index_size = _signed_index_size(len(model.morphs))
        self.body_index_size = _signed_index_size(len(model.rigid_bodies))

    # -- 基本 --------------------------------------------------------
    def pack(self, fmt, *values):
        self.buffer += struct.pack(fmt, *values)

    def text(self, value):
        data = (value or "").encode("utf-16-le" if self.encoding == 0 else "utf-8")
        self.buffer += struct.pack("<i", len(data)) + data

    def vertex_index(self, value):
        self.buffer += struct.pack(_UNSIGNED_FORMATS[self.vertex_index_size], value)

    def signed_index(self, value, size):
        self.buffer += struct.pack(_SIGNED_FORMATS[size], value)

    # -- 各セクション ------------------------------------------------
    def write_header(self):
        self.buffer += PMX_SIGNATURE
        self.pack("<f", PMX_VERSION)
        self.pack("<B", 8)
        self.pack("<8B", self.encoding, 0, self.vertex_index_size,
                  self.texture_index_size, self.material_index_size,
                  self.bone_index_size, self.morph_index_size, self.body_index_size)
        self.text(self.model.name)
        self.text(self.model.name_en)
        self.text(self.model.comment)
        self.text(self.model.comment_en)

    def write_vertices(self):
        self.pack("<i", len(self.model.vertices))
        for vertex in self.model.vertices:
            self.pack("<8f", vertex.position[0], vertex.position[1], vertex.position[2],
                      vertex.normal[0], vertex.normal[1], vertex.normal[2],
                      vertex.uv[0], vertex.uv[1])
            bones = vertex.bones
            weights = vertex.weights
            if len(bones) <= 1:
                self.pack("<B", 0)
                self.signed_index(bones[0] if bones else 0, self.bone_index_size)
            elif len(bones) == 2:
                self.pack("<B", 1)
                self.signed_index(bones[0], self.bone_index_size)
                self.signed_index(bones[1], self.bone_index_size)
                self.pack("<f", weights[0])
            else:
                self.pack("<B", 2)
                for i in range(4):
                    self.signed_index(bones[i] if i < len(bones) else 0,
                                      self.bone_index_size)
                for i in range(4):
                    self.pack("<f", weights[i] if i < len(weights) else 0.0)
            self.pack("<f", vertex.edge_scale)

    def write_faces(self):
        self.pack("<i", len(self.model.faces) * 3)
        fmt = _UNSIGNED_FORMATS[self.vertex_index_size][1] * 3
        packer = struct.Struct("<" + fmt).pack
        for face in self.model.faces:
            self.buffer += packer(face[0], face[1], face[2])

    def write_textures(self):
        self.pack("<i", len(self.model.textures))
        for path in self.model.textures:
            self.text(path)

    def write_materials(self):
        self.pack("<i", len(self.model.materials))
        for material in self.model.materials:
            self.text(material.name)
            self.text(material.name_en)
            self.pack("<4f", *material.diffuse)
            self.pack("<3f", *material.specular)
            self.pack("<f", material.specular_power)
            self.pack("<3f", *material.ambient)
            self.pack("<B", material.flags)
            self.pack("<4f", *material.edge_color)
            self.pack("<f", material.edge_size)
            self.signed_index(material.texture_index, self.texture_index_size)
            self.signed_index(material.sphere_index, self.texture_index_size)
            self.pack("<B", material.sphere_mode)
            self.pack("<B", material.toon_shared)
            if material.toon_shared:
                self.pack("<B", max(0, min(255, material.toon_index)))
            else:
                self.signed_index(material.toon_index, self.texture_index_size)
            self.text(material.memo)
            self.pack("<i", material.surface_count)

    def write_bones(self):
        self.pack("<i", len(self.model.bones))
        for bone in self.model.bones:
            self.text(bone.name)
            self.text(bone.name_en)
            self.pack("<3f", *bone.position)
            self.signed_index(bone.parent_index, self.bone_index_size)
            self.pack("<i", bone.layer)
            self.pack("<H", bone.flags)
            if bone.flags & BONE_TAIL_IS_INDEX:
                self.signed_index(bone.tail_index, self.bone_index_size)
            else:
                self.pack("<3f", *bone.tail_offset)
            if bone.flags & (BONE_INHERIT_ROTATION | BONE_INHERIT_TRANSLATION):
                self.signed_index(bone.inherit_parent_index, self.bone_index_size)
                self.pack("<f", bone.inherit_rate)
            if bone.flags & BONE_FIXED_AXIS:
                self.pack("<3f", *bone.fixed_axis)
            if bone.flags & BONE_LOCAL_AXIS:
                self.pack("<3f", *bone.local_axis_x)
                self.pack("<3f", *bone.local_axis_z)
            if bone.flags & BONE_EXTERNAL_PARENT:
                self.pack("<i", bone.external_key)
            if bone.flags & BONE_IK:
                ik = bone.ik
                self.signed_index(ik.target_index, self.bone_index_size)
                self.pack("<i", ik.loop_count)
                self.pack("<f", ik.limit_radian)
                self.pack("<i", len(ik.links))
                for link in ik.links:
                    self.signed_index(link.bone_index, self.bone_index_size)
                    self.pack("<B", 1 if link.has_limit else 0)
                    if link.has_limit:
                        self.pack("<3f", *link.limit_min)
                        self.pack("<3f", *link.limit_max)

    def write_morphs(self):
        self.pack("<i", len(self.model.morphs))
        for morph in self.model.morphs:
            self.text(morph.name)
            self.text(morph.name_en)
            self.pack("<B", morph.panel)
            self.pack("<B", morph.morph_type)
            self.pack("<i", len(morph.offsets))
            if morph.morph_type == MORPH_VERTEX:
                for index, delta in morph.offsets:
                    self.vertex_index(index)
                    self.pack("<3f", delta[0], delta[1], delta[2])
            elif morph.morph_type == MORPH_GROUP:
                for index, rate in morph.offsets:
                    self.signed_index(index, self.morph_index_size)
                    self.pack("<f", rate)
            else:
                raise ValueError("未対応のモーフ種別: %d" % morph.morph_type)

    def write_display_frames(self):
        self.pack("<i", len(self.model.display_frames))
        for frame in self.model.display_frames:
            self.text(frame.name)
            self.text(frame.name_en)
            self.pack("<B", 1 if frame.special else 0)
            self.pack("<i", len(frame.elements))
            for element_type, index in frame.elements:
                self.pack("<B", element_type)
                if element_type == FRAME_ELEMENT_BONE:
                    self.signed_index(index, self.bone_index_size)
                else:
                    self.signed_index(index, self.morph_index_size)

    def write_physics(self):
        self.pack("<i", len(self.model.rigid_bodies))
        for body in self.model.rigid_bodies:
            self.text(body.name)
            self.text(body.name_en)
            self.signed_index(body.bone_index, self.bone_index_size)
            self.pack("<B", body.group)
            self.pack("<H", body.collision_mask & 0xFFFF)
            self.pack("<B", body.shape)
            self.pack("<3f", *body.size)
            self.pack("<3f", *body.position)
            self.pack("<3f", *body.rotation)
            self.pack("<5f", body.mass, body.linear_damping, body.angular_damping,
                      body.restitution, body.friction)
            self.pack("<B", body.mode)

        self.pack("<i", len(self.model.joints))
        for joint in self.model.joints:
            self.text(joint.name)
            self.text(joint.name_en)
            self.pack("<B", 0)  # ばね付き 6DOF
            self.signed_index(joint.body_a, self.body_index_size)
            self.signed_index(joint.body_b, self.body_index_size)
            for vector in (joint.position, joint.rotation,
                           joint.translation_min, joint.translation_max,
                           joint.rotation_min, joint.rotation_max,
                           joint.spring_translation, joint.spring_rotation):
                self.pack("<3f", *vector)

    def run(self):
        self.write_header()
        self.write_vertices()
        self.write_faces()
        self.write_textures()
        self.write_materials()
        self.write_bones()
        self.write_morphs()
        self.write_display_frames()
        self.write_physics()
        return bytes(self.buffer)


def write_pmx(model, path, encoding=0):
    """PmxModel をファイルへ書き出す。"""
    data = _Writer(model, encoding).run()
    with open(path, "wb") as stream:
        stream.write(data)
    return len(data)


# --------------------------------------------------------------------------
# 読み込み(検証・確認用)
# --------------------------------------------------------------------------

class _Reader:
    def __init__(self, data):
        self.data = data
        self.pos = 0
        self.encoding = 0

    def unpack(self, fmt):
        values = struct.unpack_from(fmt, self.data, self.pos)
        self.pos += struct.calcsize(fmt)
        return values

    def text(self):
        length = self.unpack("<i")[0]
        raw = self.data[self.pos:self.pos + length]
        self.pos += length
        return raw.decode("utf-16-le" if self.encoding == 0 else "utf-8", errors="replace")

    def index(self, size, signed):
        fmt = (_SIGNED_FORMATS if signed else _UNSIGNED_FORMATS)[size]
        return self.unpack(fmt)[0]


def read_pmx(path):
    """PMX を読み込んで PmxModel を返す。"""
    with open(path, "rb") as stream:
        reader = _Reader(stream.read())

    signature = reader.data[:4]
    if signature != PMX_SIGNATURE:
        raise ValueError("PMX ファイルではありません: %s" % path)
    reader.pos = 4
    version = reader.unpack("<f")[0]
    globals_count = reader.unpack("<B")[0]
    flags = reader.unpack("<%dB" % globals_count)
    reader.encoding = flags[0]
    extra_uv = flags[1]
    vertex_size, texture_size, material_size = flags[2], flags[3], flags[4]
    bone_size, morph_size, body_size = flags[5], flags[6], flags[7]

    model = PmxModel()
    model.version = version
    model.name = reader.text()
    model.name_en = reader.text()
    model.comment = reader.text()
    model.comment_en = reader.text()

    count = reader.unpack("<i")[0]
    for _ in range(count):
        values = reader.unpack("<8f")
        vertex = PmxVertex(values[0:3], values[3:6], values[6:8])
        for _uv in range(extra_uv):
            reader.unpack("<4f")
        weight_type = reader.unpack("<B")[0]
        if weight_type == 0:
            vertex.bones = [reader.index(bone_size, True)]
            vertex.weights = [1.0]
        elif weight_type == 1:
            b1 = reader.index(bone_size, True)
            b2 = reader.index(bone_size, True)
            w1 = reader.unpack("<f")[0]
            vertex.bones = [b1, b2]
            vertex.weights = [w1, 1.0 - w1]
        elif weight_type == 2:
            vertex.bones = [reader.index(bone_size, True) for _ in range(4)]
            vertex.weights = list(reader.unpack("<4f"))
        elif weight_type == 3:
            b1 = reader.index(bone_size, True)
            b2 = reader.index(bone_size, True)
            w1 = reader.unpack("<f")[0]
            reader.unpack("<9f")
            vertex.bones = [b1, b2]
            vertex.weights = [w1, 1.0 - w1]
        else:
            raise ValueError("未対応のウェイト種別: %d" % weight_type)
        vertex.edge_scale = reader.unpack("<f")[0]
        model.vertices.append(vertex)

    index_count = reader.unpack("<i")[0]
    indices = [reader.index(vertex_size, False) for _ in range(index_count)]
    model.faces = [tuple(indices[i:i + 3]) for i in range(0, index_count, 3)]

    count = reader.unpack("<i")[0]
    model.textures = [reader.text() for _ in range(count)]

    count = reader.unpack("<i")[0]
    for _ in range(count):
        material = PmxMaterial(reader.text(), reader.text())
        material.diffuse = reader.unpack("<4f")
        material.specular = reader.unpack("<3f")
        material.specular_power = reader.unpack("<f")[0]
        material.ambient = reader.unpack("<3f")
        material.flags = reader.unpack("<B")[0]
        material.edge_color = reader.unpack("<4f")
        material.edge_size = reader.unpack("<f")[0]
        material.texture_index = reader.index(texture_size, True)
        material.sphere_index = reader.index(texture_size, True)
        material.sphere_mode = reader.unpack("<B")[0]
        material.toon_shared = reader.unpack("<B")[0]
        if material.toon_shared:
            material.toon_index = reader.unpack("<B")[0]
        else:
            material.toon_index = reader.index(texture_size, True)
        material.memo = reader.text()
        material.surface_count = reader.unpack("<i")[0]
        model.materials.append(material)

    count = reader.unpack("<i")[0]
    for _ in range(count):
        bone = PmxBone(reader.text(), reader.text())
        bone.position = reader.unpack("<3f")
        bone.parent_index = reader.index(bone_size, True)
        bone.layer = reader.unpack("<i")[0]
        bone.flags = reader.unpack("<H")[0]
        if bone.flags & BONE_TAIL_IS_INDEX:
            bone.tail_index = reader.index(bone_size, True)
        else:
            bone.tail_offset = reader.unpack("<3f")
        if bone.flags & (BONE_INHERIT_ROTATION | BONE_INHERIT_TRANSLATION):
            bone.inherit_parent_index = reader.index(bone_size, True)
            bone.inherit_rate = reader.unpack("<f")[0]
        if bone.flags & BONE_FIXED_AXIS:
            bone.fixed_axis = reader.unpack("<3f")
        if bone.flags & BONE_LOCAL_AXIS:
            bone.local_axis_x = reader.unpack("<3f")
            bone.local_axis_z = reader.unpack("<3f")
        if bone.flags & BONE_EXTERNAL_PARENT:
            bone.external_key = reader.unpack("<i")[0]
        if bone.flags & BONE_IK:
            target = reader.index(bone_size, True)
            loop_count, limit_radian, link_count = reader.unpack("<ifi")
            ik = PmxIk(target, loop_count, limit_radian)
            for _link in range(link_count):
                link = PmxIkLink(reader.index(bone_size, True))
                link.has_limit = bool(reader.unpack("<B")[0])
                if link.has_limit:
                    link.limit_min = reader.unpack("<3f")
                    link.limit_max = reader.unpack("<3f")
                ik.links.append(link)
            bone.ik = ik
        model.bones.append(bone)

    count = reader.unpack("<i")[0]
    for _ in range(count):
        morph = PmxMorph(reader.text(), reader.text())
        morph.panel = reader.unpack("<B")[0]
        morph.morph_type = reader.unpack("<B")[0]
        offset_count = reader.unpack("<i")[0]
        for _offset in range(offset_count):
            if morph.morph_type == MORPH_VERTEX:
                index = reader.index(vertex_size, False)
                morph.offsets.append((index, reader.unpack("<3f")))
            elif morph.morph_type == MORPH_GROUP:
                index = reader.index(morph_size, True)
                morph.offsets.append((index, reader.unpack("<f")[0]))
            elif morph.morph_type == MORPH_BONE:
                index = reader.index(bone_size, True)
                morph.offsets.append((index, reader.unpack("<7f")))
            elif morph.morph_type in (MORPH_UV, 4, 5, 6, 7):
                index = reader.index(vertex_size, False)
                morph.offsets.append((index, reader.unpack("<4f")))
            elif morph.morph_type == MORPH_MATERIAL:
                index = reader.index(material_size, True)
                reader.unpack("<B")
                morph.offsets.append((index, reader.unpack("<28f")))
            else:
                raise ValueError("未対応のモーフ種別: %d" % morph.morph_type)
        model.morphs.append(morph)

    count = reader.unpack("<i")[0]
    for _ in range(count):
        frame = PmxDisplayFrame(reader.text(), reader.text())
        frame.special = bool(reader.unpack("<B")[0])
        element_count = reader.unpack("<i")[0]
        for _element in range(element_count):
            element_type = reader.unpack("<B")[0]
            if element_type == FRAME_ELEMENT_BONE:
                frame.elements.append((element_type, reader.index(bone_size, True)))
            else:
                frame.elements.append((element_type, reader.index(morph_size, True)))
        model.display_frames.append(frame)

    count = reader.unpack("<i")[0]
    for _ in range(count):
        body = PmxRigidBody(reader.text(), reader.text())
        body.bone_index = reader.index(bone_size, True)
        body.group = reader.unpack("<B")[0]
        body.collision_mask = reader.unpack("<H")[0]
        body.shape = reader.unpack("<B")[0]
        body.size = reader.unpack("<3f")
        body.position = reader.unpack("<3f")
        body.rotation = reader.unpack("<3f")
        (body.mass, body.linear_damping, body.angular_damping,
         body.restitution, body.friction) = reader.unpack("<5f")
        body.mode = reader.unpack("<B")[0]
        model.rigid_bodies.append(body)

    count = reader.unpack("<i")[0]
    for _ in range(count):
        joint = PmxJoint(reader.text(), reader.text())
        joint_type = reader.unpack("<B")[0]
        if joint_type != 0:
            raise ValueError("未対応のジョイント種別: %d" % joint_type)
        joint.body_a = reader.index(body_size, True)
        joint.body_b = reader.index(body_size, True)
        (joint.position, joint.rotation, joint.translation_min, joint.translation_max,
         joint.rotation_min, joint.rotation_max, joint.spring_translation,
         joint.spring_rotation) = [reader.unpack("<3f") for _vector in range(8)]
        model.joints.append(joint)
    return model
