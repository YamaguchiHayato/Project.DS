# -*- coding: utf-8 -*-
"""髪・スカート・袖などの揺れものに、剛体とジョイントを自動で付ける。

FBX には物理演算の情報が入っていないため、ボーン名と頂点の付き方から
叩き台を作る。揺れ具合は好みが分かれるので PMXEditor で調整する前提。

- 当たり判定: 頭・首・上半身・肩・胸・下半身・脚・腕にボーン追従のカプセル(球)。
  太さは体の表面に近づける(髪が乗る肩・前に垂れる髪が当たる胸も付ける)
- 揺れもの: 名前で判定したボーンごとに物理演算のカプセル + ジョイント
- 大きさ: そのボーンに主に乗っている頂点の広がりから決める
- 初期状態で当たり判定に食い込む揺れもの剛体は細くして隙間を作る(重なったままだと
  MMD で暴れるため)。細くしきれないときだけ、その相手グループとぶつけない

座標は PMX(MMD) 空間。剛体の回転は MMD と同じく Ry * Rx * Rz の順。

ゲームのエンジン(k2Engine の PMXPhysics)はボーン追従の剛体を 75% に縮めて使うので、
engine_scale=0.75 を渡すと、縮められた後にちょうど狙った大きさになるよう大きめに書き出す。
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
GROUP_OUTER = 12    # 重ね着の外側の揺れもの(外側のスカートなど)。内側の層とだけぶつける
GROUP_INNER_ROOT = 13   # 重ね着の内側の付け根の段(腰で外側の付け根と重なるので、外側とはぶつけない)
# 外側の層とみなす名前(ボーン名の一部)
_OUTER_WORDS = ("outer", "over", "外")
# 揺れもの連鎖の付け根は付け根の当たり判定とぶつけない。腰に付くスカートの付け根は
# 上半身にも接しているので(体を曲げると押し出されて跳ね上がる)、そちらともぶつけない
_ANCHOR_NEIGHBORS = {GROUP_LOWER: (GROUP_BODY,), GROUP_BODY: (GROUP_LOWER,)}

# 当たり判定を付ける標準ボーンとグループ(首は頭と同じグループにして、髪の付け根とは
# ぶつけない。肩は髪が乗るので胴と同じグループ)
_COLLISION_BONES = (
    ("頭", GROUP_HEAD), ("首", GROUP_HEAD),
    ("上半身", GROUP_BODY), ("上半身2", GROUP_BODY), ("上半身3", GROUP_BODY),
    ("左肩", GROUP_BODY), ("右肩", GROUP_BODY),
    ("下半身", GROUP_LOWER),
    ("左足", GROUP_LEG), ("左ひざ", GROUP_LEG), ("右足", GROUP_LEG), ("右ひざ", GROUP_LEG),
    ("左腕", GROUP_ARM), ("左ひじ", GROUP_ARM), ("右腕", GROUP_ARM), ("右ひじ", GROUP_ARM),
)
# 当たり判定のカプセルの、次の端点にするボーン
_COLLISION_ENDS = {
    "首": ("頭",), "左肩": ("左腕",), "右肩": ("右腕",),
    "上半身": ("上半身2", "上半身3", "首"), "上半身2": ("上半身3", "首"), "上半身3": ("首",),
    "左足": ("左ひざ",), "左ひざ": ("左足首",), "右足": ("右ひざ",), "右ひざ": ("右足首",),
    "左腕": ("左ひじ",), "左ひじ": ("左手首",), "右腕": ("右ひじ",), "右ひじ": ("右手首",),
}
# 当たり判定は体の表面に近い太さにする(静止状態で食い込む揺れもの側を細くして隙間を作る)。
# 首・肩・腕・胸は頂点までの距離の中央値のやや内側、脚は中央値そのもの
_COLLISION_SHRINK = 0.95
_LEG_SHRINK = 1.5
# 胴は前後より左右に広いので、後ろ側(髪が垂れる側)の頂点までの距離で太さを決める
_TORSO_BONES = ("上半身", "上半身2", "上半身3")
# 頭の球は髪が乗る頭皮より内側に収める(中心から近い頂点の下位この割合の距離を半径にする)
_HEAD_RADIUS_PERCENTILE = 0.01
# 当たり判定との隙間(揺れもの剛体の半径をこの割合だけ控えめにする)
_FIT_MARGIN = 0.95
# ジョイントの回転の制限のうち、体の中心へ向かって振れる側に残す割合。後ろ髪のように節の
# 多い連鎖が前へ回り込んで体や腕を突き抜けないよう、体のほうへはほとんど振らせない
_INWARD_LIMIT_RATIO = 0.2
_INWARD_LIMIT_MAX = math.radians(5.0)    # 体のほうへ振れる側はどの種類でもこれまで
# ジョイントの移動の範囲(± この値。0 だとエンジンが遊びを足すので、わずかに開ける)
_JOINT_TRANSLATION_EPSILON = 1e-4


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
    # 前髪は顔のすぐ前の短い髪なので、振れ幅を小さく、ばねを強くして形を保つ
    # (重力が強いエンジンでも垂れ下がって目にかからないように)
    SwingKind("前髪", ("hair_front", "front_hair", "fronthair", "hairfront", "前髪", "bangs",
                      "fringe", "maegami"), 5, (10.0, 5.0, 10.0), 600.0, 0.95),
    SwingKind("髪", ("hair", "髪", "kami", "pony", "twintail", "ahoge",
                    "もみあげ", "後髪", "横髪"), 5, (25.0, 10.0, 25.0), 5.0, 0.9),
    SwingKind("スカート", ("skirt", "スカート", "dress", "cape", "cloak", "mantle", "マント",
                       "coat", "コート"), 6, (90.0, 5.0, 60.0), 10.0, 0.9),
    SwingKind("袖", ("sode", "sleeve", "袖", "cuff"), 8, (20.0, 5.0, 20.0), 20.0, 0.9),
    SwingKind("揺れもの", ("tail", "しっぽ", "尻尾", "string", "紐", "tassel", "frill",
                       "フリル", "chain"), 9, (20.0, 10.0, 20.0), 10.0, 0.9),
)
# 胸揺れ(--bust-physics のときだけ使う)。小さく速く揺れて戻るよう、硬いばねと狭い制限
# ばねはゲームのエンジン(重力 -120・質量 3 倍)でも垂れ下がらず、動くと揺れる強さ
BUST_KIND = SwingKind("胸", ("bust", "breast", "oppai", "mune", "胸", "おっぱい"), 11,
                      (4.0, 4.0, 4.0), 1500.0, 0.99)
_SWING_GROUPS = {kind.group for kind in SWING_KINDS} | {BUST_KIND.group, GROUP_OUTER,
                                                        GROUP_INNER_ROOT}


class PhysicsReport:
    def __init__(self):
        self.collision_bodies = 0
        self.swing_bodies = 0
        self.joints = 0
        self.chains = {}           # 種類名 -> 連鎖の数
        self.shrunk = 0            # 当たり判定に食い込まないよう細くした剛体の数
        self.layered = 0           # 胸の揺れに合わせて重みを写した、重ね着の頂点の数
        self.relaxed = 0           # 細くしきれず衝突を外した組の数


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

def swing_kind(bone, kinds=SWING_KINDS):
    """ボーン名から揺れものの種類を返す。標準ボーンや IK は対象外。"""
    if bone.flags & pmxfile.BONE_IK:
        return None
    original = bone.name_en or bone.name
    if bonenames.to_mmd_name(original) is not None:
        return None
    text = (original + " " + bone.name).lower()
    for kind in kinds:
        if any(keyword in text for keyword in kind.keywords):
            return kind
    return None


class _Builder:
    def __init__(self, model, report, kinds=SWING_KINDS):
        self.model = model
        self.report = report
        self.kinds = kinds
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
    def _points(self, bone_index):
        """ボーンに乗る頂点。D 系やひねり補助へウェイトを移した分も含める。"""
        bone = self.bones[bone_index]
        points = list(self.main_vertices.get(bone_index, ()))
        twin = self.index_of.get(bone.name + "D")
        if twin is not None:
            points.extend(self.main_vertices.get(twin, ()))
        for child in self.children.get(bone_index, ()):
            if "捩" in self.bones[child].name:
                points.extend(self.main_vertices.get(child, ()))
        return points

    def _radius(self, bone_index, start, end, fallback, percentile=0.5):
        """ボーンに乗る頂点から線分までの距離の分位点(既定は中央値)。"""
        points = self._points(bone_index)
        if not points:
            return fallback
        distances = sorted(_point_segment_distance(p, start, end) for p in points)
        return distances[int(len(distances) * percentile)]

    def _back_radius(self, bone_index, start, end, fallback):
        """線分より後ろ(+Z)にある頂点までの距離の中央値。少なければ全体の中央値の 3/4。"""
        points = self._points(bone_index)
        axis = _sub(end, start)
        denominator = _dot(axis, axis) or 1.0
        behind = []
        for p in points:
            t = max(0.0, min(1.0, _dot(_sub(p, start), axis) / denominator))
            closest = _add(start, _scale(axis, t))
            if p[2] > closest[2]:
                behind.append(_length(_sub(p, closest)))
        if len(behind) < 8:
            return self._radius(bone_index, start, end, fallback) * 0.75
        behind.sort()
        return behind[len(behind) // 2]

    def _add_body(self, body, shape):
        self.model.rigid_bodies.append(body)
        self.shapes.append(shape)
        index = len(self.model.rigid_bodies) - 1
        if body.bone_index >= 0:
            self.body_of_bone.setdefault(body.bone_index, index)
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
                # 重心は細かい顔に引っ張られて前に出るので、頂点の範囲の中ほどを中心にする
                center = []
                for k in range(3):
                    values = sorted(p[k] for p in points)
                    center.append((values[len(values) // 20] + values[len(values) * 19 // 20]) / 2.0)
                center = tuple(center)
                distances = sorted(_length(_sub(p, center)) for p in points)
                radius = max(distances[int(len(distances) * _HEAD_RADIUS_PERCENTILE)] * _FIT_MARGIN,
                             self.unit * 3)
                body, shape = self._capsule(name, "head", index, center, center, radius)
            elif name == "下半身":
                left = self.index_of.get("左足")
                right = self.index_of.get("右足")
                if left is None or right is None:
                    continue
                a, b = bones[right].position, bones[left].position
                width = _length(_sub(b, a))
                # 股関節の間に渡すカプセルは両端が腰の横へはみ出しやすいので、
                # 中央値ではなく近い側(下位 5%)の頂点までの距離に収める
                radius = self._radius(index, a, b, width * 0.5, 0.05)
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
                if name in _TORSO_BONES:
                    radius = self._back_radius(index, start, end, length * 0.25)
                else:
                    shrink = _LEG_SHRINK if group == GROUP_LEG else _COLLISION_SHRINK
                    # 頂点が乗っていないボーンは仮の太さ(長さの 1/4)をそのまま使う
                    radius = self._radius(index, start, end, None)
                    radius = length * 0.25 if radius is None else radius * shrink
                radius = max(min(radius, length * 0.6), self.unit)
                body, shape = self._capsule(name, bones[index].name_en or name, index,
                                            start, end, radius)
            body.group = group
            body.collision_mask = swing_mask
            body.mode = pmxfile.BODY_FOLLOW_BONE
            self._add_body(body, shape)
            self.report.collision_bodies += 1
        self._build_bust_collisions(swing_mask, BUST_KIND in self.kinds)

    def _build_bust_collisions(self, swing_mask, bust_swings=False):
        """胸(bust / breast / 胸 など)に当たり判定を付ける(前に垂れる髪が沈まないように)。

        bust_swings: 胸を揺らすときは、当たり判定を胸の付け根(親のボーン)に付ける
        """
        bones = self.bones
        for index, bone in enumerate(bones):
            if swing_kind(bone, (BUST_KIND,)) is None:
                continue
            parent = bone.parent_index
            if parent >= 0 and swing_kind(bones[parent], (BUST_KIND,)) is not None:
                continue                              # 先端側のボーンは根元にまとめる
            tips = [c for c in self.children.get(index, ())
                    if swing_kind(bones[c], (BUST_KIND,)) is not None]
            if not tips:
                continue
            start, end = bone.position, bones[tips[0]].position
            points = self._points(index) + self._points(tips[0])
            if not points or _length(_sub(end, start)) < 1e-6:
                continue
            distances = sorted(_point_segment_distance(p, start, end) for p in points)
            radius = max(distances[len(distances) // 2] * _COLLISION_SHRINK, self.unit)
            owner = parent if bust_swings and parent >= 0 else index
            body, shape = self._capsule(bone.name + "判定" if bust_swings else bone.name,
                                        bone.name_en, owner, start, end, radius)
            body.group = GROUP_BODY
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
            kind = swing_kind(bone, self.kinds)
            parent = bone.parent_index
            if kind is None and parent in kinds and index in self.weighted:
                kind = kinds[parent]  # 揺れものの子は同じ種類として扱う
            if kind is not None and self._subtree_weighted(index):
                kinds[index] = kind

        segments = {}
        depth = {}
        outer_kinds = {kinds[i] for i in kinds if self._is_outer(bones[i])}
        centers = {}                             # 連鎖の付け根の当たり判定の中心(体の中心の目安)
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
            # 幅のある布(スカートの板など)は頂点の広がりが大きく出るので、太くしすぎない
            radius = max(min(radius, length * 0.35), length * 0.1, self.unit * 0.5)

            bone = bones[index]
            body, shape = self._capsule(bone.name, bone.name_en, index, start, end, radius)
            body.group = kind.group
            outer = self._is_outer(bone)
            if outer:
                body.group = GROUP_OUTER
            elif kind in outer_kinds and is_root:
                body.group = GROUP_INNER_ROOT
            body.mode = pmxfile.BODY_PHYSICS_ALIGNED
            body.mass = max(0.2, 0.8 ** depth[index])
            body.linear_damping = 0.5
            body.angular_damping = kind.angular_damping
            if kind is BUST_KIND:
                # 胸は軽く、強く減衰させて小さく揺らす(ゲームのエンジンで確かめた値。重いと
                # 動きに遅れて大きく振れ、肌がネクタイや襟の上へはみ出す)
                body.mass = 0.05
                body.linear_damping = 0.99
            body.restitution = 0.0
            body.friction = 0.5
            mask = 0xFFFF
            for group in _SWING_GROUPS:
                mask &= ~(1 << group)           # 揺れもの同士はぶつけない
            # ただし重ね着の外側と内側(同じ種類)の下の段同士はぶつけて、外側が内側を突き抜けない
            # ようにする(腰の付け根の段同士は最初から重なっているのでぶつけない)
            if outer and not is_root:
                mask |= 1 << kind.group
            elif kind in outer_kinds and not outer and not is_root:
                mask |= 1 << GROUP_OUTER
            parent_body = self._anchor_body(parent) if is_root else self.body_of_bone[parent]
            if is_root:
                anchor = self.shapes[parent_body] if parent_body is not None else None
                centers[index] = _scale(_add(anchor.start, anchor.end), 0.5) if anchor \
                    else bones[parent].position if parent >= 0 else start
            else:
                centers[index] = centers[parent]
            if is_root and parent_body is not None:
                anchor_group = self.model.rigid_bodies[parent_body].group
                mask &= ~(1 << anchor_group)    # 付け根の当たり判定とはぶつけない
                for group in _ANCHOR_NEIGHBORS.get(anchor_group, ()):
                    mask &= ~(1 << group)
                self.report.chains[kind.name] = self.report.chains.get(kind.name, 0) + 1
            body.collision_mask = mask
            body_index = self._add_body(body, shape)
            self.report.swing_bodies += 1

            if parent_body is not None:
                joint = pmxfile.PmxJoint(bone.name, bone.name_en, parent_body, body_index)
                joint.position = start
                joint.rotation_min, joint.rotation_max = self._limits(kind, start, end,
                                                                      centers[index])
                joint.spring_rotation = (kind.spring, kind.spring, kind.spring)
                # 移動の範囲を 0 ちょうどにすると、k2Engine はジョイントに ±0.2 の遊びを足し、
                # 重力で剛体がずり落ちる(前髪が目にかかる・スカートが脚へ落ちる)。
                # ごくわずかに開けておけば遊びは足されず、実質固定のまま
                joint.translation_min = (-_JOINT_TRANSLATION_EPSILON,) * 3
                joint.translation_max = (_JOINT_TRANSLATION_EPSILON,) * 3
                self.model.joints.append(joint)
                self.report.joints += 1

    @staticmethod
    def _is_outer(bone):
        return any(word in (bone.name_en + " " + bone.name).lower() for word in _OUTER_WORDS)

    def _limits(self, kind, start, end, center):
        """ジョイントの回転の制限 (最小, 最大)。体の中心へ向かって振れる側だけ狭くする。

        ジョイントの軸は静止状態のワールド軸。X 軸・Z 軸まわりの回転の制限のうち、先端が
        体の中心へ向かう側を狭める(向きの符号はエンジンの Bullet で確かめたもの)。
        """
        low = [-v for v in kind.limit]
        high = list(kind.limit)
        direction = _sub(end, start)
        inward = (center[0] - start[0], 0.0, center[2] - start[2])
        if _length(direction) < 1e-6 or _length(inward) < self.unit * 0.5:
            return tuple(low), tuple(high)
        direction = mat4.normalize(direction)
        inward = mat4.normalize(inward)
        for k, axis in ((0, (1.0, 0.0, 0.0)), (2, (0.0, 0.0, 1.0))):
            motion = (axis[1] * direction[2] - axis[2] * direction[1],
                      axis[2] * direction[0] - axis[0] * direction[2],
                      axis[0] * direction[1] - axis[1] * direction[0])
            # k2Engine の Bullet(btGeneric6DofSpringConstraint)で確かめると、制限の + 側は
            # ここで求めた向きと逆に効く。そのため体へ向かう向きの符号を反転して判定する
            toward = -_dot(motion, inward)
            if toward > 0.3:
                high[k] = min(kind.limit[k] * _INWARD_LIMIT_RATIO, _INWARD_LIMIT_MAX)
            elif toward < -0.3:
                low[k] = -min(kind.limit[k] * _INWARD_LIMIT_RATIO, _INWARD_LIMIT_MAX)
        return tuple(low), tuple(high)

    # -- 左右をそろえる --------------------------------------------------
    def symmetrize(self, smaller=False):
        """左右対になる剛体(J_L_xxx / J_R_xxx など)の半径をそろえる。

        smaller: 平均ではなく細いほうに合わせる(当たり判定に合わせて細くした後に使う)
        """
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
            sizes = [self.model.rigid_bodies[i].size[0] for i in indexes]
            radius = min(sizes) if smaller else sum(sizes) / 2.0
            for i in indexes:
                body = self.model.rigid_bodies[i]
                body.size = (radius,) + tuple(body.size[1:])
                self.shapes[i].radius = radius

    # -- 初期状態の重なり ------------------------------------------------
    def relax_initial_overlaps(self):
        """静止状態で当たり判定に食い込んでいる揺れもの剛体を細くする。

        以前は重なった相手のグループ(脚なら両脚まとめて)とぶつけないようにしていたため、
        スカートが脚を、髪が顔をすり抜けていた。今は半径を隙間に合わせて細くし、
        ボーンの線そのものが当たり判定の中にあって細くしきれないときだけ衝突を外す。
        """
        bodies = self.model.rigid_bodies
        for i, body in enumerate(bodies):
            if body.mode == pmxfile.BODY_FOLLOW_BONE:
                continue
            shape = self.shapes[i]
            floor = max(self.unit * 0.3, shape.radius * 0.3)
            original = shape.radius
            for j, other in enumerate(bodies):
                if i == j:
                    continue
                # 揺れもの同士(重ね着の外側と内側)は、外側のほうを細くして隙間を作る
                if other.mode != pmxfile.BODY_FOLLOW_BONE and body.group != GROUP_OUTER:
                    continue
                if not body.collision_mask & (1 << other.group):
                    continue
                if not other.collision_mask & (1 << body.group):
                    continue
                target = self.shapes[j]
                gap = segment_distance(shape.start, shape.end, target.start, target.end)                     - target.radius
                allowed = gap * _FIT_MARGIN
                if allowed >= floor:
                    shape.radius = min(shape.radius, allowed)
                else:
                    body.collision_mask &= ~(1 << other.group)
                    self.report.relaxed += 1
            if shape.radius < original:
                body.size = (shape.radius,) + tuple(body.size[1:])
                self.report.shrunk += 1


def sync_layer_weights(model, bones, reach=0.25):
    """揺らすボーン(胸など)の上に重なった服の頂点に、すぐ下の体の頂点の重みを写す。

    元のモデルは胸を揺らさない前提で、体の頂点は胸のボーンに 100% 乗っているのに、
    上に重なる服は半分ほどしか乗っていないことがある。そのまま揺らすと肌だけ大きく動いて
    服から飛び出すので、揺れるボーンへの重みが近くの頂点(reach 以内)より小さい頂点は、
    その近くの頂点と同じ重みにする。写した頂点の数を返す。
    """
    if not bones:
        return 0
    vertices = model.vertices

    def amount(vertex):
        return sum(w for b, w in zip(vertex.bones, vertex.weights) if b in bones)

    cell = reach
    grid = {}
    strong = []
    for i, vertex in enumerate(vertices):
        a = amount(vertex)
        if a > 0.0:
            strong.append((i, a))
            key = tuple(int(c // cell) for c in vertex.position)
            grid.setdefault(key, []).append(i)
    # 面でつながった部品のうち、もともと揺らすボーンに少しでも乗っているものだけを対象にする
    # (ネクタイや襟のような別の部品まで胸と一緒に動かして、体に埋めないように)
    parent = list(range(len(vertices)))

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for a_, b_, c_ in model.faces:
        parent[find(a_)] = find(b_)
        parent[find(c_)] = find(b_)
    touched = {find(i) for i, _a in strong}
    copied = 0
    for i, vertex in enumerate(vertices):
        if find(i) not in touched:
            continue
        a = amount(vertex)
        position = vertex.position
        key = tuple(int(c // cell) for c in position)
        best = None
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for j in grid.get((key[0] + dx, key[1] + dy, key[2] + dz), ()):
                        other = vertices[j]
                        if amount(other) <= a + 0.05:
                            continue
                        d = sum((other.position[k] - position[k]) ** 2 for k in range(3))
                        if d <= reach * reach and (best is None or d < best[0]):
                            best = (d, j)
        if best is not None:
            source = vertices[best[1]]
            vertices[i].bones = list(source.bones)
            vertices[i].weights = list(source.weights)
            copied += 1
    return copied


# ゲームのエンジン(k2Engine の PMXPhysics)がボーン追従の剛体に掛ける縮小率
K2ENGINE_FOLLOW_SCALE = 0.75


def generate(model, bust=False, engine_scale=1.0):
    """モデルに剛体とジョイントを追加する。既にあれば何もしない。

    bust: 胸のボーン(bust / breast / 胸 など)にも揺れを付けるか
    engine_scale: 読み込む側がボーン追従の剛体に掛ける縮小率(k2Engine は 0.75、MMD は 1.0)。
        隙間の計算は狙った大きさで行い、書き出すときだけ 1 / engine_scale 倍にする
    """
    report = PhysicsReport()
    if model.rigid_bodies or model.joints:
        return report
    kinds = SWING_KINDS + (BUST_KIND,) if bust else SWING_KINDS
    builder = _Builder(model, report, kinds)
    builder.build_collisions()
    builder.build_swings()
    if report.swing_bodies == 0:
        model.rigid_bodies = []   # 揺れものが無ければ当たり判定も付けない
        model.joints = []
        report.collision_bodies = 0
        return report
    builder.symmetrize()
    builder.relax_initial_overlaps()
    builder.symmetrize(smaller=True)
    if bust:
        report.layered = sync_layer_weights(model, {
            i for i, bone in enumerate(model.bones)
            if swing_kind(bone, (BUST_KIND,)) is not None and i in builder.body_of_bone},
            reach=builder.unit * 2.0)
    if engine_scale and abs(engine_scale - 1.0) > 1e-9:
        for body in model.rigid_bodies:
            if body.mode == pmxfile.BODY_FOLLOW_BONE and body.group != GROUP_ANCHOR:
                body.size = tuple(c / engine_scale for c in body.size)
    return report
