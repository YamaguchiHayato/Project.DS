# -*- coding: utf-8 -*-
"""ゲーム(k2Engine)独自形式の tkm / tks / tka を PMX / VMD へ変換する。

- tkm: モデル(メッシュ・材質・スキン)。三角形リスト、インデックスは 1 始まり
- tks: 骨格。ボーンごとにワールドのバインド行列(4x3・行ベクトル規約)
- tka: アニメーション。ボーンごと・時刻ごとのローカル行列(4x3)

どれも 3ds Max のエクスポーターが書き出す形式で、座標は Max のまま
(右手系・Z 上・正面 -Y)。単位の情報は入っていないので、人型なら脚の長さで
縮尺を合わせ、それ以外は 1 単位 = 1cm とみなす。
"""

from __future__ import annotations

import math
import os
import struct

import bonenames
import converter
import mat4
import motion
import pmxfile
import posing
import uvcheck

TKM_VERSION = 100
_TKM_VERTEX = struct.Struct("<3f3f2f4f4h")      # 56 バイト
_TKA_KEY = struct.Struct("<If12f")               # 56 バイト


class TkmMesh:
    __slots__ = ("materials", "vertices", "groups")

    def __init__(self):
        self.materials = []    # [[アルベド, 法線, スペキュラ, 反射, 屈折] のファイル名]
        self.vertices = []     # [(座標, 法線, UV, 重み, ボーン番号)]
        self.groups = []       # 材質ごとの三角形の頂点番号(0 始まり)


class TksBone:
    __slots__ = ("name", "parent", "bind")

    def __init__(self, name, parent, bind):
        self.name = name
        self.parent = parent
        self.bind = bind       # ワールドのバインド行列(列ベクトル規約の 4x4)


def _row4x3_to_matrix(r):
    """tk 形式の 4x3 行列(行ベクトル規約・最終行が平行移動)を列ベクトル規約の 4x4 にする。"""
    return (r[0], r[3], r[6], r[9],
            r[1], r[4], r[7], r[10],
            r[2], r[5], r[8], r[11],
            0.0, 0.0, 0.0, 1.0)


def _decode(raw):
    for encoding in ("utf-8", "cp932"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1")


def read_tkm(path):
    with open(path, "rb") as stream:
        data = stream.read()
    version, _flat, count = struct.unpack_from("<BBH", data, 0)
    if version != TKM_VERSION:
        raise ValueError("tkm のバージョンが %d ではありません(%d): %s" % (TKM_VERSION, version, path))
    pos = 4
    meshes = []
    for _mesh in range(count):
        mesh = TkmMesh()
        material_count, vertex_count, index_size = struct.unpack_from("<IIB", data, pos)
        pos += 12
        for _material in range(material_count):
            names = []
            for _texture in range(5):
                length = struct.unpack_from("<I", data, pos)[0]
                pos += 4
                if length:
                    names.append(_decode(data[pos:pos + length]))
                    pos += length + 1
                else:
                    names.append("")
            mesh.materials.append(names)
        for i in range(vertex_count):
            v = _TKM_VERTEX.unpack_from(data, pos + i * _TKM_VERTEX.size)
            mesh.vertices.append((v[0:3], v[3:6], v[6:8], v[8:12], v[12:16]))
        pos += vertex_count * _TKM_VERTEX.size
        code = "H" if index_size == 2 else "I"
        for _material in range(material_count):
            polygons = struct.unpack_from("<i", data, pos)[0]
            pos += 4
            indices = struct.unpack_from("<%d%s" % (polygons * 3, code), data, pos)
            pos += polygons * 3 * index_size
            mesh.groups.append([i - 1 for i in indices])   # Max は 1 始まり
        meshes.append(mesh)
    return meshes


def read_tks(path):
    with open(path, "rb") as stream:
        data = stream.read()
    count = struct.unpack_from("<i", data, 0)[0]
    pos = 4
    bones = []
    for _ in range(count):
        length = data[pos]
        pos += 1
        name = _decode(data[pos:pos + length])
        pos += length + 1
        parent = struct.unpack_from("<i", data, pos)[0]
        pos += 4
        bind = struct.unpack_from("<12f", data, pos)
        pos += 96                                   # バインド行列 + その逆行列
        bones.append(TksBone(name, parent, _row4x3_to_matrix(bind)))
    return bones


def read_tka(path):
    """tka を読んで [(ボーン番号, 時刻(秒), ローカル行列)] を返す。"""
    with open(path, "rb") as stream:
        data = stream.read()
    key_count, event_count = struct.unpack_from("<II", data, 0)
    pos = 8
    for _ in range(event_count):
        length = struct.unpack_from("<I", data, pos + 4)[0]
        pos += 8 + length + 1
    keys = []
    for i in range(key_count):
        values = _TKA_KEY.unpack_from(data, pos + i * _TKA_KEY.size)
        keys.append((values[0], values[1], _row4x3_to_matrix(values[2:14])))
    return keys


# --------------------------------------------------------------------------
# 骨格ファイルを探す
# --------------------------------------------------------------------------

def _nearby_tks(folder, levels, limit=3000):
    """folder とその上 levels 階層までのフォルダ以下にある tks を、近い順に返す。

    デスクトップなど大きなフォルダの下で時間がかからないよう、見るフォルダ数に上限を設ける。
    """
    seen = set()
    found = []
    visited = 0
    current = folder
    for _level in range(levels + 1):
        for root, dirs, names in os.walk(current):
            dirs[:] = [d for d in dirs if not d.startswith(".")]
            visited += 1
            if visited > limit:
                return found
            for name in sorted(names):
                if name.lower().endswith(".tks"):
                    path = os.path.normpath(os.path.join(root, name))
                    if path not in seen:
                        seen.add(path)
                        found.append(path)
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
    return found


def find_skeleton_for_model(tkm_path, candidates=(), loose=True):
    """tkm の骨格(tks)を探す。見つからなければ None。

    1. 一緒に渡された tks のうち同じ名前のもの(無ければ 1 つだけ渡されていればそれ)
    2. tkm と同じフォルダの同じ名前の tks(無ければフォルダに 1 つだけの tks)
    3. 近くのフォルダ(2 階層上まで)にある同じ名前の tks

    loose: 名前が違っても「1 つだけ」の tks を使うか。ボーンに乗っていない置物の
    メッシュでは False にして、無関係な骨格を付けないようにする。
    """
    stem = os.path.splitext(os.path.basename(tkm_path))[0].lower()

    def same_name(paths):
        return [p for p in paths if os.path.splitext(os.path.basename(p))[0].lower() == stem]

    candidates = list(candidates)
    if same_name(candidates):
        return same_name(candidates)[0]
    if loose and len(candidates) == 1:
        return candidates[0]
    folder = os.path.dirname(os.path.abspath(tkm_path))
    local = [os.path.join(folder, name) for name in sorted(os.listdir(folder))
             if name.lower().endswith(".tks")]
    if same_name(local):
        return same_name(local)[0]
    if loose and len(local) == 1:
        return local[0]
    nearby = same_name(_nearby_tks(folder, 2))
    return nearby[0] if nearby else None


def is_skinned(meshes):
    """tkm の頂点がボーンに乗っているか(骨格が無いと動かせないメッシュか)。"""
    return any(weight > 0.0 and index >= 0
               for mesh in meshes for vertex in mesh.vertices
               for weight, index in zip(vertex[3], vertex[4]))


def _tks_bone_count(path):
    with open(path, "rb") as stream:
        return struct.unpack("<i", stream.read(4))[0]


def find_skeleton_for_animation(tka_path, keys, given=()):
    """tka のボーン数と一致する tks を、一緒に渡されたもの → 近くのフォルダの順に探す。"""
    needed = max((key[0] for key in keys), default=-1) + 1
    matching = [p for p in given if _tks_bone_count(p) == needed]
    if matching:
        return matching[0]
    folder = os.path.dirname(os.path.abspath(tka_path))
    label = os.path.basename(folder).lower()
    seen = set()
    candidates = []
    current = folder
    for _level in range(5):
        for root, _dirs, names in os.walk(current):
            for name in names:
                if name.lower().endswith(".tks"):
                    path = os.path.normpath(os.path.join(root, name))
                    if path not in seen:
                        seen.add(path)
                        candidates.append(path)
        parent = os.path.dirname(current)
        if parent == current:
            break
        current = parent
        if candidates and any(_tks_bone_count(p) == needed for p in candidates):
            break
    matching = [p for p in candidates if _tks_bone_count(p) == needed]
    if not matching:
        return None
    # フォルダ名(player / zombie / Bomber)と名前が近いものを優先
    matching.sort(key=lambda p: (label not in p.lower(), len(p)))
    return matching[0]


# --------------------------------------------------------------------------
# 変換
# --------------------------------------------------------------------------

def _axis_matrix(up):
    """Max の座標(右手系・Z 上・正面 -Y)を PMX(左手系・Y 上・正面 -Z)へ。"""
    if up == "y":    # Y 上・正面 +Z の書き出しにも対応
        rows = ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, -1.0))
    else:
        rows = ((1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, 1.0, 0.0))
    return (rows[0] + (0.0,)) + (rows[1] + (0.0,)) + (rows[2] + (0.0,)) + (0.0, 0.0, 0.0, 1.0)


def _topological(bones):
    children = {}
    roots = []
    for index, bone in enumerate(bones):
        if 0 <= bone.parent < len(bones) and bone.parent != index:
            children.setdefault(bone.parent, []).append(index)
        else:
            roots.append(index)
    order = []
    stack = list(reversed(roots))
    while stack:
        index = stack.pop()
        order.append(index)
        stack.extend(reversed(children.get(index, [])))
    return order, children


def _detect_yaw(positions, base_matrix):
    toes, ankles = [], []
    table = bonenames.build_name_table(list(positions))
    for name, mmd in table.items():
        point = mat4.transform_point(base_matrix, positions[name])
        if mmd.endswith("つま先"):
            toes.append(point)
        elif mmd.endswith("足首"):
            ankles.append(point)
    if not toes or not ankles:
        return 0.0
    toe_z = sum(p[2] for p in toes) / len(toes)
    ankle_z = sum(p[2] for p in ankles) / len(ankles)
    return 180.0 if toe_z > ankle_z + 1e-4 else 0.0


def convert_tkm(tkm_path, output_path, options=None, log=None, skeleton_path=None,
                skeleton_only=False, given=()):
    """tkm(+ tks)を PMX へ変換して ConvertResult を返す。

    skeleton_only: メッシュを読まず骨格だけで組み立てる(tka の変換用)
    given: 一緒に渡された tks(skeleton_path が無いとき、この中から骨格を選ぶ)
    """
    options = options or converter.ConvertOptions()
    log = log or (lambda message: None)
    meshes = [] if skeleton_only else read_tkm(tkm_path)
    if not skeleton_path and tkm_path:
        skeleton_path = find_skeleton_for_model(tkm_path, given, loose=is_skinned(meshes))
    bones_tk = read_tks(skeleton_path) if skeleton_path else []
    if not bones_tk and skeleton_only:
        raise ValueError("骨格(tks)が見つかりません")
    if not bones_tk and is_skinned(meshes):
        # 骨格なしで変換するとボーン 1 本の置物になり、モーションが当たらない
        name = os.path.splitext(os.path.basename(tkm_path))[0] + ".tks"
        raise ValueError(
            "骨格のファイル %s が見つかりません。tkm にはボーンが入っておらず、骨格は同じ名前の "
            ".tks に入っています。%s を tkm と同じフォルダに置くか、tkm と一緒に"
            "ドロップしてください(--skeleton でも指定できます)" % (name, name))
    needed = max((index + 1 for mesh in meshes for vertex in mesh.vertices
                  for weight, index in zip(vertex[3], vertex[4]) if weight > 0.0), default=0)
    if bones_tk and len(bones_tk) < needed:
        raise ValueError("骨格 %s のボーンは %d 本ですが、tkm は %d 本目まで使っています。"
                         "別のモデルの骨格です。正しい .tks を --skeleton で指定してください"
                         % (skeleton_path, len(bones_tk), needed))
    if skeleton_path:
        log("  骨格: %s (%d 本)" % (skeleton_path, len(bones_tk)))

    model = pmxfile.PmxModel()
    result = converter.ConvertResult(model)
    result.source_meshes = len(meshes)

    if options.scale is not None:
        scale = float(options.scale)
    elif options.unit == "raw":
        scale = 1.0
    else:   # 1 単位 = 1cm とみなす
        scale = 0.01 * (converter.MMD_UNITS_PER_METER if options.unit == "mmd" else 1.0)
    axis = _axis_matrix(getattr(options, "tk_up", "z"))
    base = mat4.multiply(mat4.scaling(scale, scale, scale), axis)
    positions = {bone.name: mat4.get_translation(bone.bind) for bone in bones_tk}
    if options.fit == "auto" and options.scale is None and positions:
        factor, leg = converter.fit_factor_from_positions(positions, base, options.leg_length)
        if factor is not None:
            scale *= factor
            base = mat4.multiply(mat4.scaling(scale, scale, scale), axis)
            log("  体格合わせ: 脚の長さ %.2f → %.2f (×%.3f)" % (leg, options.leg_length, factor))
    yaw = _detect_yaw(positions, base) if options.yaw == "auto" else float(options.yaw)
    matrix = mat4.multiply(mat4.rotation_y(math.radians(yaw)), base)
    linear = motion._linear_part(matrix)
    result.scale, result.yaw, result.matrix = scale, yaw, matrix
    result.mirrored = (converter._determinant3(matrix) < 0.0) != bool(options.flip_faces)

    # ---- ボーン ----
    order, children = _topological(bones_tk)
    name_table = {}
    if options.use_mmd_bone_names and bones_tk:
        extra = bonenames.load_bone_map(options.bone_map_path) if options.bone_map_path else None
        name_table = bonenames.build_name_table([bone.name for bone in bones_tk], extra)
    result.renamed_bones = sum(1 for v in name_table.values() if any(ord(c) > 127 for c in v))
    pmx_index = {}
    for tk_index in order:
        bone_tk = bones_tk[tk_index]
        parent = pmx_index.get(bone_tk.parent, -1)
        bone = pmxfile.PmxBone(name_table.get(bone_tk.name, bone_tk.name), bone_tk.name,
                               mat4.transform_point(matrix, mat4.get_translation(bone_tk.bind)),
                               parent)
        bone.flags = converter._bone_flags(parent >= 0, bone.name)
        pmx_index[tk_index] = len(model.bones)
        model.bones.append(bone)
        result.bone_sources[id(bone)] = tk_index
        result.bind_globals[tk_index] = bone_tk.bind
    for tk_index in order:
        bone = model.bones[pmx_index[tk_index]]
        kids = [pmx_index[c] for c in children.get(tk_index, ())
                if model.bones[pmx_index[c]].position != bone.position]
        if kids:
            bone.flags |= pmxfile.BONE_TAIL_IS_INDEX
            bone.tail_index = kids[0]
        elif bone.parent_index >= 0:
            parent_position = model.bones[bone.parent_index].position
            direction = mat4.normalize(tuple(bone.position[i] - parent_position[i]
                                             for i in range(3)))
            length = math.dist(bone.position, parent_position) * 0.5
            bone.tail_offset = tuple(d * length for d in direction)
    if not model.bones:
        model.bones.append(pmxfile.PmxBone("センター", "center", (0.0, 0.0, 0.0), -1))
        model.bones[0].flags = converter._bone_flags(False, "センター")

    # ---- メッシュ・材質 ----
    output_dir = os.path.dirname(os.path.abspath(output_path)) or "."
    context = converter._TextureContext(tkm_path or skeleton_path, output_dir, options)
    for mesh_number, mesh in enumerate(meshes):
        base_index = len(model.vertices)
        for position, normal, uv, weights, indices in mesh.vertices:
            influences = [(pmx_index[i], w) for i, w in zip(indices, weights)
                          if i >= 0 and w > 1e-6 and i in pmx_index]
            influences.sort(key=lambda item: -item[1])
            influences = influences[:options.max_weights] or [(0, 1.0)]
            total = sum(w for _i, w in influences) or 1.0
            vertex = pmxfile.PmxVertex(
                mat4.transform_point(matrix, position),
                mat4.normalize(mat4.transform_vector(linear, normal)) if any(normal)
                else (0.0, 1.0, 0.0),
                (uv[0], uv[1]),   # エンジンと同じく上が v=0 なので PMX でもそのまま使う
                [i for i, _w in influences], [w / total for _i, w in influences])
            if len(vertex.bones) > 2:
                while len(vertex.bones) < 4:
                    vertex.bones.append(vertex.bones[0])
                    vertex.weights.append(0.0)
            model.vertices.append(vertex)
        for material_number, group in enumerate(mesh.groups):
            names = mesh.materials[material_number] if material_number < len(mesh.materials) \
                else [""] * 5
            albedo = names[0]
            label = os.path.splitext(os.path.basename(albedo))[0] if albedo \
                else "material%d_%d" % (mesh_number, material_number)
            material = pmxfile.PmxMaterial(label, label)
            material.diffuse = (0.8, 0.8, 0.8, 1.0)
            material.ambient = (0.4, 0.4, 0.4)
            material.texture_index = _texture_index(albedo, context, result, label)
            if options.edge:
                material.flags |= pmxfile.MATERIAL_EDGE
            else:
                material.flags &= ~pmxfile.MATERIAL_EDGE
            if options.double_sided:
                material.flags |= pmxfile.MATERIAL_DOUBLE_SIDED
            if options.mmd_look:
                converter._apply_mmd_look(material, albedo)
            faces = []
            for t in range(0, len(group) - 2, 3):
                a, b, c = (base_index + group[t], base_index + group[t + 1],
                           base_index + group[t + 2])
                faces.append((a, c, b) if result.mirrored else (a, b, c))
            material.surface_count = len(faces) * 3
            model.materials.append(material)
            model.faces.extend(faces)
    model.textures = context.textures
    if not options.skip_textures:
        uvcheck.fix_swapped_textures(model, output_dir, log)

    converter.finalize(model, result, options, tkm_path or skeleton_path, scale, yaw, log)
    return result


def _texture_index(albedo, context, result, material_name):
    """tkm の材質が参照するアルベド画像(エンジンは同名の .dds を読む)を探して登録する。"""
    if not albedo or context.options.skip_textures:
        return -1
    pool = context.pool
    source = pool.find_name(albedo) or pool.find_stem(albedo)
    method = "同名ファイル" if source and os.path.basename(source).lower() == \
        os.path.basename(albedo).lower() else "拡張子違い"
    if source is None:
        result.missing_textures.append(albedo)
        result.texture_assignments.append((material_name, None, "見つからない: " + albedo, None))
        return -1
    entry = converter._place_texture(source, context, result).replace("/", "\\")
    result.texture_assignments.append((material_name, entry, method, source))
    if entry not in context.index:
        context.index[entry] = len(context.textures)
        context.textures.append(entry)
    return context.index[entry]


# --------------------------------------------------------------------------
# tka -> VMD
# --------------------------------------------------------------------------

def _decompose(matrix):
    translation = mat4.get_translation(matrix)
    scale = tuple(math.sqrt(matrix[c] ** 2 + matrix[4 + c] ** 2 + matrix[8 + c] ** 2)
                  for c in range(3))
    return translation, posing.q_from_matrix(matrix), scale


def _compose(translation, rotation, scale):
    x, y, z, w = rotation
    r = (1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w),
         2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w),
         2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y))
    return (r[0] * scale[0], r[1] * scale[1], r[2] * scale[2], translation[0],
            r[3] * scale[0], r[4] * scale[1], r[5] * scale[2], translation[1],
            r[6] * scale[0], r[7] * scale[1], r[8] * scale[2], translation[2],
            0.0, 0.0, 0.0, 1.0)


def convert_tka(tka_path, options=None, log=None, skeleton_path=None, given=()):
    """tka を VMD へ変換する。(VmdMotion, MotionReport, 骨格の ConvertResult) を返す。

    given: 一緒に渡された tks(ボーン数が合うものを優先して使う)
    """
    options = options or motion.MotionOptions()
    log = log or (lambda message: None)
    keys = read_tka(tka_path)
    skeleton_path = skeleton_path or find_skeleton_for_animation(tka_path, keys, given)
    if skeleton_path is None:
        needed = max((key[0] for key in keys), default=-1) + 1
        raise ValueError("ボーン数 %d の骨格(tks)が見つかりません。--skeleton で指定してください"
                         % needed)
    result = convert_tkm(None, tka_path, options.model, log, skeleton_path, skeleton_only=True)
    bones_tk = read_tks(skeleton_path)
    order, _children = _topological(bones_tk)

    report = motion.MotionReport()
    report.take_name = os.path.splitext(os.path.basename(tka_path))[0]
    tracks = {}
    for bone, time, matrix in keys:
        if bone < len(bones_tk):
            tracks.setdefault(bone, []).append((time, _decompose(matrix)))
    for track in tracks.values():
        track.sort(key=lambda item: item[0])
    rest_local = {}
    for index, bone in enumerate(bones_tk):
        parent = bone.parent
        rest_local[index] = bone.bind if not 0 <= parent < len(bones_tk) else mat4.multiply(
            mat4.inverse(bones_tk[parent].bind), bone.bind)
    duration = max((key[1] for key in keys), default=0.0)
    frame_count = int(round(duration * options.fps)) + 1

    def local_at(index, time):
        track = tracks.get(index)
        if not track:
            return rest_local[index]
        if time <= track[0][0]:
            return _compose(*track[0][1])
        if time >= track[-1][0]:
            return _compose(*track[-1][1])
        low, high = 0, len(track) - 1
        while high - low > 1:
            middle = (low + high) // 2
            if track[middle][0] <= time:
                low = middle
            else:
                high = middle
        (t0, (p0, r0, s0)), (t1, (p1, r1, s1)) = track[low], track[high]
        f = (time - t0) / (t1 - t0) if t1 > t0 else 0.0
        return _compose(tuple(a + (b - a) * f for a, b in zip(p0, p1)),
                        posing.q_slerp(r0, r1, f),
                        tuple(a + (b - a) * f for a, b in zip(s0, s1)))

    def sample(frame):
        time = frame / float(options.fps)
        world = {}
        for index in order:
            local = local_at(index, time)
            parent = bones_tk[index].parent
            world[index] = local if parent not in world else mat4.multiply(world[parent], local)
        return world

    data = motion.build_motion(result, frame_count, sample, options, report)
    return data, report, result
