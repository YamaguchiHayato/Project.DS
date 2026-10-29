# -*- coding: utf-8 -*-
"""PMX に VMD を当てたときの姿勢を計算する(確認・検証用の簡易版 MMD)。

MMD と同じくボーン番号順の FK、回転付与(IK の結果を含む)、CCD による IK
(ひざなど角度制限付きのリンクは X 軸まわりだけ)、線形ブレンドスキニングで
頂点を動かす。物理演算は行わない。

クォータニオンは (x, y, z, w)。PMX / VMD の座標値をそのまま使う
(MMD の左手系の回転は、数値上は通常の回転行列と同じ働きになる)。
"""

from __future__ import annotations

import bisect
import math

import pmxfile

IDENTITY = (0.0, 0.0, 0.0, 1.0)


def q_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def q_conj(q):
    return (-q[0], -q[1], -q[2], q[3])


def q_norm(q):
    length = math.sqrt(sum(c * c for c in q)) or 1.0
    return tuple(c / length for c in q)


def q_axis_angle(axis, angle):
    s = math.sin(angle * 0.5)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(angle * 0.5))


def q_rotate(q, v):
    """v' = q v q*"""
    x, y, z, w = q
    tx = 2.0 * (y * v[2] - z * v[1])
    ty = 2.0 * (z * v[0] - x * v[2])
    tz = 2.0 * (x * v[1] - y * v[0])
    return (v[0] + w * tx + (y * tz - z * ty),
            v[1] + w * ty + (z * tx - x * tz),
            v[2] + w * tz + (x * ty - y * tx))


def q_power(q, t):
    """q の回転角を t 倍したクォータニオン(負の t は逆回転)。付与率に使う。"""
    if q[3] < 0.0:
        q = tuple(-c for c in q)
    angle = 2.0 * math.acos(max(-1.0, min(1.0, q[3])))
    s = math.sin(angle * 0.5)
    if angle < 1e-9 or s < 1e-9:
        return IDENTITY
    return q_axis_angle((q[0] / s, q[1] / s, q[2] / s), angle * t)


def q_slerp(a, b, t):
    dot = sum(x * y for x, y in zip(a, b))
    if dot < 0.0:
        b = tuple(-c for c in b)
        dot = -dot
    if dot > 0.9995:
        return q_norm(tuple(x + (y - x) * t for x, y in zip(a, b)))
    theta = math.acos(dot)
    s = math.sin(theta)
    wa = math.sin((1.0 - t) * theta) / s
    wb = math.sin(t * theta) / s
    return tuple(x * wa + y * wb for x, y in zip(a, b))


def q_from_matrix(m):
    """4x4(列ベクトル規約)の回転部分からクォータニオンを作る(拡縮は正規化して除く)。"""
    columns = []
    for c in range(3):
        v = (m[c], m[4 + c], m[8 + c])
        length = math.sqrt(sum(x * x for x in v)) or 1.0
        columns.append(tuple(x / length for x in v))
    r00, r10, r20 = columns[0]
    r01, r11, r21 = columns[1]
    r02, r12, r22 = columns[2]
    trace = r00 + r11 + r22
    if trace > 0.0:
        s = math.sqrt(trace + 1.0) * 2.0
        q = ((r21 - r12) / s, (r02 - r20) / s, (r10 - r01) / s, 0.25 * s)
    elif r00 > r11 and r00 > r22:
        s = math.sqrt(1.0 + r00 - r11 - r22) * 2.0
        q = (0.25 * s, (r01 + r10) / s, (r02 + r20) / s, (r21 - r12) / s)
    elif r11 > r22:
        s = math.sqrt(1.0 + r11 - r00 - r22) * 2.0
        q = ((r01 + r10) / s, 0.25 * s, (r12 + r21) / s, (r02 - r20) / s)
    else:
        s = math.sqrt(1.0 + r22 - r00 - r11) * 2.0
        q = ((r02 + r20) / s, (r12 + r21) / s, 0.25 * s, (r10 - r01) / s)
    return q_norm(q)


def q_angle(a, b):
    """2 つの回転の差の角度(ラジアン)。"""
    dot = abs(sum(x * y for x, y in zip(a, b)))
    return 2.0 * math.acos(max(-1.0, min(1.0, dot)))


class Pose:
    """ボーンごとのローカル回転・移動から、グローバルな姿勢と頂点位置を求める。"""

    def __init__(self, model):
        self.model = model
        count = len(model.bones)
        self.rotation = [IDENTITY] * count       # VMD の回転
        self.translation = [(0.0, 0.0, 0.0)] * count
        self.ik_rotation = [IDENTITY] * count    # IK が加えた回転
        self.global_rotation = [IDENTITY] * count
        self.global_position = [bone.position for bone in model.bones]

    def local_rotation(self, index):
        bone = self.model.bones[index]
        q = q_mul(self.ik_rotation[index], self.rotation[index])
        if bone.flags & pmxfile.BONE_INHERIT_ROTATION and bone.inherit_parent_index >= 0:
            j = bone.inherit_parent_index
            source = q_mul(self.ik_rotation[j], self.rotation[j])  # 付与は IK の結果も含む
            q = q_mul(q_power(source, bone.inherit_rate), q)
        return q

    def update(self, start=0):
        bones = self.model.bones
        for index in range(start, len(bones)):
            bone = bones[index]
            q = self.local_rotation(index)
            parent = bone.parent_index
            if parent < 0:
                self.global_rotation[index] = q
                self.global_position[index] = tuple(
                    bone.position[k] + self.translation[index][k] for k in range(3))
                continue
            offset = tuple(bone.position[k] - bones[parent].position[k]
                           + self.translation[index][k] for k in range(3))
            moved = q_rotate(self.global_rotation[parent], offset)
            self.global_position[index] = tuple(
                self.global_position[parent][k] + moved[k] for k in range(3))
            self.global_rotation[index] = q_norm(q_mul(self.global_rotation[parent], q))

    def solve(self):
        """FK を計算し、IK ボーンを番号順に解く。"""
        self.update()
        for index, bone in enumerate(self.model.bones):
            if bone.flags & pmxfile.BONE_IK and bone.ik is not None:
                self._solve_ik(index, bone.ik)
        self.update()

    def _solve_ik(self, index, ik):
        start = min([link.bone_index for link in ik.links] + [ik.target_index])
        for _iteration in range(max(1, ik.loop_count)):
            for link in ik.links:
                j = link.bone_index
                goal = self.global_position[index]
                effector = self.global_position[ik.target_index]
                origin = self.global_position[j]
                inverse = q_conj(self.global_rotation[j])
                to_effector = q_rotate(inverse, tuple(effector[k] - origin[k] for k in range(3)))
                to_goal = q_rotate(inverse, tuple(goal[k] - origin[k] for k in range(3)))
                le = math.sqrt(sum(c * c for c in to_effector))
                lg = math.sqrt(sum(c * c for c in to_goal))
                if le < 1e-9 or lg < 1e-9:
                    continue
                to_effector = tuple(c / le for c in to_effector)
                to_goal = tuple(c / lg for c in to_goal)
                dot = max(-1.0, min(1.0, sum(a * b for a, b in zip(to_effector, to_goal))))
                angle = math.acos(dot)
                if angle < 1e-6:
                    continue
                angle = min(angle, ik.limit_radian)
                axis = (to_effector[1] * to_goal[2] - to_effector[2] * to_goal[1],
                        to_effector[2] * to_goal[0] - to_effector[0] * to_goal[2],
                        to_effector[0] * to_goal[1] - to_effector[1] * to_goal[0])
                length = math.sqrt(sum(c * c for c in axis))
                axis = (1.0, 0.0, 0.0) if length < 1e-9 else tuple(c / length for c in axis)
                q = q_norm(q_mul(self.ik_rotation[j], q_axis_angle(axis, angle)))
                if link.has_limit:
                    total = q_mul(q, self.rotation[j])
                    theta = 2.0 * math.atan2(total[0], total[3])
                    if theta > math.pi:
                        theta -= 2.0 * math.pi
                    theta = max(link.limit_min[0], min(link.limit_max[0], theta))
                    q = q_mul(q_axis_angle((1.0, 0.0, 0.0), theta), q_conj(self.rotation[j]))
                self.ik_rotation[j] = q
                self.update(start)
            if math.dist(self.global_position[ik.target_index], self.global_position[index]) < 1e-5:
                break

    def skinned_vertices(self, morph_weights=None):
        """モーフと姿勢を反映した頂点位置と法線を返す。"""
        model = self.model
        positions = [list(v.position) for v in model.vertices]
        for morph in model.morphs:
            weight = (morph_weights or {}).get(morph.name, 0.0)
            if weight and morph.morph_type == pmxfile.MORPH_VERTEX:
                for index, delta in morph.offsets:
                    for k in range(3):
                        positions[index][k] += delta[k] * weight
        bones = model.bones
        out_positions = []
        out_normals = []
        for vi, vertex in enumerate(model.vertices):
            p = [0.0, 0.0, 0.0]
            n = [0.0, 0.0, 0.0]
            for b, w in zip(vertex.bones, vertex.weights):
                if w <= 0.0:
                    continue
                rest = bones[b].position
                moved = q_rotate(self.global_rotation[b],
                                 tuple(positions[vi][k] - rest[k] for k in range(3)))
                normal = q_rotate(self.global_rotation[b], vertex.normal)
                for k in range(3):
                    p[k] += w * (self.global_position[b][k] + moved[k])
                    n[k] += w * normal[k]
            out_positions.append(tuple(p))
            out_normals.append(tuple(n))
        return out_positions, out_normals


def _interpolate(keys, frame):
    """[(フレーム, 値)] から frame の値を補間する(位置は線形、回転は球面線形)。"""
    frames = [k[0] for k in keys]
    i = bisect.bisect_right(frames, frame)
    if i == 0:
        return keys[0][1], keys[0][2]
    if i >= len(keys):
        return keys[-1][1], keys[-1][2]
    f0, p0, r0 = keys[i - 1]
    f1, p1, r1 = keys[i]
    t = (frame - f0) / float(f1 - f0) if f1 != f0 else 0.0
    position = tuple(a + (b - a) * t for a, b in zip(p0, p1))
    return position, q_slerp(r0, r1, t)


class MotionSampler:
    """VMD のキーをボーン・モーフごとに並べておき、好きなフレームの姿勢を作る。

    同じモーションから何フレームも姿勢を作るとき(プレビューの物理演算など)に使う。
    """

    def __init__(self, model, motion):
        self.model = model
        index_of = {bone.name: i for i, bone in enumerate(model.bones)}
        tracks = {}
        for key in motion.bone_frames:
            tracks.setdefault(key.name, []).append((key.frame, key.position, q_norm(key.rotation)))
        self.missing = []
        self.bone_tracks = []
        for name, keys in tracks.items():
            index = index_of.get(name)
            if index is None:
                self.missing.append(name)
                continue
            keys.sort(key=lambda k: k[0])
            self.bone_tracks.append((index, keys))
        morph_names = {morph.name for morph in model.morphs}
        morph_tracks = {}
        for key in motion.morph_frames:
            morph_tracks.setdefault(key.name, []).append((key.frame, (key.weight, 0.0, 0.0), IDENTITY))
        self.morph_tracks = []
        for name, keys in morph_tracks.items():
            if name not in morph_names:
                self.missing.append("(モーフ)" + name)
                continue
            keys.sort(key=lambda k: k[0])
            self.morph_tracks.append((name, keys))

    def pose(self, frame):
        """frame 時点の (Pose, モーフの重み) を返す。IK まで解いてある。"""
        pose = Pose(self.model)
        for index, keys in self.bone_tracks:
            position, rotation = _interpolate(keys, frame)
            pose.translation[index] = position
            pose.rotation[index] = rotation
        weights = {name: _interpolate(keys, frame)[0][0] for name, keys in self.morph_tracks}
        pose.solve()
        return pose, weights


def pose_from_motion(model, motion, frame=0):
    """VMD の frame 時点の姿勢を作る。(Pose, モーフの重み, 見つからなかった名前) を返す。"""
    sampler = MotionSampler(model, motion)
    pose, weights = sampler.pose(frame)
    return pose, weights, sampler.missing
