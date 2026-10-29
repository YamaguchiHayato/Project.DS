# -*- coding: utf-8 -*-
"""FBX シーンを PMX モデルへ変換する。

座標系の違い(FBX: 右手系 / 軸はファイル指定, PMX: 左手系 Y-up)、単位、
UV 原点、面の向き、スキンウェイト、モーフをここで吸収する。
"""

from __future__ import annotations

import math
import os
import shutil

import bonenames
import facial
import fbxscene
import mat4
import mmdize
import physics
import pmxfile
import texconv
import texmatch
import uvcheck

# MMD の 1 単位はおよそ 8cm (身長 158cm の標準モデルで約 20 単位)
MMD_UNITS_PER_METER = 12.5
# 体格合わせの目標にする脚(股関節 → 足首)の長さ。身長 158cm の人の脚
# (身長の約 48% ≒ 76cm)を 1 単位 8cm で換算した値
STANDARD_LEG_LENGTH = 9.5


# モーフ名から表示パネルを推測するためのキーワード
_MORPH_PANELS = (
    (2, ("eye", "blink", "blk", "目", "まばたき", "ウィンク")),
    (3, ("mouth", "mth", "lip", "jaw", "口", "あ", "い", "う", "え", "お")),
    (1, ("brow", "brw", "blw", "eyebrow", "眉", "まゆ")),
)


class ConvertOptions:
    """変換時の指定。"""

    def __init__(self):
        self.scale = None             # 明示スケール(None なら unit から決める)
        self.unit = "mmd"             # mmd / meter / raw
        self.yaw = "auto"             # auto もしくは度数
        self.max_weights = 4
        self.use_mmd_bone_names = True
        self.bone_map_path = None
        self.model_name = None
        self.model_name_en = None
        self.comment = None
        self.copy_textures = False
        self.convert_textures = True    # DDS など MMD が読めない形式を PNG へ
        self.texture_files = []         # 一緒に渡された画像(割り当ての候補)
        self.texture_dirs = []          # 画像を探すフォルダ
        self.texture_map_path = None    # {"材質名": "画像"} の割り当て表
        self.texture_format = "png"
        self.texture_max_size = 0
        self.include_morphs = True
        self.edge = True
        self.double_sided = False
        self.flip_faces = False       # 面の表裏が逆だったときの救済
        self.rig = "mmd"              # mmd: 標準ボーン構造・足IKを整える / none: そのまま
        self.arm_angle = "auto"       # auto / off / 度数
        self.rename_morphs = True     # 表情モーフ名を あ・い・う… へ寄せる
        self.morph_map_path = None
        self.winks = True             # まばたきを左右に分けてウィンクを作る
        self.auto_face = True         # 表情モーフが無ければ目・口のモーフを顔の形から作る
        self.semi_standard = True     # 腕捩・手捩・肩P・腰・足IK親・足D 系を作る
        self.fit = "auto"             # auto: 脚の長さを標準体格に合わせる / none
        self.leg_length = STANDARD_LEG_LENGTH
        self.bust_physics = True      # 胸のボーンにも物理を付ける(揺らす)
        self.skip_textures = False    # テクスチャを探さない(モーション変換で骨格だけ欲しいとき)
        self.tk_up = "z"              # ゲームの tkm / tka の上方向の軸(3ds Max の書き出しは z)
        self.physics = "auto"         # auto: 揺れものに剛体・ジョイントを付ける / none
        # 物理を読み込む側: k2engine(ゲームのエンジン。ボーン追従の剛体を 75% に縮めて使う)/ mmd
        self.physics_target = "k2engine"
        self.mmd_look = True          # 材質を MMD 向けの値へ調整する


# k2Engine 向けに当たり判定を大きめに書き出したことを示すコメント(pmxpreview が読む)
PHYSICS_K2ENGINE_NOTE = "物理: k2Engine 向け(ボーン追従の剛体は 75% に縮めて使う前提で大きめ)"


def _warn(result, message):
    """同じ警告を何度も積まないようにする。"""
    if message not in result.warnings:
        result.warnings.append(message)


class ConvertResult:
    """変換結果と統計。"""

    def __init__(self, model):
        self.model = model
        self.scale = 1.0
        self.yaw = 0.0
        self.mirrored = False
        self.source_meshes = 0
        self.renamed_bones = 0
        self.missing_textures = []
        self.converted_textures = []
        self.texture_assignments = []  # [(材質名, PMX に書いたパス, 見つけ方, 元画像)]
        # モーション変換(motion.py)が使う情報
        self.scene = None
        self.matrix = mat4.IDENTITY    # FBX 座標 -> PMX 座標(軸・縮尺・Y 回転)
        self.bone_sources = {}         # id(PmxBone) -> 元の FBX Model id
        self.bind_globals = {}         # FBX Model id -> バインド時のグローバル行列
        self.rig = None               # mmdize.RigReport
        self.physics = None           # physics.PhysicsReport
        self.renamed_morphs = 0
        self.winks = 0
        self.face = None              # facial.FaceReport(表情モーフを自動で作ったとき)
        self.warnings = []


def _quantize(value, digits=4):
    return int(round(value * (10 ** digits)))


def _axis_matrix(scene):
    """FBX の軸設定から PMX(左手系 Y-up, 正面 -Z) への変換行列を作る。"""
    right, up, front = scene.axis_vectors()
    back = (-front[0], -front[1], -front[2])
    return (
        right[0], right[1], right[2], 0.0,
        up[0], up[1], up[2], 0.0,
        back[0], back[1], back[2], 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


def _determinant3(m):
    return (m[0] * (m[5] * m[10] - m[6] * m[9])
            - m[1] * (m[4] * m[10] - m[6] * m[8])
            + m[2] * (m[4] * m[9] - m[5] * m[8]))


def _resolve_scale(scene, options):
    if options.scale is not None:
        return float(options.scale)
    meters = scene.meters_per_unit()
    if options.unit == "raw":
        return 1.0
    if options.unit == "meter":
        return meters
    return meters * MMD_UNITS_PER_METER


# --------------------------------------------------------------------------
# ボーン
# --------------------------------------------------------------------------

def _collect_bone_models(scene, meshes):
    """ボーンとして扱う Model を集める。"""
    bind_globals = {}
    bone_ids = set()

    for mesh in meshes:
        for cluster in mesh.clusters:
            bone_ids.add(cluster.bone.id)
            if cluster.transform_link is not None:
                bind_globals[cluster.bone.id] = cluster.transform_link

    for model in scene.models("LimbNode"):
        bone_ids.add(model.id)

    # 祖先をたどってスケルトンの根まで含める
    for bone_id in list(bone_ids):
        model = scene.objects.get(bone_id)
        parent = scene.parent_model(model.id) if model else None
        while parent is not None and parent.id not in bone_ids:
            bone_ids.add(parent.id)
            parent = scene.parent_model(parent.id)

    # メッシュノード自身はボーンにしない(祖先として必要な場合のみ残る)
    mesh_model_ids = {model.id for model in scene.models("Mesh")}
    bone_ids -= mesh_model_ids

    bind_pose = scene.bind_pose_matrices()
    for bone_id in bone_ids:
        if bone_id not in bind_globals:
            model = scene.objects.get(bone_id)
            if bone_id in bind_pose:
                bind_globals[bone_id] = bind_pose[bone_id]
            elif model is not None:
                bind_globals[bone_id] = scene.global_matrix(model)

    # 親 -> 子の順に並べる
    ordered = []
    children = {}
    roots = []
    for bone_id in bone_ids:
        model = scene.objects[bone_id]
        parent = scene.parent_model(bone_id)
        if parent is not None and parent.id in bone_ids:
            children.setdefault(parent.id, []).append(bone_id)
        else:
            roots.append(bone_id)

    def visit(bone_id):
        ordered.append(bone_id)
        for child_id in sorted(children.get(bone_id, ()),
                               key=lambda i: scene.objects[i].name):
            visit(child_id)

    for root_id in sorted(roots, key=lambda i: scene.objects[i].name):
        visit(root_id)
    return ordered, bind_globals, children


def _detect_yaw(scene, bone_ids, bind_globals, base_matrix):
    """つま先と足首の前後関係から正面向きを推測し、必要なら 180 度回す。"""
    toes = []
    ankles = []
    for bone_id in bone_ids:
        name = scene.objects[bone_id].name.lower()
        position = mat4.transform_point(
            base_matrix, mat4.get_translation(bind_globals[bone_id]))
        if "toe" in name or "つま先" in name:
            toes.append(position)
        elif "foot" in name or "ankle" in name or "足首" in name:
            ankles.append(position)
    if not toes or not ankles:
        return 0.0, ("つま先ボーンが無いため FBX の FrontAxis 指定をそのまま採用"
                     "(MMD で後ろを向く場合は --yaw 180 を指定)")
    toe_z = sum(p[2] for p in toes) / len(toes)
    ankle_z = sum(p[2] for p in ankles) / len(ankles)
    if toe_z > ankle_z + 1e-4:  # MMD の正面は -Z
        return 180.0, "つま先が +Z を向いていたため 180 度回転"
    return 0.0, "正面は -Z のままで良いと判断"


# MMD で移動させる前提のボーン
_TRANSLATABLE_BONES = ("全ての親", "センター", "グルーブ", "腰")


def _fit_factor(scene, bone_ids, bind_globals, base_matrix, target):
    """脚(足 → 足首)の長さを target に合わせる倍率を返す。脚が無ければ None。

    配布モーションのセンター移動や足ＩＫの位置は絶対値なので、体格が標準から
    大きく外れるとステップ幅や足の位置が合わない。そこで脚の長さを基準にそろえる。
    """
    positions = {scene.objects[bone_id].name: mat4.get_translation(bind_globals[bone_id])
                 for bone_id in bone_ids}
    return fit_factor_from_positions(positions, base_matrix, target)


def fit_factor_from_positions(positions, base_matrix, target):
    """{ボーン名: 元の座標} から、脚の長さを target に合わせる倍率を求める。"""
    table = bonenames.build_name_table(list(positions))
    by_mmd = {mmd: name for name, mmd in table.items()}
    lengths = []
    for side in ("左", "右"):
        leg = by_mmd.get(side + "足")
        ankle = by_mmd.get(side + "足首")
        if leg and ankle:
            a = mat4.transform_point(base_matrix, positions[leg])
            b = mat4.transform_point(base_matrix, positions[ankle])
            lengths.append(math.dist(a, b))
    if not lengths or target <= 0.0:
        return None, 0.0
    length = sum(lengths) / len(lengths)
    if length < 1e-6:
        return None, 0.0
    return target / length, length


def _bone_flags(has_parent, name=""):
    flags = pmxfile.BONE_ROTATABLE | pmxfile.BONE_VISIBLE | pmxfile.BONE_ENABLED
    if not has_parent or name in _TRANSLATABLE_BONES:
        flags |= pmxfile.BONE_TRANSLATABLE
    return flags


def _build_bones(scene, skeleton, matrix, options, result):
    """PMX のボーン配列と {FBX オブジェクト id: ボーン番号} を作る。"""
    bone_ids, bind_globals, children = skeleton
    index_of = {bone_id: index for index, bone_id in enumerate(bone_ids)}

    name_table = {}
    if options.use_mmd_bone_names:
        extra = bonenames.load_bone_map(options.bone_map_path) \
            if options.bone_map_path else None
        name_table = bonenames.build_name_table(
            [scene.objects[bone_id].name for bone_id in bone_ids], extra)
    result.renamed_bones = sum(1 for v in name_table.values() if any(ord(c) > 127 for c in v))

    bones = []
    for bone_id in bone_ids:
        model = scene.objects[bone_id]
        parent = scene.parent_model(bone_id)
        parent_index = index_of.get(parent.id, -1) if parent is not None else -1
        position = mat4.transform_point(
            matrix, mat4.get_translation(bind_globals[bone_id]))
        bone = pmxfile.PmxBone(
            name_table.get(model.name, model.name) or ("bone%d" % len(bones)),
            model.name,
            position,
            parent_index,
        )
        bone.flags = _bone_flags(parent_index >= 0, bone.name)
        bones.append(bone)
        result.bone_sources[id(bone)] = bone_id   # モーション変換で元の FBX ボーンを引く

    # 接続先(末端)の設定
    for bone_id in bone_ids:
        index = index_of[bone_id]
        bone = bones[index]
        child_ids = [i for i in children.get(bone_id, ()) if i in index_of]
        child_ids = [i for i in child_ids
                     if bones[index_of[i]].position != bone.position]
        if child_ids:
            bone.flags |= pmxfile.BONE_TAIL_IS_INDEX
            bone.tail_index = index_of[child_ids[0]]
        elif bone.parent_index >= 0:
            parent_position = bones[bone.parent_index].position
            direction = mat4.normalize(tuple(
                bone.position[i] - parent_position[i] for i in range(3)))
            length = math.dist(bone.position, parent_position) * 0.5
            bone.tail_offset = tuple(d * length for d in direction)

    if not bones:
        bones.append(pmxfile.PmxBone("センター", "center", (0.0, 0.0, 0.0), -1))
        bones[0].flags = _bone_flags(False, "センター")
    return bones, index_of


def _default_bone_index(scene, model, bone_index):
    """スキンが無いメッシュを乗せるボーン番号を探す。"""
    parent = scene.parent_model(model.id)
    while parent is not None:
        if parent.id in bone_index:
            return bone_index[parent.id]
        parent = scene.parent_model(parent.id)
    return 0


# --------------------------------------------------------------------------
# マテリアル・テクスチャ
# --------------------------------------------------------------------------

def _material_color(material, name, default):
    values = material.prop(name)
    if values and len(values) >= 3:
        return (float(values[0]), float(values[1]), float(values[2]))
    return default


class _TextureContext:
    """1 回の変換で使うテクスチャまわりの状態。"""

    def __init__(self, fbx_path, output_dir, options):
        self.fbx_path = fbx_path
        self.output_dir = output_dir
        self.options = options
        self.pool = texmatch.build_pool(fbx_path, options.texture_files, options.texture_dirs)
        self.texture_map = texmatch.load_texture_map(options.texture_map_path) \
            if options.texture_map_path else {}
        self.textures = []      # PMX のテクスチャ表
        self.index = {}         # テクスチャ表に書くパス -> 番号
        self.placed = {}        # 元画像 -> テクスチャ表に書くパス


def _resolve_texture(scene, material, mesh_name, context, result):
    """マテリアルに使う画像を探して (元画像のパス or None, 見つけ方) を返す。"""
    name = material.name
    if name in context.texture_map:
        value = context.texture_map[name]
        if value is None:
            return None, "割り当て表でなし"
        path = value if os.path.isfile(value) else (
            context.pool.find_name(value) or context.pool.find_stem(value))
        if path:
            return path, "割り当て表"
        _warn(result, "割り当て表の画像が見つかりません: %s → %s" % (name, value))

    texture = fbxscene.find_diffuse_texture(scene, material)
    relative = fbxscene.texture_path(texture) if texture is not None else None
    if relative:
        path = fbxscene.resolve_texture_file(context.fbx_path, relative)
        if path:
            return path, "参照どおり"
        path = context.pool.find_name(relative)
        if path:
            return path, "同名ファイル"
        path = context.pool.find_stem(relative)
        if path:
            return path, "拡張子違い"

    if texture is not None:
        embedded = texmatch.embedded_image(scene, texture)
        if embedded is not None:
            return texmatch.write_embedded(context.output_dir, *embedded), "埋め込み"

    keys = [relative, texture.name if texture is not None else None, name, mesh_name]
    path, _score = context.pool.guess(keys)
    if path:
        return path, "名前から推定"
    path = context.pool.single_explicit_diffuse()
    if path:
        return path, "唯一の候補"
    if relative:
        result.missing_textures.append(relative)
        return None, "見つからない: %s" % os.path.basename(relative)
    return None, "なし"


def _texture_entry(scene, material, mesh_name, context, result):
    """マテリアルのテクスチャを PMX のテクスチャ表へ登録し番号を返す。"""
    if context.options.skip_textures:
        return -1
    source, method = _resolve_texture(scene, material, mesh_name, context, result)
    if source is None:
        result.texture_assignments.append((material.name, None, method, None))
        return -1

    entry = _place_texture(source, context, result).replace("/", "\\")
    result.texture_assignments.append((material.name, entry, method, source))
    if entry not in context.index:
        context.index[entry] = len(context.textures)
        context.textures.append(entry)
    return context.index[entry]


def _copy_destination(source, output_dir):
    """コピー先を決める。同名で中身の違うファイルがあれば別名にする。"""
    name = os.path.basename(source)
    stem, extension = os.path.splitext(name)
    destination = os.path.join(output_dir, name)
    number = 1
    while os.path.isfile(destination) and not os.path.samefile(destination, source):
        if os.path.getsize(destination) == os.path.getsize(source):
            with open(destination, "rb") as a, open(source, "rb") as b:
                if a.read() == b.read():
                    return destination
        destination = os.path.join(output_dir, "%s_%d%s" % (stem, number, extension))
        number += 1
    return destination


def _place_texture(source, context, result):
    """画像を出力先へ配置し、PMX に書き込む相対パスを返す。

    - MMD が読めない形式(DDS など)は PNG / TGA へ変換して出力先へ置く
    - 出力先の外にある画像は出力先へコピーする(PMX のフォルダだけで完結させる)
    - --copy-textures なら出力先の中にあってもコピーする
    """
    if source in context.placed:
        return context.placed[source]
    options = context.options
    output_dir = context.output_dir
    entry = None

    if options.convert_textures and texconv.needs_conversion(source):
        try:
            converted = texconv.convert_for_mmd(
                source, output_dir, options.texture_format, options.texture_max_size)
            record = (os.path.basename(source), os.path.basename(converted))
            if record not in result.converted_textures:
                result.converted_textures.append(record)
            entry = os.path.basename(converted)
        except Exception as error:
            _warn(result, "テクスチャを変換できませんでした: %s (%s)"
                  % (os.path.basename(source), error))

    if entry is None:
        try:
            relative = os.path.relpath(source, output_dir)
        except ValueError:  # ドライブが違うと相対パスにできない
            relative = None
        if options.copy_textures or relative is None or relative.startswith(".."):
            destination = _copy_destination(source, output_dir)
            if not os.path.isfile(destination) or not os.path.samefile(destination, source):
                shutil.copyfile(source, destination)
            entry = os.path.basename(destination)
        else:
            entry = relative

    if texconv.needs_conversion(entry):
        _warn(result, "%s は MMD が読めない形式です(--no-convert-textures を外すと"
                      "PNG へ変換します)。" % os.path.basename(entry))
    context.placed[source] = entry
    return entry


def _build_material(scene, fbx_material, mesh_name, options):
    """FBX マテリアルから PMX マテリアルを作る(テクスチャ番号は後で設定)。"""
    if fbx_material is None:
        material = pmxfile.PmxMaterial(mesh_name, mesh_name)
        diffuse = (0.8, 0.8, 0.8)
        alpha = 1.0
    else:
        material = pmxfile.PmxMaterial(fbx_material.name or mesh_name,
                                       fbx_material.name or mesh_name)
        diffuse = _material_color(fbx_material, "DiffuseColor", (0.8, 0.8, 0.8))
        factor = fbx_material.prop_float("DiffuseFactor", 1.0)
        diffuse = tuple(min(1.0, c * factor) for c in diffuse)
        alpha = 1.0 - fbx_material.prop_float("TransparencyFactor", 0.0)
        if "Opacity" in (fbx_material.props or {}):
            alpha = fbx_material.prop_float("Opacity", 1.0)

    material.diffuse = (diffuse[0], diffuse[1], diffuse[2], max(0.0, min(1.0, alpha)))
    if fbx_material is not None:
        specular = _material_color(fbx_material, "SpecularColor", (0.0, 0.0, 0.0))
        specular_factor = fbx_material.prop_float("SpecularFactor", 1.0)
        material.specular = tuple(min(1.0, c * specular_factor) for c in specular)
        material.specular_power = max(1.0, min(100.0,
                                               fbx_material.prop_float("ShininessExponent", 5.0)))
        ambient = fbx_material.prop("AmbientColor")
        if ambient and len(ambient) >= 3:
            material.ambient = tuple(min(1.0, float(c)) for c in ambient[:3])
        else:
            material.ambient = tuple(c * 0.5 for c in diffuse)
    else:
        material.ambient = tuple(c * 0.5 for c in diffuse)

    flags = pmxfile.MATERIAL_GROUND_SHADOW | pmxfile.MATERIAL_SELF_SHADOW_MAP | \
        pmxfile.MATERIAL_SELF_SHADOW
    if options.edge:
        flags |= pmxfile.MATERIAL_EDGE
    if options.double_sided:
        flags |= pmxfile.MATERIAL_DOUBLE_SIDED
    material.flags = flags
    return material


# --------------------------------------------------------------------------
# 頂点・面
# --------------------------------------------------------------------------

def _mesh_weights(mesh, bone_index, fallback_bone, max_weights):
    """制御点ごとのウェイトを [(ボーン番号, 重み), ...] で返す。"""
    table = {}
    for cluster in mesh.clusters:
        index = bone_index.get(cluster.bone.id)
        if index is None:
            continue
        indexes = cluster.indexes
        weights = cluster.weights
        for i in range(min(len(indexes), len(weights))):
            weight = float(weights[i])
            if weight <= 1e-6:
                continue
            table.setdefault(int(indexes[i]), []).append((index, weight))

    result = {}
    for control_point, entries in table.items():
        entries.sort(key=lambda item: -item[1])
        entries = entries[:max_weights]
        total = sum(weight for _index, weight in entries)
        if total <= 1e-9:
            result[control_point] = [(fallback_bone, 1.0)]
        else:
            result[control_point] = [(index, weight / total) for index, weight in entries]
    return result


def _append_vertex(model, cache, key, position, normal, uv, influences):
    index = cache.get(key)
    if index is not None:
        return index
    bones = [influence[0] for influence in influences]
    weights = [influence[1] for influence in influences]
    if len(bones) > 2:
        while len(bones) < 4:
            bones.append(0)
            weights.append(0.0)
    index = len(model.vertices)
    model.vertices.append(pmxfile.PmxVertex(position, normal, uv, bones, weights))
    cache[key] = index
    return index


def _morph_panel(name):
    lowered = name.lower()
    for panel, keywords in _MORPH_PANELS:
        if any(keyword in lowered for keyword in keywords):
            return panel
    return 4


# --------------------------------------------------------------------------
# 本体
# --------------------------------------------------------------------------

def convert(fbx_path, output_path, options=None, log=None, skeleton_only=False):
    """FBX を読み込み PmxModel を組み立てて ConvertResult を返す。

    skeleton_only: メッシュが無くても骨格だけで変換する(モーション変換用)
    """
    options = options or ConvertOptions()
    log = log or (lambda message: None)

    scene = fbxscene.FbxScene(fbx_path)
    meshes = fbxscene.build_meshes(scene)
    if not meshes and not skeleton_only:
        raise ValueError("変換できるメッシュが FBX に含まれていません: %s" % fbx_path)

    model = pmxfile.PmxModel()
    result = ConvertResult(model)
    result.scene = scene
    result.source_meshes = len(meshes)

    scale = _resolve_scale(scene, options)
    axis = _axis_matrix(scene)
    base_matrix = mat4.multiply(mat4.scaling(scale, scale, scale), axis)

    skeleton = _collect_bone_models(scene, meshes)
    bone_ids, bind_globals = skeleton[0], skeleton[1]
    if options.fit == "auto" and options.scale is None:
        factor, leg = _fit_factor(scene, bone_ids, bind_globals, base_matrix,
                                  options.leg_length)
        if factor is not None:
            scale *= factor
            base_matrix = mat4.multiply(mat4.scaling(scale, scale, scale), axis)
            log("  体格合わせ: 脚の長さ %.2f → %.2f (×%.3f)"
                % (leg, options.leg_length, factor))
    result.scale = scale
    if options.yaw == "auto":
        yaw, reason = _detect_yaw(scene, bone_ids, bind_globals, base_matrix)
        log("  正面判定: %s" % reason)
    else:
        yaw = float(options.yaw)
    result.yaw = yaw
    matrix = mat4.multiply(mat4.rotation_y(math.radians(yaw)), base_matrix)
    result.mirrored = (_determinant3(matrix) < 0.0) != bool(options.flip_faces)
    result.matrix = matrix
    result.bind_globals = bind_globals

    bones, bone_index = _build_bones(scene, skeleton, matrix, options, result)
    model.bones = bones

    output_dir = os.path.dirname(os.path.abspath(output_path)) or "."
    texture_context = _TextureContext(fbx_path, output_dir, options)
    material_entries = {}   # キー -> [PmxMaterial, 面リスト]
    vertex_cache = {}
    morphs = []

    for mesh in meshes:
        mesh_model = mesh.model
        mesh_matrix = mat4.multiply(matrix, mesh.bind_matrix)
        fallback_bone = _default_bone_index(scene, mesh_model, bone_index) \
            if mesh_model is not None else 0
        weights = _mesh_weights(mesh, bone_index, fallback_bone, options.max_weights)
        control_point_vertices = {}

        corner_index = 0
        for polygon_index, polygon in enumerate(mesh.polygons):
            corner_vertices = []
            for control_point in polygon:
                position = mat4.transform_point(
                    mesh_matrix, mesh.control_points[control_point])
                if mesh.corner_normals is not None:
                    normal = mat4.normalize(mat4.transform_vector(
                        mesh_matrix, mesh.corner_normals[corner_index]))
                else:
                    normal = (0.0, 1.0, 0.0)
                if mesh.corner_uvs is not None:
                    raw_uv = mesh.corner_uvs[corner_index]
                    uv = (raw_uv[0], 1.0 - raw_uv[1])  # PMX の UV 原点は左上
                else:
                    uv = (0.0, 0.0)
                influences = weights.get(control_point) or [(fallback_bone, 1.0)]
                key = (id(mesh), control_point,
                       _quantize(normal[0]), _quantize(normal[1]), _quantize(normal[2]),
                       _quantize(uv[0], 5), _quantize(uv[1], 5))
                vertex = _append_vertex(model, vertex_cache, key, position, normal,
                                        uv, influences)
                corner_vertices.append(vertex)
                control_point_vertices.setdefault(control_point, set()).add(vertex)
                corner_index += 1

            material_number = mesh.polygon_materials[polygon_index] \
                if mesh.polygon_materials else 0
            fbx_material = mesh.materials[material_number] \
                if 0 <= material_number < len(mesh.materials) else None
            key = fbx_material.id if fbx_material is not None else ("mesh", mesh.name)
            entry = material_entries.get(key)
            if entry is None:
                material = _build_material(scene, fbx_material, mesh.name, options)
                if fbx_material is not None:
                    material.texture_index = _texture_entry(
                        scene, fbx_material, mesh.name, texture_context, result)
                if options.mmd_look:
                    _apply_mmd_look(material, mesh.name)
                entry = [material, []]
                material_entries[key] = entry

            for i in range(1, len(corner_vertices) - 1):
                triangle = (corner_vertices[0], corner_vertices[i], corner_vertices[i + 1])
                if result.mirrored:  # 鏡像変換で表裏が反転するため巻き順を戻す
                    triangle = (triangle[0], triangle[2], triangle[1])
                entry[1].append(triangle)

        if options.include_morphs:
            for shape in mesh.shapes:
                offsets = []
                for index, delta in zip(shape.indexes, shape.deltas):
                    moved = mat4.transform_vector(mesh_matrix, delta)
                    for vertex in sorted(control_point_vertices.get(index, ())):
                        offsets.append((vertex, moved))
                if offsets:
                    morph = pmxfile.PmxMorph(shape.name, shape.name,
                                             _morph_panel(shape.name))
                    morph.offsets = offsets
                    morphs.append(morph)

    for material, faces in material_entries.values():
        material.surface_count = len(faces) * 3
        model.materials.append(material)
        model.faces.extend(faces)

    model.textures = texture_context.textures
    model.morphs = morphs
    if not options.skip_textures:
        uvcheck.fix_swapped_textures(model, texture_context.output_dir, log)
    finalize(model, result, options, fbx_path, scale, yaw, log)
    return result


def finalize(model, result, options, source_path, scale, yaw, log):
    """メッシュと骨格を作った後の共通処理(名前・モーフ・MMD 化・物理・検査)。"""
    name = options.model_name or os.path.splitext(os.path.basename(source_path))[0]
    model.name = name
    model.name_en = options.model_name_en or name
    model.comment = options.comment if options.comment is not None else (
        "%s から fbx2pmx で変換\r\n"
        "元ファイル: %s\r\n"
        "スケール: %.6g 倍 / Y 回転: %.0f 度" % (
            os.path.basename(source_path), os.path.basename(source_path), scale, yaw))
    model.comment_en = "Converted from %s by fbx2pmx" % os.path.basename(source_path)

    if options.rename_morphs and model.morphs:
        extra = bonenames.load_bone_map(options.morph_map_path) \
            if options.morph_map_path else None
        result.renamed_morphs = mmdize.rename_morphs(model, extra)
    used = {morph.name for morph in model.morphs if bonenames.name_bytes(morph.name) <= bonenames.NAME_LIMIT}
    for morph in model.morphs:   # VMD の名前欄(15 バイト)に入るよう縮める
        if bonenames.name_bytes(morph.name) > bonenames.NAME_LIMIT:
            morph.name_en = morph.name_en or morph.name
            morph.name = bonenames.shorten(morph.name, used)
            used.add(morph.name)
    if options.auto_face and not model.morphs:
        result.face = facial.generate(model)
        for message in result.face.warnings:
            _warn(result, message)
    if options.winks and model.morphs:
        result.winks = mmdize.add_winks(model)

    if options.rig == "mmd" and options.use_mmd_bone_names and mmdize.is_humanoid(model):
        result.rig = mmdize.standardize(model, options.arm_angle, log, options.semi_standard)
        for message in result.rig.warnings:
            _warn(result, message)
    else:
        _build_display_frames(model)

    if options.physics == "auto":
        k2engine = options.physics_target == "k2engine"
        result.physics = physics.generate(
            model, bust=options.bust_physics,
            engine_scale=physics.K2ENGINE_FOLLOW_SCALE if k2engine else 1.0)
        if model.rigid_bodies and k2engine:
            # プレビューがエンジンと同じ縮小で当たり判定を再現できるよう、コメントに残す
            model.comment += "\r\n" + PHYSICS_K2ENGINE_NOTE
    _validate(model, result)


# 輪郭線を付けない材質(目まわり)と、両面描画にする材質(薄い布や髪の板ポリ)の目印
_NO_EDGE_WORDS = ("eye", "目", "瞳", "hitomi", "iris", "pupil", "highlight", "ハイライト",
                  "matuge", "まつげ", "lash")
_DOUBLE_SIDED_WORDS = ("hair", "髪", "ribbon", "リボン", "skirt", "スカート", "frill",
                       "フリル", "lace", "レース", "sode", "sleeve", "袖", "cape", "マント")


def _apply_mmd_look(material, mesh_name):
    """材質を MMD で自然に見える値へ寄せる。

    MMD の色はおおむね (拡散色 × 照明 0.6 + 環境色) × テクスチャ × トゥーン で
    決まるため、ゲーム用の値(環境色が黒など)のままだと暗く沈む。
    """
    text = (material.name + " " + (mesh_name or "")).lower()
    alpha = material.diffuse[3]
    if material.texture_index >= 0:
        material.diffuse = (1.0, 1.0, 1.0, alpha)  # 色はテクスチャに任せる
        material.ambient = (0.5, 0.5, 0.5)
    else:
        red, green, blue = material.diffuse[:3]
        material.ambient = (red * 0.5, green * 0.5, blue * 0.5)
    material.specular = tuple(min(1.0, c) * 0.3 for c in material.specular)
    material.specular_power = max(5.0, min(material.specular_power, 50.0))
    material.toon_shared = 1
    material.toon_index = 0                        # 共有トゥーン toon01.bmp
    if alpha < 0.999 or any(word in text for word in _NO_EDGE_WORDS):
        material.flags &= ~pmxfile.MATERIAL_EDGE
    if any(word in text for word in _DOUBLE_SIDED_WORDS):
        material.flags |= pmxfile.MATERIAL_DOUBLE_SIDED


def _build_display_frames(model):
    """MMD が要求する Root / 表情 枠と、残りのボーン枠を作る。"""
    root_frame = pmxfile.PmxDisplayFrame("Root", "Root", True)
    if model.bones:
        root_frame.elements.append((pmxfile.FRAME_ELEMENT_BONE, 0))
    expression_frame = pmxfile.PmxDisplayFrame("表情", "Exp", True)
    for index in range(len(model.morphs)):
        expression_frame.elements.append((pmxfile.FRAME_ELEMENT_MORPH, index))
    bone_frame = pmxfile.PmxDisplayFrame("ボーン", "Bone", False)
    for index in range(1, len(model.bones)):
        bone_frame.elements.append((pmxfile.FRAME_ELEMENT_BONE, index))

    model.display_frames = [root_frame, expression_frame]
    if bone_frame.elements:
        model.display_frames.append(bone_frame)


def _validate(model, result):
    """出力前の整合性チェック。問題は警告として積む。"""
    vertex_count = len(model.vertices)
    bone_count = len(model.bones)
    for face in model.faces:
        if any(not 0 <= index < vertex_count for index in face):
            result.warnings.append("面が範囲外の頂点を参照しています")
            break
    total = sum(material.surface_count for material in model.materials)
    if total != len(model.faces) * 3:
        result.warnings.append("マテリアルの面数合計が総面数と一致しません")
    for vertex in model.vertices:
        if any(not 0 <= index < bone_count for index in vertex.bones):
            result.warnings.append("頂点が範囲外のボーンを参照しています")
            break
        weight_sum = sum(vertex.weights[:len(vertex.bones)])
        if abs(weight_sum - 1.0) > 1e-3:
            result.warnings.append("ウェイトの合計が 1 でない頂点があります")
            break
    for bone in model.bones:
        if bone.parent_index >= bone_count:
            result.warnings.append("ボーンの親番号が範囲外です")
            break
