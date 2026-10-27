# -*- coding: utf-8 -*-
"""変換した PMX を MMD の標準ボーン構造へ寄せる。

ネットで配布されているモーション(VMD)は、あにまさ式初音ミクに倣った
「標準ボーン」を前提に作られている。そのため次の点を整える。

- 全ての親 / センター / グルーブ / 上半身 / 下半身 の親子関係
- 両目(左目・右目に回転を付与する操作用ボーン)
- 足ＩＫ / つま先ＩＫ(つま先ボーンが無ければ足の形から補う)
- 腕の角度(T スタンスのモデルを A スタンスへ)
- 表情モーフ名(あ・い・う・え・お・まばたき など)
- 表示枠(センター / 体(上) / 腕 / 指 / 体(下) / 足 / その他)

座標はすべて PMX(MMD) 空間で扱う。
"""

from __future__ import annotations

import math
import re

import mat4
import pmxfile

# 足ＩＫ / つま先ＩＫ の設定(標準モデルを PMX 化したときの値)
LEG_IK_LOOP = 40
LEG_IK_LIMIT = 2.0          # 単位角(ラジアン) = 114.59 度
TOE_IK_LOOP = 3
TOE_IK_LIMIT = 4.0          # 単位角(ラジアン) = 229.18 度
KNEE_LIMIT_MIN = (-math.pi, 0.0, 0.0)
KNEE_LIMIT_MAX = (math.radians(-0.5), 0.0, 0.0)

# T スタンスとみなす腕の下がり角と、A スタンス化するときの既定角度
T_STANCE_THRESHOLD = 20.0
DEFAULT_ARM_ANGLE = 35.0

_OPERABLE = pmxfile.BONE_ROTATABLE | pmxfile.BONE_VISIBLE | pmxfile.BONE_ENABLED
_MOVABLE = _OPERABLE | pmxfile.BONE_TRANSLATABLE

# 表情モーフ名の対応表(比較用に均した名前 -> MMD 名)
_MORPH_NAMES = {
    "あ": ("mtha", "moutha", "a", "aa", "vaa", "fclmtha"),
    "い": ("mthi", "mouthi", "i", "ih", "vih", "fclmthi"),
    "う": ("mthu", "mouthu", "u", "ou", "vou", "fclmthu"),
    "え": ("mthe", "mouthe", "e", "eh", "veh", "fclmthe"),
    "お": ("mtho", "moutho", "o", "oh", "voh", "fclmtho"),
    "まばたき": ("eyeblk", "blink", "eyeblink", "eyeclose", "eyesclosed",
             "fcleyeclose", "blinkboth"),
    "笑い": ("eyesml", "eyesmile", "fcleyejoy"),
    "びっくり": ("eyesup", "eyesurprised", "fcleyesurprised"),
    "怒り": ("blwang", "brwang", "browangry", "fclbrwangry"),
    "困る": ("blwsad", "brwsad", "browsad", "fclbrwsorrow"),
    "にこり": ("blwsml", "brwsml", "browsmile", "fclbrwjoy"),
    "上": ("blwsup", "brwsup", "browup", "fclbrwsurprised"),
}
_MORPH_PANELS = {"あ": 3, "い": 3, "う": 3, "え": 3, "お": 3,
                 "まばたき": 2, "笑い": 2, "びっくり": 2,
                 "怒り": 1, "困る": 1, "にこり": 1, "上": 1}

# 表示枠の並び
_FRAME_LAYOUT = (
    ("センター", "Center", ("センター", "グルーブ", "腰")),
    ("体(上)", "Upper", ("上半身", "上半身2", "上半身3", "首", "頭", "両目", "左目", "右目")),
    ("腕", "Arms", ("左肩", "左腕", "左ひじ", "左手首", "右肩", "右腕", "右ひじ", "右手首")),
    ("指", "Fingers", tuple(
        side + finger + digit
        for side in ("左", "右")
        for finger, digits in (("親指", "０１２"), ("人指", "１２３"), ("中指", "１２３"),
                               ("薬指", "１２３"), ("小指", "１２３"))
        for digit in digits)),
    ("体(下)", "Lower", ("下半身",)),
    ("足", "Legs", ("左足ＩＫ", "左つま先ＩＫ", "右足ＩＫ", "右つま先ＩＫ",
                   "左足", "左ひざ", "左足首", "右足", "右ひざ", "右足首")),
)


class RigReport:
    """MMD 化の結果。"""

    def __init__(self):
        self.added_bones = []
        self.ik_chains = 0
        self.arm_rotation = 0.0
        self.renamed_morphs = 0
        self.warnings = []


class _Node:
    """編集中のボーン。親や IK の参照を番号ではなくオブジェクトで持つ。"""

    def __init__(self, bone, old_index=-1):
        self.bone = bone
        self.old_index = old_index
        self.parent = None
        self.tail = None
        self.inherit = None
        self.ik_target = None
        self.ik_links = []      # [(ノード, 制限あり, 下限, 上限)]
        self.ik_loop = 0
        self.ik_limit = 0.0

    @property
    def position(self):
        return self.bone.position

    @property
    def name(self):
        return self.bone.name


class _Rig:
    """PMX のボーン配列を編集し、最後に番号を振り直して書き戻す。"""

    def __init__(self, model):
        self.model = model
        self.nodes = [_Node(bone, index) for index, bone in enumerate(model.bones)]
        for node in self.nodes:
            bone = node.bone
            if 0 <= bone.parent_index < len(self.nodes):
                node.parent = self.nodes[bone.parent_index]
            if bone.flags & pmxfile.BONE_TAIL_IS_INDEX and 0 <= bone.tail_index < len(self.nodes):
                node.tail = self.nodes[bone.tail_index]
            if bone.flags & (pmxfile.BONE_INHERIT_ROTATION | pmxfile.BONE_INHERIT_TRANSLATION) \
                    and 0 <= bone.inherit_parent_index < len(self.nodes):
                node.inherit = self.nodes[bone.inherit_parent_index]
            if bone.flags & pmxfile.BONE_IK and bone.ik is not None:
                node.ik_target = self.nodes[bone.ik.target_index]
                node.ik_loop = bone.ik.loop_count
                node.ik_limit = bone.ik.limit_radian
                node.ik_links = [(self.nodes[link.bone_index], link.has_limit,
                                  link.limit_min, link.limit_max)
                                 for link in bone.ik.links]
        self.weighted = set()
        for vertex in model.vertices:
            for index, weight in zip(vertex.bones, vertex.weights):
                if weight > 0.0:
                    self.weighted.add(index)

    # -- 参照 --------------------------------------------------------
    def find(self, name):
        for node in self.nodes:
            if node.bone.name == name:
                return node
        return None

    def children(self, node):
        return [child for child in self.nodes if child.parent is node]

    def descendants(self, node):
        result = []
        stack = self.children(node)
        while stack:
            current = stack.pop()
            result.append(current)
            stack.extend(self.children(current))
        return result

    def is_ancestor(self, ancestor, node):
        current = node.parent
        while current is not None:
            if current is ancestor:
                return True
            current = current.parent
        return False

    def has_weights(self, node):
        return node.old_index in self.weighted

    # -- 編集 --------------------------------------------------------
    def add(self, name, name_en, position, parent, flags=_OPERABLE):
        bone = pmxfile.PmxBone(name, name_en, tuple(position), -1)
        bone.flags = flags
        node = _Node(bone)
        node.parent = parent
        self.nodes.append(node)
        return node

    # -- 書き戻し ----------------------------------------------------
    def _order(self):
        """親 -> 子の順に並べる。IK は連鎖のボーンの直後に置く。

        MMD はボーン番号順に変形するので、IK ボーンは対象と連鎖の
        ボーンより後ろ、連鎖から枝分かれした補助ボーン(ひざ当てなど)
        より前に置く必要がある。
        """
        kids = {id(node): [] for node in self.nodes}
        roots = []
        for node in self.nodes:
            if node.parent is None:
                roots.append(node)
            else:
                kids[id(node.parent)].append(node)

        iks = [node for node in self.nodes if node.ik_target is not None]
        in_chain = set()
        for ik in iks:
            in_chain.add(id(ik.ik_target))
            in_chain.update(id(link[0]) for link in ik.ik_links)

        def dependencies(node):
            result = [node.parent] if node.parent is not None else []
            if node.inherit is not None:
                result.append(node.inherit)       # 付与親は先に変形させる
            if node.ik_target is not None:
                result.append(node.ik_target)
                result.extend(link[0] for link in node.ik_links)
            return result

        def ready(node):
            return all(id(dep) in emitted for dep in dependencies(node))

        def sort_key(node):
            original = node.old_index if node.old_index >= 0 else len(self.nodes) + 1
            return (0 if id(node) in in_chain else 1, original, node.bone.name)

        order = []
        emitted = set()
        # 親以外にも依存先がある(IK・付与)ボーンは、依存先が揃った時点で出す
        pending = [node for node in self.nodes
                   if node.ik_target is not None or node.inherit is not None]

        def flush():
            progress = True
            while progress:
                progress = False
                for node in list(pending):
                    if node in pending and ready(node):
                        pending.remove(node)
                        emit(node)
                        progress = True

        def emit(node):
            order.append(node)
            emitted.add(id(node))
            flush()
            for child in sorted(kids[id(node)], key=sort_key):
                if id(child) in emitted or child in pending:
                    continue
                emit(child)

        for root in sorted(roots, key=sort_key):
            if id(root) not in emitted and root not in pending:
                emit(root)
        for node in list(pending) + self.nodes:  # 依存先が揃わなかったボーンなど
            if id(node) not in emitted:
                if node in pending:
                    pending.remove(node)
                emit(node)
        return order

    def commit(self):
        order = self._order()
        index_of = {id(node): index for index, node in enumerate(order)}
        remap = {node.old_index: index_of[id(node)] for node in order if node.old_index >= 0}

        bones = []
        for node in order:
            bone = node.bone
            bone.parent_index = index_of[id(node.parent)] if node.parent is not None else -1
            if node.tail is not None:
                bone.flags |= pmxfile.BONE_TAIL_IS_INDEX
                bone.tail_index = index_of[id(node.tail)]
            else:
                bone.flags &= ~pmxfile.BONE_TAIL_IS_INDEX
                bone.tail_index = -1
            if node.inherit is not None:
                bone.inherit_parent_index = index_of[id(node.inherit)]
            if node.ik_target is not None:
                bone.flags |= pmxfile.BONE_IK
                bone.ik = pmxfile.PmxIk(
                    index_of[id(node.ik_target)], node.ik_loop, node.ik_limit,
                    [pmxfile.PmxIkLink(index_of[id(link)], has_limit, low, high)
                     for link, has_limit, low, high in node.ik_links])
            bones.append(bone)
        self.model.bones = bones

        for vertex in self.model.vertices:
            vertex.bones = [remap.get(index, 0) for index in vertex.bones]


# --------------------------------------------------------------------------
# 各処理
# --------------------------------------------------------------------------

def _distance(a, b):
    return math.dist(a, b)


def _leg_length(rig):
    leg = rig.find("左足") or rig.find("右足")
    ankle = rig.find("左足首") or rig.find("右足首")
    if leg is not None and ankle is not None:
        return max(_distance(leg.position, ankle.position), 1e-3)
    ys = [v.position[1] for v in rig.model.vertices] or [0.0, 1.0]
    return max((max(ys) - min(ys)) * 0.45, 1e-3)


def _ensure_root(rig, report):
    """全ての親を根にする。無ければ原点に作る。"""
    root = rig.find("全ての親")
    if root is None:
        root = rig.add("全ての親", "master", (0.0, 0.0, 0.0), None, _MOVABLE)
        report.added_bones.append("全ての親")
    for node in rig.nodes:
        if node is not root and node.parent is None:
            node.parent = root
    root.parent = None
    root.bone.flags |= _MOVABLE
    return root


def _restructure_center(rig, root, report, unit):
    """センター / グルーブ / 上半身 / 下半身 の親子関係を標準に合わせる。"""
    left_leg = rig.find("左足")
    right_leg = rig.find("右足")
    upper = rig.find("上半身")
    hips = None
    if left_leg is not None and right_leg is not None and left_leg.parent is right_leg.parent:
        hips = left_leg.parent
    if hips is None or hips is root:
        hips = rig.find("センター")
    if hips is None or upper is None or hips is root:
        report.warnings.append("腰(Hips)か上半身が特定できないため、センター周りの組み替えを省略しました")
        return

    # 既存の「センター」名は一度外す(腰は下半身へ、それ以外は元の英語名へ戻す)
    for node in rig.nodes:
        if node.bone.name == "センター":
            node.bone.name = "下半身" if node is hips else (node.bone.name_en or "center_old")

    # 腰の親がウェイトを持たず位置もほぼ同じなら、それをセンターとして流用する
    center = None
    candidate = hips.parent
    if candidate is not None and candidate is not root and not rig.has_weights(candidate) \
            and _distance(candidate.position, hips.position) < unit * 0.5:
        center = candidate
    if center is None:
        center = rig.add("センター", "center", hips.position, hips.parent or root, _MOVABLE)
        hips.parent = center
        report.added_bones.append("センター")
    center.bone.name = "センター"
    center.bone.name_en = center.bone.name_en or "center"
    center.bone.flags |= _MOVABLE
    center.tail = None
    center.bone.tail_offset = (0.0, -unit, 0.0)

    groove = rig.find("グルーブ")
    if groove is None:
        groove = rig.add("グルーブ", "groove", center.position, center, _MOVABLE)
        report.added_bones.append("グルーブ")
    for child in rig.children(center):
        if child is not groove:
            child.parent = groove
    groove.tail = None
    groove.bone.tail_offset = (0.0, unit * 0.5, 0.0)

    hips.bone.name = "下半身"
    hips.bone.flags = (hips.bone.flags | _OPERABLE) & ~pmxfile.BONE_TRANSLATABLE
    hips.parent = groove
    if upper.parent is not groove:
        upper.parent = groove  # 上半身を下半身から切り離す(Mixamo 型の階層)

    # 見た目用: 下半身は脚の付け根の中点へ向ける
    if left_leg is not None and right_leg is not None:
        middle = tuple((left_leg.position[i] + right_leg.position[i]) * 0.5 for i in range(3))
        offset = tuple(middle[i] - hips.position[i] for i in range(3))
        if math.sqrt(sum(c * c for c in offset)) < unit * 0.1:
            offset = (0.0, -unit, 0.0)
        hips.tail = None
        hips.bone.tail_offset = offset


def _add_eye_controller(rig, report):
    """両目を作り、左目・右目へ回転を付与する。"""
    left = rig.find("左目")
    right = rig.find("右目")
    if left is None or right is None or rig.find("両目") is not None:
        return
    head = rig.find("頭") or left.parent
    spread = max(abs(left.position[0] - right.position[0]), 1e-3)
    position = ((left.position[0] + right.position[0]) * 0.5,
                max(left.position[1], right.position[1]) + spread * 1.5,
                (left.position[2] + right.position[2]) * 0.5)
    both = rig.add("両目", "eyes", position, head, _OPERABLE)
    both.bone.tail_offset = (0.0, 0.0, -spread)
    report.added_bones.append("両目")
    for eye in (left, right):
        eye.inherit = both
        eye.bone.inherit_rate = 1.0
        eye.bone.flags |= pmxfile.BONE_INHERIT_ROTATION


def _toe_position(rig, ankle, leg_length):
    """足首にウェイトが乗っている頂点から、つま先の位置を推定する。"""
    model = rig.model
    candidates = {ankle.old_index}
    candidates.update(node.old_index for node in rig.descendants(ankle) if node.old_index >= 0)
    points = []
    for vertex in model.vertices:
        count = min(len(vertex.bones), len(vertex.weights))
        if count == 0:
            continue
        main = max(range(count), key=lambda k: vertex.weights[k])
        if vertex.bones[main] in candidates:
            points.append(vertex.position)
    if points:
        front = min(p[2] for p in points)          # MMD の正面は -Z
        bottom = min(p[1] for p in points)
        return (ankle.position[0], max(bottom, 0.0), front)
    return (ankle.position[0], 0.0, ankle.position[2] - leg_length * 0.25)


def _add_leg_ik(rig, root, report, unit):
    """左右の足ＩＫ・つま先ＩＫを作る。"""
    leg_length = _leg_length(rig)
    for side in ("左", "右"):
        if rig.find(side + "足ＩＫ") is not None:
            continue
        leg = rig.find(side + "足")
        knee = rig.find(side + "ひざ")
        ankle = rig.find(side + "足首")
        if leg is None or knee is None or ankle is None:
            report.warnings.append("%s足の足/ひざ/足首が揃わないため足ＩＫを作れませんでした" % side)
            continue
        if not (rig.is_ancestor(leg, knee) and rig.is_ancestor(knee, ankle)):
            report.warnings.append("%s足の足→ひざ→足首が親子になっていないため足ＩＫを省略しました" % side)
            continue

        toe = rig.find(side + "つま先")
        if toe is None:
            toe = rig.add(side + "つま先", ("toe_L" if side == "左" else "toe_R"),
                          _toe_position(rig, ankle, leg_length), ankle,
                          pmxfile.BONE_ROTATABLE | pmxfile.BONE_ENABLED)
            report.added_bones.append(side + "つま先")
        if ankle.tail is None:
            ankle.tail = toe

        leg_ik = rig.add(side + "足ＩＫ", "leg IK_" + ("L" if side == "左" else "R"),
                         ankle.position, root, _MOVABLE | pmxfile.BONE_IK)
        leg_ik.bone.tail_offset = (0.0, 0.0, unit)
        leg_ik.ik_target = ankle
        leg_ik.ik_loop = LEG_IK_LOOP
        leg_ik.ik_limit = LEG_IK_LIMIT
        leg_ik.ik_links = [(knee, True, KNEE_LIMIT_MIN, KNEE_LIMIT_MAX),
                           (leg, False, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0))]

        toe_ik = rig.add(side + "つま先ＩＫ", "toe IK_" + ("L" if side == "左" else "R"),
                         toe.position, leg_ik, _MOVABLE | pmxfile.BONE_IK)
        toe_ik.bone.tail_offset = (0.0, -unit, 0.0)
        toe_ik.ik_target = toe
        toe_ik.ik_loop = TOE_IK_LOOP
        toe_ik.ik_limit = TOE_IK_LIMIT
        toe_ik.ik_links = [(ankle, False, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0))]

        report.added_bones.extend([side + "足ＩＫ", side + "つま先ＩＫ"])
        report.ik_chains += 2


def _arm_angle(rig, side):
    arm = rig.find(side + "腕")
    end = rig.find(side + "手首") or rig.find(side + "ひじ")
    if arm is None or end is None:
        return None
    dx = abs(end.position[0] - arm.position[0])
    dy = end.position[1] - arm.position[1]
    return math.degrees(math.atan2(-dy, dx))


def _rotate_arms(rig, target_angle, report):
    """腕の付け根を軸に、腕から先を回して A スタンスにする。"""
    model = rig.model
    total = 0.0
    for side in ("左", "右"):
        current = _arm_angle(rig, side)
        if current is None:
            continue
        delta = target_angle - current
        if abs(delta) < 0.5:
            continue
        arm = rig.find(side + "腕")
        nodes = [arm] + rig.descendants(arm)
        affected = {node.old_index for node in nodes if node.old_index >= 0}
        # 左腕は +X 側なので時計回り、右腕は反時計回りに回すと下がる
        radians = math.radians(-delta if side == "左" else delta)
        pivot = arm.position
        cos_a, sin_a = math.cos(radians), math.sin(radians)

        def rotate_point(p):
            x, y = p[0] - pivot[0], p[1] - pivot[1]
            return (pivot[0] + x * cos_a - y * sin_a, pivot[1] + x * sin_a + y * cos_a, p[2])

        def rotate_vector(v):
            return (v[0] * cos_a - v[1] * sin_a, v[0] * sin_a + v[1] * cos_a, v[2])

        def blend(a, b, w):
            return tuple(a[i] * (1.0 - w) + b[i] * w for i in range(3))

        vertex_share = {}
        for index, vertex in enumerate(model.vertices):
            share = sum(weight for bone, weight in zip(vertex.bones, vertex.weights)
                        if bone in affected)
            if share <= 1e-6:
                continue
            vertex_share[index] = share
            vertex.position = blend(vertex.position, rotate_point(vertex.position), share)
            vertex.normal = mat4.normalize(
                blend(vertex.normal, rotate_vector(vertex.normal), share))
        for morph in model.morphs:
            if morph.morph_type != pmxfile.MORPH_VERTEX:
                continue
            morph.offsets = [
                (index, blend(delta, rotate_vector(delta), vertex_share[index]))
                if index in vertex_share else (index, delta)
                for index, delta in morph.offsets]
        for node in nodes:
            if node is not arm:
                node.bone.position = rotate_point(node.bone.position)
            if node.tail is None:
                node.bone.tail_offset = rotate_vector(node.bone.tail_offset)
        total = max(total, abs(delta))
    report.arm_rotation = total


def _canonical_morph(name):
    text = name.lower()
    text = re.sub(r"shape$", "", text)
    text = re.sub(r"^(blendshape\d*\.|blendshape\d*_)", "", text)
    return re.sub(r"[^a-z0-9]", "", text)


def rename_morphs(model, extra_map=None):
    """表情モーフ名を MMD の標準名へ寄せる。変換した数を返す。"""
    lookup = {}
    for mmd_name, keys in _MORPH_NAMES.items():
        for key in keys:
            lookup[key] = mmd_name
    used = {morph.name for morph in model.morphs}
    renamed = 0
    for morph in model.morphs:
        target = None
        if extra_map:
            target = extra_map.get(morph.name)
        if target is None:
            target = lookup.get(_canonical_morph(morph.name))
        if not target or target == morph.name or target in used:
            continue
        used.add(target)
        morph.name_en = morph.name_en or morph.name
        morph.name = target
        morph.panel = _MORPH_PANELS.get(target, morph.panel)
        renamed += 1
    return renamed


# (元のモーフ, 左目用の名前, 右目用の名前)。MMD の「右」はモデルから見た右(-X 側)
_WINK_SOURCES = (
    ("まばたき", ("ウィンク", "Wink"), ("ウィンク右", "Wink_R")),
    ("笑い", ("ウィンク２", "Wink2"), ("ウィンク２右", "Wink2_R")),
)


def add_winks(model):
    """まばたき・笑いを左右の目に分けて、ウィンク系のモーフを作る。作った数を返す。"""
    names = {morph.name for morph in model.morphs}
    created = 0
    for source_name, (left_name, left_en), (right_name, right_en) in _WINK_SOURCES:
        source = next((m for m in model.morphs
                       if m.name == source_name and m.morph_type == pmxfile.MORPH_VERTEX), None)
        if source is None or left_name in names or right_name in names:
            continue
        left = pmxfile.PmxMorph(left_name, left_en, 2, pmxfile.MORPH_VERTEX)
        right = pmxfile.PmxMorph(right_name, right_en, 2, pmxfile.MORPH_VERTEX)
        for index, delta in source.offsets:
            x = model.vertices[index].position[0]
            if x > 1e-6:          # モデルの左目(+X 側)
                left.offsets.append((index, delta))
            elif x < -1e-6:
                right.offsets.append((index, delta))
            else:                 # 中央の頂点は両方に半分ずつ
                half = tuple(c * 0.5 for c in delta)
                left.offsets.append((index, half))
                right.offsets.append((index, half))
        if left.offsets and right.offsets:
            model.morphs.extend([left, right])
            names.update((left_name, right_name))
            created += 2
    return created


def build_display_frames(model):
    """MMD でよく見る並びの表示枠を作る。"""
    index_of = {bone.name: index for index, bone in enumerate(model.bones)}
    root = pmxfile.PmxDisplayFrame("Root", "Root", True)
    root.elements.append((pmxfile.FRAME_ELEMENT_BONE, index_of.get("全ての親", 0)))
    expression = pmxfile.PmxDisplayFrame("表情", "Exp", True)
    expression.elements = [(pmxfile.FRAME_ELEMENT_MORPH, i) for i in range(len(model.morphs))]
    frames = [root, expression]

    placed = {root.elements[0][1]}
    for name, name_en, members in _FRAME_LAYOUT:
        frame = pmxfile.PmxDisplayFrame(name, name_en, False)
        for member in members:
            index = index_of.get(member)
            if index is not None and index not in placed:
                frame.elements.append((pmxfile.FRAME_ELEMENT_BONE, index))
                placed.add(index)
        if frame.elements:
            frames.append(frame)

    others = pmxfile.PmxDisplayFrame("その他", "Others", False)
    for index, bone in enumerate(model.bones):
        if index in placed or not bone.flags & pmxfile.BONE_VISIBLE:
            continue
        others.elements.append((pmxfile.FRAME_ELEMENT_BONE, index))
    if others.elements:
        frames.append(others)
    model.display_frames = frames


def is_humanoid(model):
    """人型の骨格(上半身か左右の足)を持つか。小物や武器には MMD 化をかけない。"""
    names = {bone.name for bone in model.bones}
    return "上半身" in names or ("左足" in names and "右足" in names)


def standardize(model, arm_angle="auto", log=None):
    """PMX を MMD 標準ボーン構造へ寄せる。RigReport を返す。

    arm_angle: "auto"(T スタンスのときだけ A スタンス化) / "off" / 角度(度)
    """
    log = log or (lambda message: None)
    report = RigReport()
    rig = _Rig(model)
    unit = _leg_length(rig) * 0.15

    root = _ensure_root(rig, report)
    _restructure_center(rig, root, report, unit)
    _add_eye_controller(rig, report)
    _add_leg_ik(rig, root, report, unit)

    if arm_angle != "off":
        current = [a for a in (_arm_angle(rig, "左"), _arm_angle(rig, "右")) if a is not None]
        if current:
            average = sum(current) / len(current)
            if arm_angle == "auto":
                if average < T_STANCE_THRESHOLD:
                    _rotate_arms(rig, DEFAULT_ARM_ANGLE, report)
                log("  腕の角度: 水平から %.1f 度下がり%s" % (
                    average, (" → %.0f 度へ A スタンス化" % DEFAULT_ARM_ANGLE)
                    if report.arm_rotation else "(変更なし)"))
            else:
                _rotate_arms(rig, float(arm_angle), report)
                log("  腕の角度: %.1f 度 → %.1f 度" % (average, float(arm_angle)))

    rig.commit()
    build_display_frames(model)
    return report
