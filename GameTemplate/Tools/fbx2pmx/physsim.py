# -*- coding: utf-8 -*-
"""プレビュー用の簡易物理演算。

物理演算の剛体が付いたボーン(髪・スカートなどの揺れもの)を、重力・ばね・回転の制限・
当たり判定(ボーン追従の剛体)で動かして Pose に書き戻す。MMD(Bullet)の剛体計算を
そのまま再現するものではなく、揺れもの剛体を「付け根を支点に振れる棒」として扱う近似。
スカートが脚を、髪が顔を突き抜けていないかを画像で確かめるためのもの。

    poses = [posing.pose_from_motion(model, motion, f)[0] for f in range(start, frame + 1)]
    physsim.simulate(model, poses)      # poses[-1] に揺れものの回転が入る
"""

from __future__ import annotations

import math

import physics
import pmxfile
from posing import IDENTITY, q_axis_angle, q_conj, q_mul, q_norm, q_rotate

FPS = 30
SUBSTEPS = 4
GRAVITY = 98.0          # MMD の重力 9.8 を 1 単位 = 10cm 相当で
DAMPING = 0.08          # 1 ステップごとに失う速さの割合
COLLISION_PASSES = 2


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _length(a):
    return math.sqrt(_dot(a, a))


def _normalize(a):
    length = _length(a)
    return (0.0, -1.0, 0.0) if length < 1e-12 else _scale(a, 1.0 / length)


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _rotation_between(a, b):
    """単位ベクトル a を b へ向ける最小の回転。"""
    dot = max(-1.0, min(1.0, _dot(a, b)))
    if dot > 1.0 - 1e-12:
        return IDENTITY
    axis = _cross(a, b)
    if _length(axis) < 1e-12:
        axis = _cross(a, (1.0, 0.0, 0.0))
        if _length(axis) < 1e-12:
            axis = _cross(a, (0.0, 0.0, 1.0))
    return q_axis_angle(_normalize(axis), math.acos(dot))


def closest_points(p1, q1, p2, q2):
    """線分 p1-q1 と p2-q2 の最も近い点。(s, 点1, 点2) を返す(s は線分 1 上の位置 0〜1)。"""
    d1 = _sub(q1, p1)
    d2 = _sub(q2, p2)
    r = _sub(p1, p2)
    a = _dot(d1, d1)
    e = _dot(d2, d2)
    f = _dot(d2, r)
    if a < 1e-12 and e < 1e-12:
        s = t = 0.0
    elif a < 1e-12:
        s, t = 0.0, max(0.0, min(1.0, f / e))
    else:
        c = _dot(d1, r)
        if e < 1e-12:
            s, t = max(0.0, min(1.0, -c / a)), 0.0
        else:
            b = _dot(d1, d2)
            denominator = a * e - b * b
            s = max(0.0, min(1.0, (b * f - c * e) / denominator)) if denominator > 1e-12 else 0.0
            t = (b * s + f) / e
            if t < 0.0:
                t, s = 0.0, max(0.0, min(1.0, -c / a))
            elif t > 1.0:
                t, s = 1.0, max(0.0, min(1.0, (b - c) / a))
    return s, _add(p1, _scale(d1, s)), _add(p2, _scale(d2, t))


def _endpoints(body):
    if body.shape != pmxfile.SHAPE_CAPSULE:
        return body.position, body.position
    axis = physics.rotation_axis_y(body.rotation)
    half = body.size[1] * 0.5
    return _sub(body.position, _scale(axis, half)), _add(body.position, _scale(axis, half))


class _Swing:
    """揺れもの剛体 1 つ(付け根を支点に振れる棒)。"""

    def __init__(self, bone_index, body, tip, parent_swing):
        self.bone = bone_index
        self.parent = parent_swing          # 親の _Swing の番号(親が揺れものでなければ None)
        self.radius = body.size[0]
        self.rest_direction = None
        self.length = 0.0
        self.tip_rest = tip
        self.low = (-math.radians(25.0),) * 3      # ジョイントの回転の制限(X, Y, Z)
        self.high = (math.radians(25.0),) * 3
        self.spring = 0.02
        self.colliders = []
        self.children = []                  # 子の _Swing の番号
        self.tip = None
        self.previous = None
        self.position = None                # 今の付け根の位置・回転
        self.rotation = IDENTITY


def _build(model):
    bones = model.bones
    body_of_bone = {}
    for index, body in enumerate(model.rigid_bodies):
        if body.bone_index >= 0 and body.mode != pmxfile.BODY_FOLLOW_BONE:
            body_of_bone.setdefault(body.bone_index, index)
    joint_of_body = {joint.body_b: joint for joint in model.joints}
    followers = [i for i, body in enumerate(model.rigid_bodies)
                 if body.mode == pmxfile.BODY_FOLLOW_BONE and body.bone_index >= 0]

    swings = []
    swing_of_bone = {}
    for bone_index in range(len(bones)):     # 親 -> 子の順に並んでいる
        body_index = body_of_bone.get(bone_index)
        if body_index is None:
            continue
        body = model.rigid_bodies[body_index]
        pivot = bones[bone_index].position
        ends = _endpoints(body)
        tip = max(ends, key=lambda p: _length(_sub(p, pivot)))
        if _length(_sub(tip, pivot)) < 1e-4:
            continue
        parent = bones[bone_index].parent_index
        swing = _Swing(bone_index, body, tip, swing_of_bone.get(parent))
        swing.length = _length(_sub(tip, pivot))
        swing.rest_direction = _normalize(_sub(tip, pivot))
        joint = joint_of_body.get(body_index)
        if joint is not None:
            swing.low = tuple(joint.rotation_min)
            swing.high = tuple(joint.rotation_max)
            swing.spring = min(0.2, max(joint.spring_rotation) * 0.002)
        for other in followers:
            target = model.rigid_bodies[other]
            if body.collision_mask & (1 << target.group) and target.collision_mask & (1 << body.group):
                swing.colliders.append(other)
        swing_of_bone[bone_index] = len(swings)
        if swing.parent is not None:
            swings[swing.parent].children.append(len(swings))
        swings.append(swing)
    return swings


def _posed_colliders(model, pose, follow_scale=1.0):
    """ボーン追従の剛体を今の姿勢へ動かした (始点, 終点, 半径)。

    follow_scale: 読み込む側がボーン追従の剛体に掛ける縮小率(k2Engine は 0.75)
    """
    result = {}
    for index, body in enumerate(model.rigid_bodies):
        if body.mode != pmxfile.BODY_FOLLOW_BONE or body.bone_index < 0:
            continue
        rest = model.bones[body.bone_index].position
        rotation = pose.global_rotation[body.bone_index]
        origin = pose.global_position[body.bone_index]
        a, b = _endpoints(body)
        middle = _scale(_add(a, b), 0.5)
        a = _add(middle, _scale(_sub(a, middle), follow_scale))
        b = _add(middle, _scale(_sub(b, middle), follow_scale))
        result[index] = (_add(origin, q_rotate(rotation, _sub(a, rest))),
                         _add(origin, q_rotate(rotation, _sub(b, rest))),
                         body.size[0] * follow_scale)
    return result


def _frame(model, swings, pose, swing):
    """親の今の状態から、このボーンの付け根の位置と「揺れなければ向く方向」を求める。"""
    bones = model.bones
    bone = bones[swing.bone]
    parent = bone.parent_index
    if swing.parent is not None:
        parent_swing = swings[swing.parent]
        parent_rotation, parent_position = parent_swing.rotation, parent_swing.position
    elif parent >= 0:
        parent_rotation, parent_position = pose.global_rotation[parent], pose.global_position[parent]
    else:
        parent_rotation, parent_position = IDENTITY, (0.0, 0.0, 0.0)
    rest_parent = bones[parent].position if parent >= 0 else (0.0, 0.0, 0.0)
    position = _add(parent_position, q_rotate(parent_rotation, _sub(bone.position, rest_parent)))
    follow = q_norm(q_mul(parent_rotation, pose.local_rotation(swing.bone)))
    direction = _normalize(q_rotate(follow, swing.rest_direction))
    return position, follow, direction, parent_rotation


def _limited(tip, position, direction, swing, frame_rotation):
    """長さを保ち、ジョイントの回転の制限の内側へ直した先端。

    揺れなかったときの向きから今の向きへの回転を、ジョイントの軸(静止状態のワールド軸を
    親の回転で動かしたもの)まわりの成分に分け、軸ごとに最小〜最大へ収める。
    """
    current = _normalize(_sub(tip, position))
    axis = _cross(direction, current)
    sine = _length(axis)
    if sine < 1e-9:
        return _add(position, _scale(current, swing.length))
    angle = math.atan2(sine, _dot(direction, current))
    vector = _scale(axis, angle / sine)              # 回転ベクトル(軸 × 角度)
    axes = [q_rotate(frame_rotation, a) for a in ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0),
                                                   (0.0, 0.0, 1.0))]
    clamped = (0.0, 0.0, 0.0)
    for k in range(3):
        # 制限の + 側は右手系の回転と逆向きに効く(k2Engine の Bullet で確かめた向き)
        amount = max(swing.low[k], min(swing.high[k], -_dot(vector, axes[k])))
        clamped = _add(clamped, _scale(axes[k], -amount))
    angle = _length(clamped)
    if angle > 1e-9:
        current = q_rotate(q_axis_angle(_scale(clamped, 1.0 / angle), angle), direction)
    else:
        current = direction
    return _add(position, _scale(current, swing.length))


def _step(model, swings, pose, colliders, dt):
    gravity = (0.0, -GRAVITY * dt * dt, 0.0)
    for swing in swings:
        position, follow, direction, parent_rotation = _frame(model, swings, pose, swing)
        # ジョイントの軸は親の剛体に付いて動く(静止状態からの親の回転)
        frame_rotation = parent_rotation
        if swing.tip is None:
            swing.tip = swing.previous = _add(position, _scale(direction, swing.length))
        target = _add(position, _scale(direction, swing.length))
        velocity = _scale(_sub(swing.tip, swing.previous), 1.0 - DAMPING)
        tip = _add(_add(swing.tip, velocity), gravity)
        tip = _add(tip, _scale(_sub(target, tip), swing.spring))
        tip = _limited(tip, position, direction, swing, frame_rotation)
        for _ in range(COLLISION_PASSES):
            for index in swing.colliders:
                a, b, radius = colliders[index]
                s, near, other = closest_points(position, tip, a, b)
                gap = _length(_sub(near, other))
                reach = swing.radius + radius
                if gap >= reach:
                    continue
                normal = _scale(_sub(near, other), 1.0 / gap) if gap > 1e-9 \
                    else _normalize(_sub(tip, other))
                tip = _add(tip, _scale(normal, (reach - gap) / max(s, 0.2)))
            # 先端は子の付け根でもあるので、子の当たり判定でも押し出す(付け根は
            # 付け根の当たり判定とぶつからないため、そのままだと腰などへ垂れ込み、
            # 子の剛体が根元でぶつかって大きく跳ね上がる)
            for child in swing.children:
                child = swings[child]
                for index in child.colliders:
                    a, b, radius = colliders[index]
                    _s, _near, other = closest_points(tip, tip, a, b)
                    gap = _length(_sub(tip, other))
                    reach = child.radius + radius
                    if gap < reach and gap > 1e-9:
                        tip = _add(tip, _scale(_sub(tip, other), (reach - gap) / gap))
            # 当たり判定で押し出しても回転の制限より外へは振らない(付け根の近くで
            # ぶつかったときに先が跳ね上がらないように。少しの食い込みは残る)
            tip = _limited(tip, position, direction, swing, frame_rotation)
        swing.previous, swing.tip = swing.tip, tip
        swing.position = position
        swing.rotation = q_norm(q_mul(_rotation_between(direction, _normalize(_sub(tip, position))),
                                      follow))


def follow_scale_for(model):
    """モデルのコメントから、ボーン追従の剛体の縮小率を決める(k2Engine 向けなら 0.75)。"""
    return 0.75 if "k2Engine" in (model.comment or "") else 1.0


def simulate(model, poses, follow_scale=None):
    """poses(連続したフレームの、物理なしで解いた姿勢)を順に流して揺れものを動かす。

    最後の姿勢に揺れものの回転を書き込んで返す。揺れものが無ければそのまま返す。
    follow_scale: ボーン追従の剛体の縮小率。省略するとモデルのコメントから決める
    """
    pose = poses[-1]
    swings = _build(model)
    if not swings:
        return pose
    if follow_scale is None:
        follow_scale = follow_scale_for(model)
    dt = 1.0 / (FPS * SUBSTEPS)
    for frame_pose in poses:
        colliders = _posed_colliders(model, frame_pose, follow_scale)
        for _ in range(SUBSTEPS):
            _step(model, swings, frame_pose, colliders, dt)

    # 求めた回転を、親から見たローカル回転として書き戻す
    bones = model.bones
    for swing in swings:
        parent = bones[swing.bone].parent_index
        if swing.parent is not None:
            parent_rotation = swings[swing.parent].rotation
        else:
            parent_rotation = pose.global_rotation[parent] if parent >= 0 else IDENTITY
        pose.rotation[swing.bone] = q_norm(q_mul(q_conj(parent_rotation), swing.rotation))
        pose.ik_rotation[swing.bone] = IDENTITY
    pose.update()
    return pose
