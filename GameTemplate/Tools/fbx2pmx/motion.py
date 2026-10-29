# -*- coding: utf-8 -*-
"""FBX のアニメーション(Take)を MMD のモーション(VMD)へ変換する。

モデル変換(converter.py)と同じ手順で骨格を MMD 標準ボーン構造に整え、
FBX の各フレームの姿勢をその骨格のローカル回転・移動に直して書き出す。

- 回転: バインドポーズからの差分を、PMX の親ボーンからの相対回転にする
- センター: 腰(Hips)の移動量
- 足ＩＫ / つま先ＩＫ: 足首・つま先の移動量(IK を解くと元の足の位置に来る)
- A スタンス化した腕: PMX の腕を下げた分を打ち消し、元の動きを再現する
- 表情: ブレンドシェイプの曲線(DeformPercent)をモーフのキーにする
- 毎フレームのキーは、補間で再現できるものを間引く
"""

from __future__ import annotations

import math

import converter
import fbxscene
import mat4
import pmxfile
import posing
import vmdfile

FPS = 30



class MotionOptions:
    def __init__(self):
        self.take = None              # 使う Take 名(None なら動きのある最初のもの)
        self.fps = FPS
        self.rotation_tolerance = math.radians(0.1)   # 間引きの許容誤差(親子で積み重なるので小さめ)
        self.position_tolerance = 0.002
        self.reduce = True
        self.model = converter.ConvertOptions()       # 骨格の整え方(モデル変換と合わせる)
        self.model.physics = "none"
        self.model.mmd_look = False
        self.model.skip_textures = True


class MotionReport:
    def __init__(self):
        self.take_name = ""
        self.frames = 0
        self.keyed_bones = 0
        self.keys_before = 0
        self.keys_after = 0
        self.morph_tracks = 0
        self.skipped_names = []       # VMD の名前欄(15 バイト)に収まらず外したボーン
        self.warnings = []


def _rotation_matrix_to_mmd(linear, matrix):
    """FBX ワールドの行列の回転部分を、PMX 空間のクォータニオンにする。"""
    rotation = posing.q_from_matrix(matrix)
    as_matrix = _quat_to_matrix(rotation)
    converted = mat4.multiply_all(linear, as_matrix, mat4.transpose(linear))
    return posing.q_from_matrix(converted)


def _quat_to_matrix(q):
    x, y, z, w = q
    return (
        1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w), 0.0,
        2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w), 0.0,
        2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y), 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


def _linear_part(matrix):
    """縮尺を除いた 3x3 部分(回転 + 鏡像)を 4x4 で返す。"""
    scale = math.sqrt(matrix[0] ** 2 + matrix[4] ** 2 + matrix[8] ** 2) or 1.0
    m = [v / scale for v in matrix]
    m[3] = m[7] = m[11] = 0.0
    m[12] = m[13] = m[14] = 0.0
    m[15] = 1.0
    return tuple(m)


def _twist_about(delta, helper, bones, global_rotation, axis):
    """ひねり補助ボーンの親に対する回転から、axis まわりの成分(ひねり)だけを取り出す。"""
    parent = bones[helper].parent_index
    parent_rotation = global_rotation[parent] if parent >= 0 else posing.IDENTITY
    q = posing.q_norm(posing.q_mul(posing.q_conj(parent_rotation), delta[helper]))
    length = math.sqrt(sum(c * c for c in axis))
    if length < 1e-9:
        return posing.IDENTITY
    axis = tuple(c / length for c in axis)
    projection = sum(q[k] * axis[k] for k in range(3))
    twist = (axis[0] * projection, axis[1] * projection, axis[2] * projection, q[3])
    norm = math.sqrt(sum(c * c for c in twist))
    if norm < 1e-9:
        return posing.IDENTITY
    twist = tuple(c / norm for c in twist)
    return posing.IDENTITY if posing.q_angle(twist, posing.IDENTITY) < 1e-6 else twist


def _choose_take(scene, name):
    takes = scene.takes()
    if name is not None:
        for take in takes:
            if take.name == name:
                return take
        raise ValueError("Take が見つかりません: %s (あるもの: %s)"
                         % (name, ", ".join(t.name for t in takes) or "なし"))
    for take in takes:
        if take.has_motion():
            return take
    if takes:
        return takes[0]
    raise ValueError("FBX にアニメーション(Take)が含まれていません")


def _reduce(keys, tolerance_rotation, tolerance_position):
    """キー列 [(フレーム, 位置, 回転)] から、線形補間で再現できるキーを間引く。"""
    if len(keys) <= 2:
        return keys

    def fits(a, b):
        f0, p0, r0 = keys[a]
        f1, p1, r1 = keys[b]
        for k in range(a + 1, b):
            t = (keys[k][0] - f0) / float(f1 - f0)
            position = tuple(x + (y - x) * t for x, y in zip(p0, p1))
            if math.dist(position, keys[k][1]) > tolerance_position:
                return False
            if posing.q_angle(posing.q_slerp(r0, r1, t), keys[k][2]) > tolerance_rotation:
                return False
        return True

    kept = [keys[0]]
    anchor = 0
    candidate = 2
    while candidate < len(keys):
        if candidate - anchor > 90 or not fits(anchor, candidate):
            anchor = candidate - 1
            kept.append(keys[anchor])
        candidate += 1
    kept.append(keys[-1])
    return kept


def _is_rest(keys, tolerance_rotation, tolerance_position):
    return all(posing.q_angle(r, posing.IDENTITY) <= tolerance_rotation
               and math.dist(p, (0.0, 0.0, 0.0)) <= tolerance_position
               for _f, p, r in keys)


def convert_motion(fbx_path, options=None, log=None):
    """FBX の Take を VMD に変換する。

    (VmdMotion, MotionReport, 骨格を作ったときの ConvertResult) を返す。
    """
    options = options or MotionOptions()
    log = log or (lambda message: None)
    report = MotionReport()

    result = converter.convert(fbx_path, fbx_path, options.model, log, skeleton_only=True)
    scene = result.scene
    take = _choose_take(scene, options.take)
    report.take_name = take.name
    if not take.has_motion():
        report.warnings.append("Take「%s」には動きがありません(全キーが同じ時刻)" % take.name)

    step = fbxscene.FBX_TIME_PER_SECOND / float(options.fps)
    frame_count = int(round(take.duration_seconds * options.fps)) + 1

    def sample(frame):
        time = int(round(take.start + frame * step))
        cache = {}
        return {source: scene.global_matrix_at(scene.objects[source], take, time, cache)
                for source in result.bind_globals}

    morph_curves = {}
    for target_id, curve in take.morph_curves.items():
        channel = scene.objects.get(target_id)
        if channel is not None:
            morph_curves[channel.name] = (lambda frame, curve=curve: curve.evaluate(
                int(round(take.start + frame * step))) / 100.0)
    motion = build_motion(result, frame_count, sample, options, report, morph_curves)
    return motion, report, result


def build_motion(result, frame_count, sample, options, report, morph_curves=None):
    """骨格(ConvertResult)と、フレームごとの元のグローバル行列から VMD を組み立てる。

    sample(frame) は {元のボーン id: グローバル行列(元の座標系・列ベクトル規約)} を返す。
    morph_curves は {元のモーフ名: frame -> 重み(0〜1)}。
    """
    model = result.model
    report.frames = frame_count
    bones = model.bones
    index_of = {bone.name: i for i, bone in enumerate(bones)}
    linear = _linear_part(result.matrix)
    sources = [result.bone_sources.get(id(bone)) for bone in bones]

    # A スタンス化で回した腕の補正
    arm_fix = {}
    if result.rig is not None:
        for rotation, rotated in result.rig.arm_rotations:
            for bone in rotated:
                arm_fix[id(bone)] = rotation

    rest_rotation = [None] * len(bones)
    for i, bone in enumerate(bones):
        if sources[i] is None:
            continue
        q = _rotation_matrix_to_mmd(linear, result.bind_globals[sources[i]])
        rest_rotation[i] = posing.q_mul(arm_fix[id(bone)], q) if id(bone) in arm_fix else q

    hips = index_of.get("下半身")
    center = index_of.get("センター")
    ik_targets = []
    for side in ("左", "右"):
        names = (side + "足ＩＫ", side + "足首", side + "つま先ＩＫ", side + "つま先")
        if all(name in index_of for name in names):
            ik_targets.append(tuple(index_of[name] for name in names))

    # 腕捩 / 手捩 と、そこから回転を受け取る元のひねり補助ボーン(元のボーンがあるもの)
    twist_helpers = {}
    for i, bone in enumerate(bones):
        if sources[i] is not None or not bone.flags & pmxfile.BONE_FIXED_AXIS:
            continue
        helpers = [j for j, other in enumerate(bones)
                   if sources[j] is not None and other.flags & pmxfile.BONE_INHERIT_ROTATION
                   and other.inherit_parent_index == i]
        if helpers:
            twist_helpers[i] = helpers[0]

    tracks = {i: [] for i in range(len(bones))}

    for frame in range(frame_count):
        globals_at = sample(frame)
        delta = [posing.IDENTITY] * len(bones)
        world = [None] * len(bones)
        for i, source in enumerate(sources):
            if source is None or source not in globals_at:
                continue
            global_fbx = globals_at[source]
            animated = _rotation_matrix_to_mmd(linear, global_fbx)
            delta[i] = posing.q_norm(posing.q_mul(animated, posing.q_conj(rest_rotation[i])))
            world[i] = mat4.transform_point(result.matrix, mat4.get_translation(global_fbx))

        # IK の目標位置(足首とつま先の元の位置)
        targets = {}
        for ik, ankle, toe_ik, toe in ik_targets:
            if world[ankle] is None:
                continue
            if world[toe] is not None:
                toe_world = world[toe]
            else:  # 補ったつま先は足首と一緒に動かす
                offset = tuple(bones[toe].position[k] - bones[ankle].position[k] for k in range(3))
                rotated = posing.q_rotate(delta[ankle], offset)
                toe_world = tuple(world[ankle][k] + rotated[k] for k in range(3))
            targets[ik] = world[ankle]
            targets[toe_ik] = toe_world

        # PMX の親子関係に沿って、親から順にローカル回転と移動量を解く
        global_rotation = [posing.IDENTITY] * len(bones)
        global_position = [None] * len(bones)
        local = [posing.IDENTITY] * len(bones)
        for i, bone in enumerate(bones):
            parent = bone.parent_index
            if parent >= 0:
                parent_rotation = global_rotation[parent]
                parent_position = global_position[parent]
                rest_offset = tuple(bone.position[k] - bones[parent].position[k] for k in range(3))
            else:
                parent_rotation = posing.IDENTITY
                parent_position = (0.0, 0.0, 0.0)
                rest_offset = bone.position
            inverse_parent = posing.q_conj(parent_rotation)

            def solve_translation(target):
                """bone を target に置くための VMD の移動量(親の座標系)。"""
                local_target = posing.q_rotate(inverse_parent, tuple(
                    target[k] - parent_position[k] for k in range(3)))
                return tuple(local_target[k] - rest_offset[k] for k in range(3))

            translation = (0.0, 0.0, 0.0)
            keyed = False
            inherits = bone.flags & pmxfile.BONE_INHERIT_ROTATION and bone.inherit_parent_index >= 0
            inherited = posing.q_power(local[bone.inherit_parent_index], bone.inherit_rate) \
                if inherits else posing.IDENTITY
            if sources[i] is not None:
                # 付与で受け取る分を差し引いた残りを自分のキーにする
                base = posing.q_mul(parent_rotation, inherited)
                local[i] = posing.q_norm(posing.q_mul(posing.q_conj(base), delta[i]))
                global_rotation[i] = posing.q_norm(posing.q_mul(base, local[i]))
                keyed = True
                if bone.flags & pmxfile.BONE_TRANSLATABLE and world[i] is not None:
                    translation = solve_translation(world[i])
                moved = posing.q_rotate(parent_rotation, tuple(rest_offset[k] + translation[k]
                                                               for k in range(3)))
                global_position[i] = tuple(parent_position[k] + moved[k] for k in range(3))
                tracks[i].append((frame, translation, local[i]))
                continue
            elif inherits:
                local[i] = inherited
            elif i in twist_helpers:
                # 腕捩 / 手捩: 元のひねり補助ボーンの、腕の軸まわりの回転を移す
                local[i] = _twist_about(delta, twist_helpers[i], bones, global_rotation,
                                        bone.fixed_axis)
                keyed = local[i] != posing.IDENTITY
            elif i == center and hips is not None and world[hips] is not None:
                # 元のボーンが無いセンター: 下半身が元の腰の位置に来るよう動かす
                hip_offset = tuple(bones[hips].position[k] - bone.position[k] for k in range(3))
                rotated = posing.q_rotate(parent_rotation, hip_offset)
                translation = solve_translation(tuple(world[hips][k] - rotated[k] for k in range(3)))
                keyed = True
            elif i in targets:
                translation = solve_translation(targets[i])
                keyed = True
            global_rotation[i] = posing.q_norm(posing.q_mul(parent_rotation, local[i]))
            moved = posing.q_rotate(parent_rotation, tuple(rest_offset[k] + translation[k]
                                                           for k in range(3)))
            global_position[i] = tuple(parent_position[k] + moved[k] for k in range(3))
            if keyed:
                tracks[i].append((frame, translation, local[i]))

    motion = vmdfile.VmdMotion(model.name)
    for i, keys in tracks.items():
        if not keys:
            continue
        name = bones[i].name
        if not vmdfile.name_fits(name):
            report.skipped_names.append(name)
            continue
        if _is_rest(keys, options.rotation_tolerance, options.position_tolerance):
            continue
        report.keys_before += len(keys)
        if options.reduce:
            keys = _reduce(keys, options.rotation_tolerance, options.position_tolerance)
        report.keys_after += len(keys)
        report.keyed_bones += 1
        for frame, position, rotation in keys:
            motion.bone_frames.append(vmdfile.BoneFrame(name, frame, position, rotation))

    # 表情(ブレンドシェイプの曲線)
    channels = morph_curves or {}
    for morph in model.morphs:
        curve = channels.get(morph.name_en) or channels.get(morph.name)
        if curve is None or not vmdfile.name_fits(morph.name):
            continue
        keys = [(frame, (curve(frame), 0.0, 0.0), posing.IDENTITY)
                for frame in range(frame_count)]
        if all(abs(k[1][0]) < 1e-4 for k in keys):
            continue
        if options.reduce:
            keys = _reduce(keys, math.pi, 0.002)
        for frame, value, _rotation in keys:
            motion.morph_frames.append(vmdfile.MorphFrame(morph.name, frame, value[0]))
        report.morph_tracks += 1

    motion.bone_frames.sort(key=lambda f: (f.frame, f.name))
    motion.morph_frames.sort(key=lambda f: (f.frame, f.name))
    return motion
