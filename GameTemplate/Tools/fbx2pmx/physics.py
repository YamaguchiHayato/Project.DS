# -*- coding: utf-8 -*-
"""髪・スカート・袖などの揺れものに、剛体とジョイントを自動で付ける。

FBX には物理演算の情報が入っていないため、ボーン名と頂点の付き方から
叩き台を作る。揺れ具合は好みが分かれるので PMXEditor で調整する前提。

- 当たり判定: 頭・上半身・下半身・脚・腕にボーン追従のカプセル(球)
- 揺れもの: 名前で判定したボーンごとに物理演算のカプセル + ジョイント
- 大きさ: そのボーンに主に乗っている頂点の広がりから決める
- 初期状態で重なっている剛体同士は衝突させない(MMD で暴れる原因になるため)

座標は PMX(MMD) 空間。剛体の回転は MMD と同じく Ry * Rx * Rz の順。
"""

from __future__ import annotations

import math
import re

import bonenames
import mat4
import pmxfile

# 衝突グループ(0〜15。PMXEditor 上の表示は +1)
GROUP_HEAD = 0
GROUP_BODY = 1
GROUP_LOWER = 2
GROUP_LEG = 3
GROUP_ARM = 4
GROUP_ANCHOR = 10   # 当たり判定の無い付け根用

# 当たり判定を付ける標準ボーンとグループ
_COLLISION_BONES = (
    ("頭", GROUP_HEAD),
    ("上半身", GROUP_BODY), ("上半身2", GROUP_BODY), ("上半身3", GROUP_BODY),
    ("下半身", GROUP_LOWER),
    ("左足", GROUP_LEG), ("左ひざ", GROUP_LEG), ("右足", GROUP_LEG), ("右ひざ", GROUP_LEG),
    ("左腕", GROUP_ARM), ("左ひじ", GROUP_ARM), ("右腕", GROUP_ARM), ("右ひじ", GROUP_ARM),
)
# 当たり判定のカプセルの、次の端点にするボーン
_COLLISION_ENDS = {
    "上半身": ("上半身2", "上半身3", "首"), "上半身2": ("上半身3", "首"), "上半身3": ("首",),
    "左足": ("左ひざ",), "左ひざ": ("左足首",), "右足": ("右ひざ",), "右ひざ": ("右足首",),
    "左腕": ("左ひじ",), "左ひじ": ("左手首",), "右腕": ("右ひじ",), "右ひじ": ("右手首",),
}
# 揺れものとの隙間を作るため、当たり判定は頂点の広がりより少し細くする
_COLLISION_SHRINK = 0.75


class SwingKind:
    """揺れものの種類ごとの設定。"""

    def __init__(self, name, keywords, group, limit_deg, spring, angular_damping):
        self.name = name
        self.keywords = keywords
        self.group = group
        self.limit = tuple(math.radians(v) for v in limit_deg)
        self.spring = spring
        self.angular_damping = angular_damping


# 判定は上から順(HeadRibbon はリボン、HairTail は髪として扱う)
SWING_KINDS = (
    SwingKind("リボン", ("ribbon", "リボン"), 7, (10.0, 5.0, 10.0), 100.0, 0.95),
    SwingKind("髪", ("hair", "髪", "kami", "bangs", "fringe", "pony", "twintail", "ahoge",
                    "もみあげ", "前髪", "後髪", "横髪"), 5, (25.0, 10.0, 25.0), 5.0, 0.9),
    SwingKind("スカート", ("skirt", "スカート", "dress", "cape", "cloak", "mantle", "マント",
                       "coat", "コート"), 6, (25.0, 5.0, 25.0), 10.0, 0.9),
    SwingKind("袖", ("sode", "sleeve", "袖", "cuff"), 8, (20.0, 5.0, 20.0), 20.0, 0.9),
    SwingKind("揺れもの", ("tail", "しっぽ", "尻尾", "string", "紐", "tassel", "frill",
                       "フリル", "chain"), 9, (20.0, 10.0, 20.0), 10.0, 0.9),
)
_SWING_GROUPS = {kind.group for kind in SWING_KINDS}


class PhysicsReport:
    def __init__(self):
        self.collision_bodies = 0
        self.swing_bodies = 0
        self.joints = 0
        self.chains = {}           # 種類名 -> 連鎖の数
        self.relaxed = 0           # 初期状態の重なりで衝突を外した組の数


# --------------------------------------------------------------------------
# 幾何
# --------------------------------------------------------------------------

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


def _point_segment_distance(p, a, b):
    ab = _sub(b, a)
    denominator = _dot(ab, ab)
    t = 0.0 if denominator < 1e-12 else max(0.0, min(1.0, _dot(_sub(p, a), ab) / denominator))
    return _length(_sub(p, _add(a, _scale(ab, t))))


def segment_distance(p1, q1, p2, q2):
    """線分 p1-q1 と p2-q2 の最短距離。"""
    d1 = _sub(q1, p1)
    d2 = _sub(q2, p2)
    r = _sub(p1, p2)
    a = _dot(d1, d1)
    e = _dot(d2, d2)
    f = _dot(d2, r)
    if a < 1e-12 and e < 1e-12:
        return _length(r)
    if a < 1e-12:
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
    closest1 = _add(p1, _scale(d1, s))
    closest2 = _add(p2, _scale(d2, t))
    return _length(_sub(closest1, closest2))


def capsule_rotation(direction):
    """ローカル Y 軸を direction に向ける回転 (rx, ry, rz)。R = Ry * Rx * Rz。"""
    x, y, z = mat4.normalize(direction)
    rx = math.acos(max(-1.0, min(1.0, y)))
    ry = math.atan2(x, z) if math.sin(rx) > 1e-6 else 0.0
    return (rx, ry, 0.0)


def rotation_axis_y(rotation):
    """capsule_rotation の逆確認用: 回転後のローカル Y 軸。"""
    rx, ry, rz = rotation
    matrix = mat4.multiply_all(mat4.rotation_y(ry), mat4.rotation_x(rx), mat4.rotation_z(rz))
    return mat4.transform_vector(matrix, (0.0, 1.0, 0.0))


class _Shape:
    """重なり判定用の線分 + 半径(球は両端が同じ点)。"""

    def __init__(self, start, end, radius):
        self.start = start
        self.end = end
        self.radius = radius

    def overlaps(self, other, margin=0.98):
        distance = segment_distance(self.start, self.end, other.start, other.end)
        return distance < (self.radius + other.radius) * margin


# --------------------------------------------------------------------------
# 生成
# --------------------------------------------------------------------------

def swing_kind(bone):
    """ボーン名から揺れものの種類を返す。標準ボーンや IK は対象外。"""
    if bone.flags & pmxfile.BONE_IK:
        return None
    original = bone.name_en or bone.name
    if bonenames.to_mmd_name(original) is not None:
        return None
    text = (original + " " + bone.name).lower()
    for kind in SWING_KINDS:
        if any(keyword in text for keyword in kind.keywords):
            return kind
    return None


class _Builder:
    def __init__(self, model, report):
        self.model = model
        self.report = report
        self.bones = model.bones
        self.index_of = {bone.name: i for i, bone in enumerate(self.bones)}
        self.children = {}
        for i, bone in enumerate(self.bones):
            self.children.setdefault(bone.parent_index, []).append(i)
        self.main_vertices = {}       # ボーン番号 -> 主にそのボーンに乗る頂点の位置
        self.weighted = set()
        for vertex in model.vertices:
            count = min(len(vertex.bones), len(vertex.weights))
            if count == 0:
                continue
            main = max(range(count), key=lambda k: vertex.weights[k])
            self.main_vertices.setdefault(vertex.bones[main], []).append(vertex.position)
            for k in range(count):
                if vertex.weights[k] > 0.0:
                    self.weighted.add(vertex.bones[k])
        self.body_of_bone = {}        # ボーン番号 -> 剛体番号
        self.shapes = []              # 剛体番号 -> _Shape
        ys = [v.position[1] for v in model.vertices] or [0.0, 1.0]
        self.unit = max((max(ys) - min(ys)) * 0.01, 1e-3)

    # -- 共通 ------------------------------------------------------------
    def _radius(self, bone_index, start, end, fallback):
        points = self.main_vertices.get(bone_index)
        if not points:
            return fallback
        distances = sorted(_point_segment_distance(p, start, end) for p in points)
        return distances[len(distances) // 2]

    def _add_body(self, body, shape):
        self.model.rigid_bodies.append(body)
        self.shapes.append(shape)
        index = len(self.model.rigid_bodies) - 1
        if body.bone_index >= 0:
            self.body_of_bone[body.bone_index] = index
        return index

    def _capsule(self, name, name_en, bone_index, start, end, radius):
        body = pmxfile.PmxRigidBody(name, name_en, bone_index)
        axis = _sub(end, start)
        length = _length(axis)
        if length < 1e-6:
            body.shape = pmxfile.SHAPE_SPHERE
            body.size = (radius, 0.0, 0.0)
            body.position = start
        else:
            body.shape = pmxfile.SHAPE_CAPSULE
            body.size = (radius, length, 0.0)
            body.position = _scale(_add(start, end), 0.5)
            body.rotation = capsule_rotation(axis)
        return body, _Shape(start, end, radius)

    # -- 当たり判定 ------------------------------------------------------
    def build_collisions(self):
        bones = self.bones
        swing_mask = 0
        for group in _SWING_GROUPS:
            swing_mask |= 1 << group
        for name, group in _COLLISION_BONES:
            index = self.index_of.get(name)
            if index is None:
                continue
            start = bones[index].position
            if name == "頭":
                points = self.main_vertices.get(index) or [start]
                center = tuple(sum(p[k] for p in points) / len(points) for k in range(3))
                distances = sorted(_length(_sub(p, center)) for p in points)
                radius = max(distances[len(distances) // 2] * 0.8, self.unit * 3)
                body, shape = self._capsule(name, "head", index, center, center, radius)
            elif name == "下半身":
                left = self.index_of.get("左足")
                right = self.index_of.get("右足")
                if left is None or right is None:
                    continue
                a, b = bones[right].position, bones[left].position
                width = _length(_sub(b, a))
                radius = self._radius(index, a, b, width * 0.5) * _COLLISION_SHRINK
                radius = max(min(radius, width * 0.8), self.unit)
                body, shape = self._capsule(name, "lower body", index, a, b, radius)
            else:
                end = None
                for candidate in _COLLISION_ENDS.get(name, ()):
                    if candidate in self.index_of:
                        end = bones[self.index_of[candidate]].position
                        break
                if end is None:
                    continue
                length = _length(_sub(end, start))
                radius = self._radius(index, start, end, length * 0.25) * _COLLISION_SHRINK
                radius = max(min(radius, length * 0.6), self.unit)
                body, shape = self._capsule(name, bones[index].name_en or name, index,
                                            start, end, radius)
            body.group = group
            body.collision_mask = swing_mask
            body.mode = pmxfile.BODY_FOLLOW_BONE
            self._add_body(body, shape)
            self.report.collision_bodies += 1

    # -- 揺れもの --------------------------------------------------------
    def _subtree_weighted(self, index):
        stack = [index]
        while stack:
            current = stack.pop()
            if current in self.weighted:
                return True
            stack.extend(self.children.get(current, ()))
        return False

    def _segment(self, index, swing_children, parent_segment):
        """揺れものボーンの線分(付け根 → 先端)を決める。"""
        bone = self.bones[index]
        start = bone.position
        if swing_children:
            ends = [self.bones[c].position for c in swing_children]
            end = tuple(sum(p[k] for p in ends) / len(ends) for k in range(3))
            if _length(_sub(end, start)) > 1e-6:
                return start, end
        points = self.main_vertices.get(index)
        if points:
            centroid = tuple(sum(p[k] for p in points) / len(points) for k in range(3))
            direction = _sub(centroid, start)
            if _length(direction) > 1e-6:
                direction = mat4.normalize(direction)
                reach = sorted(max(0.0, _dot(_sub(p, start), direction)) for p in points)
                length = reach[int(len(reach) * 0.9)] if reach else 0.0
                if length > 1e-6:
                    return start, _add(start, _scale(direction, length))
        if parent_segment is not None:
            direction = _sub(parent_segment[1], parent_segment[0])
            return start, _add(start, _scale(direction, 0.8))
        return start, _add(start, (0.0, -self.unit * 5, 0.0))

    def _anchor_body(self, bone_index):
        """揺れもの連鎖の付け根をつなぐ、ボーン追従の剛体を探す(無ければ作る)。"""
        current = bone_index
        while current >= 0:
            body = self.body_of_bone.get(current)
            if body is not None and self.model.rigid_bodies[body].mode == pmxfile.BODY_FOLLOW_BONE:
                return body
            current = self.bones[current].parent_index
        if bone_index < 0:
            return None
        bone = self.bones[bone_index]
        body, shape = self._capsule(bone.name, bone.name_en, bone_index,
                                    bone.position, bone.position, self.unit)
        body.group = GROUP_ANCHOR
        body.collision_mask = 0
        body.mode = pmxfile.BODY_FOLLOW_BONE
        return self._add_body(body, shape)

    def build_swings(self):
        bones = self.bones
        kinds = {}
        for index, bone in enumerate(bones):
            kind = swing_kind(bone)
            parent = bone.parent_index
            if kind is None and parent in kinds and index in self.weighted:
                kind = kinds[parent]  # 揺れものの子は同じ種類として扱う
            if kind is not None and self._subtree_weighted(index):
                kinds[index] = kind

        segments = {}
        depth = {}
        for index in range(len(bones)):          # 親 -> 子の順に並んでいる
            kind = kinds.get(index)
            if kind is None:
                continue
            parent = bones[index].parent_index
            is_root = parent not in kinds
            swing_children = [c for c in self.children.get(index, ()) if c in kinds]
            start, end = self._segment(index, swing_children,
                                       None if is_root else segments.get(parent))
            segments[index] = (start, end)
            depth[index] = 0 if is_root else depth[parent] + 1
            length = _length(_sub(end, start))
            radius = self._radius(index, start, end, length * 0.2) * 0.8
            radius = max(min(radius, length * 0.45), length * 0.1, self.unit * 0.5)

            bone = bones[index]
            body, shape = self._capsule(bone.name, bone.name_en, index, start, end, radius)
            body.group = kind.group
            body.mode = pmxfile.BODY_PHYSICS_ALIGNED
            body.mass = max(0.2, 0.8 ** depth[index])
            body.linear_damping = 0.5
            body.angular_damping = kind.angular_damping
            body.restitution = 0.0
            body.friction = 0.5
            mask = 0xFFFF
            for group in _SWING_GROUPS:
                mask &= ~(1 << group)           # 揺れもの同士はぶつけない
            parent_body = self._anchor_body(parent) if is_root else self.body_of_bone[parent]
            if is_root and parent_body is not None:
                anchor_group = self.model.rigid_bodies[parent_body].group
                mask &= ~(1 << anchor_group)    # 付け根の当たり判定とはぶつけない
                self.report.chains[kind.name] = self.report.chains.get(kind.name, 0) + 1
            body.collision_mask = mask
            body_index = self._add_body(body, shape)
            self.report.swing_bodies += 1

            if parent_body is not None:
                joint = pmxfile.PmxJoint(bone.name, bone.name_en, parent_body, body_index)
                joint.position = start
                joint.rotation_min = tuple(-v for v in kind.limit)
                joint.rotation_max = kind.limit
                joint.spring_rotation = (kind.spring, kind.spring, kind.spring)
                self.model.joints.append(joint)
                self.report.joints += 1

    # -- 左右をそろえる --------------------------------------------------
    def symmetrize(self):
        """左右対になる剛体(J_L_xxx / J_R_xxx など)の半径を平均してそろえる。"""
        def mirror_key(name):
            text = re.sub(r"(?i)(^|[_\.\s])(l|r)([_\.\s]|$)", r"\1#\3", name)
            text = re.sub(r"(?i)left|right", "#", text)
            return text.replace("左", "#").replace("右", "#")

        groups = {}
        for index, body in enumerate(self.model.rigid_bodies):
            key = mirror_key(body.name_en or body.name)
            if key != (body.name_en or body.name):
                groups.setdefault((key, body.mode), []).append(index)
        for indexes in groups.values():
            if len(indexes) != 2:
                continue
            radius = sum(self.model.rigid_bodies[i].size[0] for i in indexes) / 2.0
            for i in indexes:
                body = self.model.rigid_bodies[i]
                body.size = (radius,) + tuple(body.size[1:])
                self.shapes[i].radius = radius

    # -- 初期状態の重なり ------------------------------------------------
    def relax_initial_overlaps(self):
        """静止状態で重なっている揺れもの剛体は、その相手グループとぶつけない。"""
        bodies = self.model.rigid_bodies
        for i, body in enumerate(bodies):
            if body.mode == pmxfile.BODY_FOLLOW_BONE:
                continue
            for j, other in enumerate(bodies):
                if i == j or not body.collision_mask & (1 << other.group):
                    continue
                if not other.collision_mask & (1 << body.group):
                    continue
                if self.shapes[i].overlaps(self.shapes[j]):
                    body.collision_mask &= ~(1 << other.group)
                    self.report.relaxed += 1


def generate(model):
    """モデルに剛体とジョイントを追加する。既にあれば何もしない。"""
    report = PhysicsReport()
    if model.rigid_bodies or model.joints:
        return report
    builder = _Builder(model, report)
    builder.build_collisions()
    builder.build_swings()
    if report.swing_bodies == 0:
        model.rigid_bodies = []   # 揺れものが無ければ当たり判定も付けない
        model.joints = []
        report.collision_bodies = 0
        return report
    builder.symmetrize()
    builder.relax_initial_overlaps()
    return report
