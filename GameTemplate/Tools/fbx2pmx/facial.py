# -*- coding: utf-8 -*-
"""表情モーフの無いモデル(ゲームの tkm など)に、基本の表情モーフを顔の形から作る。

ダンスなどの VMD は「まばたき」「あ」などの名前で表情を動かすので、モデルに同じ名前の
モーフが無いと表情が固まったままになる。目のボーン(左目・右目)の位置と、そこに乗る
瞳の頂点から目の大きさを測り、次のモーフを作る。

- まばたき / 笑い: 目の奥を通る左右方向の軸で上まぶたを下まぶたまで回して閉じる
  (笑いは閉じた線が上へ弧を描く)。白目は下まぶたへ押し縮め、瞳は縮めて奥へ下げる。
  ウィンク系はこの後 mmdize.add_winks が左右に分けて作る
- あ・い・う・え・お: 口の下側を顎の軸で回して下げ、口角を左右へ広げ・すぼめる。
  口角を少し上げて、怖く見えない柔らかい口元にする
- 口角上げ・口角下げ・にやり・まるい口・ワ: 口角の上げ下げと口の開きの組み合わせ
- 眉 上・下・にこり・困る・怒り・真面目: 目の上にある小さな部品(眉)を動かす
- 柔らかな笑み: 口角上げと眉のにこりを合わせたもの(目は開けたまま)
- ぺろっ: 口の前へ飛び出している小さな部品(舌)があれば、普段は口の中へしまい、
  このモーフで出す

目や口の作りはモデルごとに違うので、あくまで叩き台(PMXEditor で調整する前提)。
座標は PMX(MMD) 空間で、正面は -Z。
"""

from __future__ import annotations

import math

import pmxfile

# 母音ごとの (顎の開き 0〜1, 口の幅 +広げる / -すぼめる, 口角の上げ)
_VOWELS = (
    ("あ", "a", 1.0, 0.0, 0.3),
    ("い", "i", 0.25, 0.3, 0.6),
    ("う", "u", 0.3, -0.35, 0.0),
    ("え", "e", 0.55, 0.15, 0.4),
    ("お", "o", 0.75, -0.25, 0.0),
)
# 口の表情 (名前, 英名, 顎の開き, 口の幅, 口角の上げ)
_MOUTH_SHAPES = (
    ("口角上げ", "mouth up", 0.0, 0.05, 1.0),
    ("口角下げ", "mouth down", 0.0, 0.0, -0.8),
    ("にやり", "grin", 0.1, 0.2, 1.0),
    ("まるい口", "round mouth", 0.5, -0.4, 0.0),
    ("ワ", "wa", 1.0, 0.3, 0.5),
)
# 眉の表情 (名前, 英名, 全体の上下, 内側の上下, 弧)。単位は両目の間隔
_BROWS = (
    ("上", "brow up", 0.1, 0.0, 0.0),
    ("下", "brow down", -0.07, 0.0, 0.0),
    ("にこり", "brow smile", 0.03, -0.03, 0.07),
    ("困る", "troubled", 0.0, 0.12, -0.02),
    ("怒り", "angry", 0.0, -0.1, 0.0),
    ("真面目", "serious", -0.04, -0.02, -0.01),
)
PANEL_BROW = 1
PANEL_EYE = 2
PANEL_MOUTH = 3


class FaceReport:
    def __init__(self):
        self.created = []          # 作ったモーフ名
        self.tongue_tucked = False
        self.warnings = []


def _smoothstep(edge0, edge1, x):
    """edge0 で 1、edge1 で 0 になるなめらかな重み。"""
    if x <= edge0:
        return 1.0
    if x >= edge1:
        return 0.0
    t = (x - edge0) / (edge1 - edge0)
    return 1.0 - t * t * (3.0 - 2.0 * t)


class _Face:
    """顔の頂点・部品(つながった面のかたまり)・目と口の位置をまとめたもの。"""

    def __init__(self, model):
        self.model = model
        bones = model.bones
        self.index_of = {bone.name: i for i, bone in enumerate(bones)}
        self.main_bone = []
        for vertex in model.vertices:
            count = min(len(vertex.bones), len(vertex.weights))
            main = max(range(count), key=lambda k: vertex.weights[k]) if count else 0
            self.main_bone.append(vertex.bones[main] if count else -1)
        self.component = self._components()

    def _components(self):
        """同じ位置の頂点をつないだうえで、面でつながった頂点のかたまり番号を返す。"""
        model = self.model
        weld = {}
        key_of = []
        for vertex in model.vertices:
            key = tuple(round(c, 4) for c in vertex.position)
            key_of.append(weld.setdefault(key, len(weld)))
        parent = list(range(len(weld)))

        def find(a):
            while parent[a] != a:
                parent[a] = parent[parent[a]]
                a = parent[a]
            return a

        for face in model.faces:
            a, b, c = (find(key_of[i]) for i in face)
            parent[a] = b
            parent[find(c)] = find(b)
        return [find(key) for key in key_of]

    def vertices_on(self, bone_names):
        targets = {self.index_of[name] for name in bone_names if name in self.index_of}
        return [i for i, bone in enumerate(self.main_bone) if bone in targets]


# --------------------------------------------------------------------------
# 目
# --------------------------------------------------------------------------

class _Eye:
    def __init__(self, face, bone_name, eye_distance):
        model = face.model
        self.iris = face.vertices_on([bone_name])
        center = model.bones[face.index_of[bone_name]].position
        if self.iris:
            points = [model.vertices[i].position for i in self.iris]
            xs, ys, zs = zip(*points)
            self.cx, self.cy = (min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0
            self.front = min(zs)
            self.rx = max((max(xs) - min(xs)) / 2.0, eye_distance * 0.08)
            self.ry = max((max(ys) - min(ys)) / 2.0, eye_distance * 0.1)
        else:
            self.cx, self.cy, self.front = center
            self.rx, self.ry = eye_distance * 0.18, eye_distance * 0.22
        self.top = self.cy + self.ry * 1.05        # 目の開いているところの上端・下端
        self.bottom = self.cy - self.ry * 0.95
        self.depth = self.ry * 2.0                 # まぶたを回す軸までの奥行き
        self.axis_z = self.front + self.depth
        self.phi_bottom = math.atan2(self.bottom - self.cy, self.depth)
        self.phi_top = math.atan2(self.top - self.cy, self.depth)

    def touches(self, position):
        x, y, z = position
        return abs(x - self.cx) < self.rx and self.bottom < y < self.top and z < self.axis_z

    def closed_offset(self, position, smile):
        """まぶたを閉じたときの移動量。smile なら閉じた線を上へ弧にする。"""
        x, y, z = position
        if z >= self.axis_z:
            return None
        weight = _smoothstep(self.rx * 1.6, self.rx * 2.3, abs(x - self.cx))
        if weight <= 0.0:
            return None
        py, pz = y - self.cy, self.axis_z - z            # 軸から見た上・前
        rho = math.hypot(py, pz)
        phi = math.atan2(py, pz)
        span = self.phi_top - self.phi_bottom
        band = span * 0.6                                 # 上まぶたとして引き伸ばす幅
        low = self.phi_bottom - span * 0.45               # ここより下は動かさない
        rim = self.phi_bottom - span * 0.15               # 下のふち(測った下端より少し下まで白目とみなす)
        if phi <= low or phi >= self.phi_top + band:
            return None
        closed = self.phi_bottom - span * 0.05            # 閉じた線(下まぶたのあたり)
        if smile:
            arch = max(0.0, 1.0 - ((x - self.cx) / (self.rx * 1.2)) ** 2)
            closed = self.phi_bottom + span * (0.15 + 0.35 * arch)
        if phi <= rim:
            # 下まぶた: 閉じた線まで持ち上げる(笑いでは大きく持ち上がる)
            target = low + (phi - low) / (rim - low) * (closed - span * 0.02 - low)
        elif phi <= self.phi_top:
            # 白目・まぶたのふち: 閉じた線へ押し縮める
            target = closed - span * 0.02 + (phi - rim) / (self.phi_top - rim) * span * 0.04
        else:
            # 上まぶた: 閉じた線から元の位置へ引き伸ばす
            start = closed + span * 0.02
            target = start + (phi - self.phi_top) / band * (self.phi_top + band - start)
        target = phi + (target - phi) * weight
        return (0.0, rho * math.sin(target) - py, -(rho * math.cos(target) - pz))

    def iris_offset(self, position, smile):
        """瞳は閉じた線へ縮めて奥へ下げ、まぶたの裏へ隠す。"""
        x, y, z = position
        closed_y = self.bottom + (self.top - self.bottom) * (0.3 if smile else 0.05)
        return (0.0, closed_y + (y - self.cy) * 0.05 - y, self.ry * 0.8)


def _eye_morph(face, eyes, name, name_en, smile):
    model = face.model
    morph = pmxfile.PmxMorph(name, name_en, PANEL_EYE, pmxfile.MORPH_VERTEX)
    head = face.index_of.get("頭")
    iris = set()
    for eye in eyes:
        iris.update(eye.iris)
    # 目の開いているところに掛かっている部品(顔の皮膚・白目・まつげ)だけ動かす。
    # 眉など離れた部品は動かさない
    parts = set()
    for i, vertex in enumerate(model.vertices):
        if face.main_bone[i] == head and any(eye.touches(vertex.position) for eye in eyes):
            parts.add(face.component[i])
    for i, vertex in enumerate(model.vertices):
        position = vertex.position
        eye = min(eyes, key=lambda e: abs(position[0] - e.cx))
        if i in iris:
            offset = eye.iris_offset(position, smile)
        elif face.component[i] in parts and face.main_bone[i] == head \
                and position[2] > eye.front - eye.ry * 0.6:
            # 前髪は顔とつながっていても髪のボーンに乗っているか、目より手前に浮いている
            offset = eye.closed_offset(position, smile)
        else:
            continue
        if offset is not None and any(abs(c) > 1e-6 for c in offset):
            morph.offsets.append((i, offset))
    return morph


# --------------------------------------------------------------------------
# 口
# --------------------------------------------------------------------------

class _Mouth:
    def __init__(self, face, eyes, eye_distance):
        model = face.model
        head = face.index_of.get("頭")
        eye_y = sum(e.cy for e in eyes) / len(eyes)
        eye_bottom = min(e.bottom for e in eyes)
        center_x = sum(e.cx for e in eyes) / len(eyes)
        left, right = max(e.cx for e in eyes), min(e.cx for e in eyes)
        components = {}
        for i, bone in enumerate(face.main_bone):
            if bone == head:
                components.setdefault(face.component[i], []).append(i)
        self.skin = max(components.values(), key=len) if components else []
        skin_component = face.component[self.skin[0]] if self.skin else None
        # 口の中の部品(歯・舌): 顔の皮膚とは別のかたまりで、両目の間の幅・目より下にある小さなもの
        self.parts = []
        for component, members in components.items():
            if component == skin_component or len(members) > 400:
                continue
            points = [model.vertices[i].position for i in members]
            xs, ys, _zs = zip(*points)
            if min(xs) >= right - eye_distance * 0.2 and max(xs) <= left + eye_distance * 0.2 \
                    and max(ys) < eye_bottom - eye_distance * 0.25 \
                    and min(ys) > eye_y - eye_distance * 1.4:
                self.parts.append(members)
        if self.parts:
            points = [model.vertices[i].position for part in self.parts for i in part]
            xs, ys, zs = zip(*points)
            self.x = (min(xs) + max(xs)) / 2.0
            self.y = (min(ys) + max(ys)) / 2.0
            self.width = max(max(xs) - min(xs), eye_distance * 0.35)
        else:
            self.x = center_x
            self.y = eye_y - eye_distance * 0.75
            self.width = eye_distance * 0.45
        # 口の位置の皮膚の表面(正面からいちばん手前)
        near = [model.vertices[i].position[2] for i in self.skin
                if abs(model.vertices[i].position[0] - self.x) < eye_distance * 0.12
                and abs(model.vertices[i].position[1] - self.y) < eye_distance * 0.12]
        self.surface = min(near) if near else min(e.front for e in eyes)
        self.eye_distance = eye_distance
        self.half = self.width * 0.5      # 口の端までの幅
        self.slit = None                  # 唇の合わせ目 [(x, y)](x の順)
        self.upper_rim = set()            # 合わせ目の縁の頂点(上唇側・下唇側)
        self.lower_rim = set()
        self._find_slit(face)
        # 顎の軸: 口より少し上・耳のあたりの奥
        self.pivot_y = self.y + eye_distance * 0.4
        self.pivot_z = self.surface + eye_distance * 0.95

    def _find_slit(self, face):
        """唇の合わせ目を探す。

        口を開けられる顔は、閉じていても上唇と下唇が別の頂点になっていて、頂点番号で
        面のつながりを見ると口のところに縁(1 枚の面にしか使われない辺)がある。
        口の幅を細かく区切り、区切りごとに縁の頂点の高さがいちばん大きく空いている
        ところ(上唇の縁と下唇の縁の間)を合わせ目の高さにする。
        """
        model = face.model
        d = self.eye_distance
        skin = set(self.skin)
        uses = {}
        for a, b, c in model.faces:
            for edge in ((a, b), (b, c), (c, a)):
                key = (edge[0], edge[1]) if edge[0] < edge[1] else (edge[1], edge[0])
                uses[key] = uses.get(key, 0) + 1
        points = set()
        rim = set()
        for (a, b), count in uses.items():
            if count != 1 or a not in skin or b not in skin:   # 歯・舌の縁は数えない
                continue
            for index in (a, b):
                x, y, z = model.vertices[index].position
                if abs(x - self.x) < self.width * 0.8 and abs(y - self.y) < d * 0.15 \
                        and z < self.surface + d * 0.2:
                    points.add((round(x, 5), round(y, 5)))
                    rim.add(index)
        if len(points) < 6:
            return
        # 上唇と下唇の縁が同じ高さに重なっているモデルもあるので、縁の頂点ごとに
        # 付いている面が下側にあるか(下唇)上側にあるか(上唇)も覚えておく
        heights = {index: [] for index in rim}
        for face_indices in model.faces:
            members = [i for i in face_indices if i in heights]
            if members:
                center = sum(model.vertices[i].position[1] for i in face_indices) / 3.0
                for i in members:
                    heights[i].append(center)
        for index, centers in heights.items():
            if centers:
                below = sum(centers) / len(centers) < model.vertices[index].position[1]
                (self.lower_rim if below else self.upper_rim).add(index)
        xs = [p[0] for p in points]
        left, right = min(xs), max(xs)
        if right - left < d * 0.15:
            return
        bins = 12
        step = (right - left) / bins
        slit = []
        for k in range(bins):
            ys = sorted({y for x, y in points if left + step * k <= x <= left + step * (k + 1)})
            if len(ys) == 1:
                slit.append((left + step * (k + 0.5), ys[0]))
            elif ys:
                gap, upper = max((b - a, b) for a, b in zip(ys, ys[1:]))
                slit.append((left + step * (k + 0.5), upper - gap / 2.0))
        if len(slit) < 3:
            return
        self.slit = slit
        self.x = (left + right) / 2.0
        self.half = (right - left) / 2.0
        self.width = right - left
        self.y = sum(y for _x, y in slit) / len(slit)

    def slit_y(self, x):
        """x での唇の合わせ目の高さ(口の端より外は端の高さのまま)。"""
        if not self.slit:
            return self.y
        if x <= self.slit[0][0]:
            return self.slit[0][1]
        for (x0, y0), (x1, y1) in zip(self.slit, self.slit[1:]):
            if x <= x1:
                return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
        return self.slit[-1][1]

    def has_closed_lips(self, face):
        """口の中ほどに上唇と下唇の両方の縁があるか(顎を回して開ける作りか)。"""
        if not self.slit:
            return False
        model = face.model

        def central(rim):
            return sum(1 for i in rim if abs(model.vertices[i].position[0] - self.x) < self.half * 0.5)

        return central(self.upper_rim) >= 3 and central(self.lower_rim) >= 3

    def tongue(self, face):
        """口の前へ飛び出している部品(舌)の頂点。無ければ空。"""
        model = face.model
        for part in self.parts:
            front = min(model.vertices[i].position[2] for i in part)
            if front < self.surface - self.eye_distance * 0.03:
                return part
        return []

    def offset(self, index, position, opening, widen, lower_part, lift=0.0):
        x, y, z = position
        d = self.eye_distance
        if z >= self.pivot_z:
            return None
        dx = abs(x - self.x)
        reach = _smoothstep(self.half * 1.4, d * 0.95, dx)
        if reach <= 0.0:
            return None
        ox = oy = oz = 0.0
        below_slit = self.slit_y(x) - y
        if abs(below_slit) < d * 0.003 and (index in self.lower_rim or index in self.upper_rim):
            below = index in self.lower_rim        # 合わせ目ちょうどの縁は付いている面の側で決める
        else:
            below = below_slit > 0.0
        if (lower_part or below) and opening > 0.0:
            # 合わせ目より下(下唇・口の中の下側・下の歯・あご)を顎の軸で回して下げる。
            # 唇は口の端へ向かって弱め(端は閉じたまま)、口の端より外は合わせ目から
            # 離れるほど強くしてなめらかに伸ばす。首寄りも弱める
            inner = _smoothstep(self.half * 0.45, self.half, dx)
            ramp = 1.0 - _smoothstep(0.0, d * 0.3, max(0.0, below_slit))
            weight = max(inner, ramp) * reach * _smoothstep(d * 0.55, d * 0.9, below_slit)
            if lower_part:
                weight = max(weight, reach)
            angle = math.radians(16.0) * opening * weight
            py, pz = y - self.pivot_y, self.pivot_z - z
            c, s = math.cos(angle), math.sin(angle)
            ny = py * c - pz * s
            nz = py * s + pz * c
            oy, oz = ny - py, -(nz - pz)
        if lift:
            # 口角を上げ下げ: 左右の口角に近いほど強く、口の高さから離れると弱める
            corner = math.hypot(dx - self.half, y - self.slit_y(x))
            weight = _smoothstep(d * 0.04, d * 0.35, corner) \
                * _smoothstep(d * 0.15, d * 0.35, abs(y - self.y))
            oy += lift * d * 0.05 * weight
            oz += abs(lift) * d * 0.01 * weight
        if widen:
            # 口角を左右へ: 口の高さに近いほど強く
            weight = _smoothstep(d * 0.05, d * 0.3, abs(y - self.y)) \
                * _smoothstep(self.width * 0.6, d * 0.8, dx)
            ox = (x - self.x) * widen * 0.6 * weight
            if widen < 0.0:      # すぼめるときは唇を少し前へ
                oz += widen * d * 0.06 * weight
        if abs(ox) < 1e-7 and abs(oy) < 1e-7 and abs(oz) < 1e-7:
            return None
        return (ox, oy, oz)


def _mouth_morph(face, mouth, name, name_en, opening, widen, tongue, lift=0.0):
    model = face.model
    morph = pmxfile.PmxMorph(name, name_en, PANEL_MOUTH, pmxfile.MORPH_VERTEX)
    targets = set(mouth.skin)
    for part in mouth.parts:
        targets.update(part)             # 歯は頂点ごとに合わせ目の上下で分ける
    lower = set(tongue)                  # 舌は顎と一緒に下げる
    for i in sorted(targets):
        offset = mouth.offset(i, model.vertices[i].position, opening, widen, i in lower, lift)
        if offset is not None:
            morph.offsets.append((i, offset))
    return morph


# --------------------------------------------------------------------------
# 眉
# --------------------------------------------------------------------------

def _brow_vertices(face, eyes, eye_distance):
    """眉の頂点: 顔の皮膚とは別の小さな部品で、目の少し上にあるもの。"""
    model = face.model
    head = face.index_of.get("頭")
    members = {}
    for i, bone in enumerate(face.main_bone):
        if bone == head:
            members.setdefault(face.component[i], []).append(i)
    found = []
    for component, indices in members.items():
        if len(indices) > 400:
            continue
        points = [model.vertices[i].position for i in indices]
        cy = sum(p[1] for p in points) / len(points)
        cz = min(p[2] for p in points)
        eye = min(eyes, key=lambda e: abs(sum(p[0] for p in points) / len(points) - e.cx))
        if eye.top < cy < eye.top + eye_distance * 0.7 and cz < eye.front + eye_distance * 0.3 \
                and all(abs(p[0] - e.cx) < eye_distance * 0.9 for p in points
                        for e in [min(eyes, key=lambda e2: abs(p[0] - e2.cx))]):
            found.extend(indices)
    return found


def _brow_morphs(face, eyes, eye_distance):
    model = face.model
    brows = _brow_vertices(face, eyes, eye_distance)
    if not brows:
        return []
    center_x = sum(e.cx for e in eyes) / len(eyes)
    sides = {}
    for i in brows:
        sides.setdefault(model.vertices[i].position[0] > center_x, []).append(i)
    morphs = []
    for name, name_en, whole, inner, arch in _BROWS:
        morph = pmxfile.PmxMorph(name, name_en, PANEL_BROW, pmxfile.MORPH_VERTEX)
        for side in sides.values():
            spans = [abs(model.vertices[i].position[0] - center_x) for i in side]
            near, far = min(spans), max(spans)
            for i in side:
                t = (abs(model.vertices[i].position[0] - center_x) - near) / ((far - near) or 1.0)
                lift = whole + inner * (1.0 - t) + arch * (1.0 - (2.0 * t - 1.0) ** 2)
                if abs(lift) > 1e-6:
                    morph.offsets.append((i, (0.0, lift * eye_distance, 0.0)))
        morphs.append(morph)
    return morphs


def _blend(name, name_en, panel, parts):
    """いくつかの頂点モーフを重みを付けて足し合わせた頂点モーフ。parts: [(モーフ, 重み)]"""
    total = {}
    for morph, weight in parts:
        for i, offset in morph.offsets:
            current = total.get(i, (0.0, 0.0, 0.0))
            total[i] = tuple(current[k] + offset[k] * weight for k in range(3))
    morph = pmxfile.PmxMorph(name, name_en, panel, pmxfile.MORPH_VERTEX)
    morph.offsets = sorted(total.items())
    return morph


# --------------------------------------------------------------------------
# まとめ
# --------------------------------------------------------------------------

def generate(model):
    """表情モーフが 1 つも無い人型モデルに、目と口のモーフを作る。FaceReport を返す。"""
    report = FaceReport()
    if model.morphs or not model.vertices:     # 骨格だけ(モーション変換用)のときも作らない
        return report
    face = _Face(model)
    if not all(name in face.index_of for name in ("頭", "左目", "右目")):
        return report
    left = model.bones[face.index_of["左目"]].position
    right = model.bones[face.index_of["右目"]].position
    eye_distance = math.dist(left, right)
    if eye_distance < 1e-4:
        return report
    eyes = [_Eye(face, "左目", eye_distance), _Eye(face, "右目", eye_distance)]
    morphs = [_eye_morph(face, eyes, "まばたき", "blink", False),
              _eye_morph(face, eyes, "笑い", "smile", True)]

    mouth = _Mouth(face, eyes, eye_distance)
    if mouth.skin:
        tongue = mouth.tongue(face)
        if tongue:
            # 舌を口の中へしまい、ぺろっ で元の位置へ出す
            zs = [model.vertices[i].position[2] for i in tongue]
            depth = (max(zs) - min(zs)) + (mouth.surface - min(zs)) + eye_distance * 0.05
            out = pmxfile.PmxMorph("ぺろっ", "tongue out", PANEL_MOUTH, pmxfile.MORPH_VERTEX)
            for i in tongue:
                x, y, z = model.vertices[i].position
                model.vertices[i].position = (x, y, z + depth)
                out.offsets.append((i, (0.0, 0.0, -depth)))
            report.tongue_tucked = True
            morphs.append(out)
        if mouth.has_closed_lips(face):
            for name, name_en, opening, widen, lift in _VOWELS + _MOUTH_SHAPES:
                morphs.append(_mouth_morph(face, mouth, name, name_en, opening, widen, tongue,
                                           lift))
        else:
            report.warnings.append("閉じた唇の合わせ目が見つからないため、あいうえお のモーフは"
                                   "作りませんでした(口を開けた顔・写実的な顔など)")
    else:
        report.warnings.append("口の位置が分からないため、口のモーフは作りませんでした")

    morphs.extend(_brow_morphs(face, eyes, eye_distance))
    by_name = {morph.name: morph for morph in morphs if morph.offsets}
    if "口角上げ" in by_name:
        # 目を半分閉じると白目だけが見えるので、目は開けたまま口角と眉で笑う
        parts = [(by_name["口角上げ"], 1.0)] +             ([(by_name["にこり"], 1.0)] if "にこり" in by_name else [])
        morphs.append(_blend("柔らかな笑み", "soft smile", PANEL_MOUTH, parts))
    for morph in morphs:
        if morph.offsets:
            model.morphs.append(morph)
            report.created.append(morph.name)
    return report
