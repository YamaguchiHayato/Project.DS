# -*- coding: utf-8 -*-
"""ボーン名を MMD の標準ボーン名へ寄せるための変換表。

Mixamo (mixamorig:Hips...)、Autodesk の Character1_*、3ds Max Biped
(Bip001 *) と CAT (Base HumanLArmUpperarm...)、Unity Humanoid / VRM 風、
SapphiArt 風 (Leg_L / Knee_L ...) の名前に対応する。表に無い名前は変換せず
元の名前のまま残す。

名前だけでは決まらないもの(Leg が太ももかすねか、Spine1 が何番目の背骨か、
全ボーン共通の接頭辞など)は、build_name_table で骨格全体を見て判断する。
"""

from __future__ import annotations

import json
import os
import re

# 名前の前に付きがちな装飾
_STRIP_PREFIXES = ("character1_", "character2_", "mixamorig", "bip001", "bip01", "biped",
                   "bone")

# 左右の接頭辞 / 接尾辞
_LEFT_WORDS = ("left", "l")
_RIGHT_WORDS = ("right", "r")

# 側を持たないボーン
_CENTER_BONES = {
    "hips": "センター",
    "hip": "センター",
    "pelvis": "センター",
    "root": "全ての親",
    "reference": "全ての親",
    "center": "センター",
    "waist": "腰",
    "spine": "上半身",
    "spine1": "上半身2",
    "spine2": "上半身3",
    "spine3": "上半身3",
    "chest": "上半身2",
    "ribcage": "上半身3",
    "upperchest": "上半身3",
    "neck": "首",
    "neck1": "首",
    "head": "頭",
    "headtop": "頭先",
    "headtopend": "頭先",
}

# 背骨の並び(骨格に含まれるものを、この順に 上半身 / 上半身2 / 上半身3 へ割り当てる)
_SPINE_RANK = {"spine": 0, "spine1": 1, "spine2": 2, "spine3": 3, "chest": 4,
               "ribcage": 4, "upperchest": 5}
_SPINE_NAMES = ("上半身", "上半身2", "上半身3")

# 左右を持つボーン ({s} に 左 / 右 が入る)
_SIDED_BONES = {
    "eye": "{s}目",
    "shoulder": "{s}肩",
    "clavicle": "{s}肩",
    "collarbone": "{s}肩",
    "arm": "{s}腕",
    "upperarm": "{s}腕",
    "forearm": "{s}ひじ",
    "lowerarm": "{s}ひじ",
    "elbow": "{s}ひじ",
    "hand": "{s}手首",
    "wrist": "{s}手首",
    "palm": "{s}手首",
    "upleg": "{s}足",
    "upperleg": "{s}足",
    "thigh": "{s}足",
    "leg": "{s}ひざ",
    "lowerleg": "{s}ひざ",
    "calf": "{s}ひざ",
    "knee": "{s}ひざ",
    "foot": "{s}足首",
    "ankle": "{s}足首",
    "toe": "{s}つま先",
    "toe0": "{s}つま先",
    "toes": "{s}つま先",
    "toebase": "{s}つま先",
    "toeend": "{s}つま先先",
}
# 「腕」「脚」を表す中間語(CAT の LArmUpperarm / LLegThigh など)
_ARM_PARTS = ("collarbone", "clavicle", "upperarm", "forearm", "palm", "hand")
_LEG_PARTS = ("thigh", "calf", "foot", "ankle", "toe")

# 指 (MMD は親指だけ ０ 始まり)
_FINGER_BONES = {
    "thumb": ("{s}親指", ("０", "１", "２", "２")),
    "index": ("{s}人指", ("１", "２", "３", "３")),
    "middle": ("{s}中指", ("１", "２", "３", "３")),
    "ring": ("{s}薬指", ("１", "２", "３", "３")),
    "pinky": ("{s}小指", ("１", "２", "３", "３")),
    "little": ("{s}小指", ("１", "２", "３", "３")),
}
# CAT の DigitXY: X = 指(1 親指 〜 5 小指), Y = 関節
_DIGIT_FINGERS = ("thumb", "index", "middle", "ring", "pinky")


def _canonical(name, prefix=""):
    """比較用に名前を小文字の英数字だけへ均す(共通の接頭辞があれば外す)。"""
    if prefix and name.startswith(prefix):
        name = name[len(prefix):]
    text = name.lower()
    for separator in ("|", ":"):
        if separator in text:
            text = text.rsplit(separator, 1)[-1]
    for strip in _STRIP_PREFIXES:
        if text.startswith(strip):
            text = text[len(strip):]
            break
    return re.sub(r"[^a-z0-9]", "", text)


def _side_candidates(core):
    """左右表記の切り出し方の候補を (side, 残り) の形で優先順に並べる。

    "leg" の先頭 1 文字を左と誤認しないよう、1 文字表記は最後に試す。
    """
    candidates = []
    for words, side in ((_LEFT_WORDS, "左"), (_RIGHT_WORDS, "右")):
        for word in words:
            if len(word) == 1:
                continue
            if core.startswith(word) and len(core) > len(word):
                candidates.append((side, core[len(word):]))
            if core.endswith(word) and len(core) > len(word):
                candidates.append((side, core[:-len(word)]))
    for words, side in ((_LEFT_WORDS, "左"), (_RIGHT_WORDS, "右")):
        for word in words:
            if len(word) != 1:
                continue
            if core.startswith(word) and len(core) > 1:
                candidates.append((side, core[1:]))
            if core.endswith(word) and len(core) > 1:
                candidates.append((side, core[:-1]))
    candidates.append(("", core))
    return candidates


def _match_finger(core, side, limb=""):
    """指ボーンなら MMD 名を返す。limb は CAT の中間語("arm" / "leg" / "")。"""
    if core.startswith("hand") and len(core) > 4:
        core = core[4:]
    digit = re.match(r"^digit(\d)(\d)$", core)
    if digit:
        finger, joint = int(digit.group(1)), int(digit.group(2))
        if limb == "arm" and 1 <= finger <= 5:
            template, digits = _FINGER_BONES[_DIGIT_FINGERS[finger - 1]]
            return template.format(s=side) + digits[max(1, min(joint, len(digits))) - 1]
        if limb != "arm" and finger == 1 and joint == 1:
            return side + "つま先"     # 脚の Digit11 はつま先
        return None
    match = re.match(r"^([a-z]+?)(\d*)$", core)
    if not match:
        return None
    base, number = match.group(1), match.group(2)
    entry = _FINGER_BONES.get(base)
    if entry is None:
        return None
    template, digits = entry
    index = int(number) if number else 1
    index = max(1, min(index, len(digits)))
    return template.format(s=side) + digits[index - 1]


def _sided_name(remainder, side, leg_is_thigh):
    """左右を取り除いた残りの名前から MMD 名を求める。"""
    limb = ""
    for infix, parts in (("arm", _ARM_PARTS), ("leg", _LEG_PARTS)):
        rest = remainder[len(infix):]
        if remainder.startswith(infix) and (rest in parts or rest.startswith("digit")):
            limb, remainder = infix, rest
            break
    if remainder == "leg" and leg_is_thigh:
        return side + "足"
    sided = _SIDED_BONES.get(remainder)
    if sided is not None:
        return sided.format(s=side)
    finger = _match_finger(remainder, side, limb)
    if finger is not None:
        return finger
    return None


def to_mmd_name(name, prefix="", leg_is_thigh=False):
    """ボーン名に対応する MMD 標準ボーン名を返す。無ければ None。

    prefix: 骨格の全ボーンに共通する接頭辞(build_name_table が求める)
    leg_is_thigh: 骨格に knee があり、leg が太ももを指す場合 True
    """
    core = _canonical(name, prefix)
    if not core:
        return None

    direct = _CENTER_BONES.get(core)
    if direct is not None:
        return direct

    for side, remainder in _side_candidates(core):
        if not side:
            continue
        found = _sided_name(remainder, side, leg_is_thigh)
        if found is not None:
            return found
        center = _CENTER_BONES.get(remainder)
        if center is not None:
            return center
    # 頭の下にぶら下がる目(CAT の HeadREye など)
    if core.startswith("head") and len(core) > 4:
        for side, remainder in _side_candidates(core[4:]):
            if side and remainder == "eye":
                return side + "目"
    return None


def common_prefix(names):
    """全ボーンに共通する接頭辞(Zombie_ / Base Human / SapphiArt_19: など)を返す。

    名前の途中で切れていない(区切り文字で終わる、または残りが大文字で始まる)
    ときだけ採用する。
    """
    names = [name for name in names if name]
    if len(names) < 2:
        return ""
    prefix = os.path.commonprefix(names)
    while prefix:
        bare = re.sub(r"[_\s:.|]+$", "", prefix).lower()
        boundary = prefix[-1] in "_ :.|" or all(
            len(name) > len(prefix) and (name[len(prefix)].isupper() or name[len(prefix)].isdigit())
            for name in names)
        if len(bare) >= 3 and bare not in ("left", "right") and boundary:
            if all(len(name) > len(prefix) for name in names):
                return prefix
        prefix = prefix[:-1]
    return ""


# VMD(モーション)の名前欄は Shift-JIS で 15 バイトまで。これを超える名前の
# ボーン・モーフにはモーションのキーを当てられない
NAME_LIMIT = 15


def name_bytes(text):
    try:
        return len(text.encode("cp932"))
    except UnicodeEncodeError:
        return len(text.encode("utf-8"))


def _truncate_bytes(text, limit):
    while text and name_bytes(text) > limit:
        text = text[:-1]
    return text


def shorten(name, used, prefix=""):
    """名前を VMD に入る 15 バイト以内へ縮める(共通接頭辞・区切り文字を外し、末尾の番号と左右は残す)。

    used に含まれる名前とは重ならないようにする。同じ入力なら必ず同じ結果になる。
    """
    text = name[len(prefix):] if prefix and name.startswith(prefix) else name
    if ":" in text:
        text = text.rsplit(":", 1)[1]
    candidates = [text, re.sub(r"[_\s\-.|]", "", text)]
    for candidate in candidates:
        if candidate and name_bytes(candidate) <= NAME_LIMIT and candidate not in used:
            return candidate
    compact = candidates[1] or text
    tail_match = re.search(r"(\d+[LRlr]?|[LR])$", compact)
    tail = tail_match.group(1) if tail_match else ""
    head = compact[:len(compact) - len(tail)] if tail else compact
    result = _truncate_bytes(head, NAME_LIMIT - name_bytes(tail)) + tail
    number = 1
    while result in used or not result:
        mark = str(number)
        result = _truncate_bytes(head, NAME_LIMIT - name_bytes(tail) - len(mark)) + mark + tail
        number += 1
    return result


# 同じ MMD 名を取り合ったときに優先するボーン(数字が小さいほど優先)
_ASSIGN_PRIORITY = {
    "hips": 0, "hip": 0, "pelvis": 0,
    "center": 5, "waist": 5, "root": 5, "reference": 6,
}


def build_name_table(names, extra_map=None):
    """ボーン名のリストから {元の名前: MMD 名} を作る。

    骨格全体を見て次を判断する。
    - 全ボーン共通の接頭辞は外して比べる
    - knee があれば leg は太もも(足)、無ければすね(ひざ)
    - 背骨は Spine / Spine1 / Chest / Ribcage などの並び順で 上半身 / 上半身2 / 上半身3
    同じ MMD 名が複数のボーンに割り当たる場合は最初の 1 本だけ採用する。
    """
    prefix = common_prefix(names)
    cores = {name: _canonical(name, prefix) for name in names}
    side_free = set()
    for core in cores.values():
        for side, remainder in _side_candidates(core):
            side_free.add(remainder)
    leg_is_thigh = "knee" in side_free

    spine = sorted((name for name in names if cores[name] in _SPINE_RANK),
                   key=lambda name: (_SPINE_RANK[cores[name]], names.index(name)))
    spine_table = {name: _SPINE_NAMES[i] for i, name in enumerate(spine[:len(_SPINE_NAMES)])}

    table = {}
    used = set()
    order = sorted(range(len(names)),
                   key=lambda i: (_ASSIGN_PRIORITY.get(cores[names[i]], 2), i))
    for name in [names[i] for i in order]:
        mapped = None
        if extra_map:
            mapped = extra_map.get(name)
        if mapped is None and cores[name] in _SPINE_RANK:
            mapped = spine_table.get(name)
        elif mapped is None:
            mapped = to_mmd_name(name, prefix, leg_is_thigh)
        if not mapped or mapped in used:
            continue
        used.add(mapped)
        table[name] = mapped

    # 標準名にならなかったボーンも、モーションのキーが当たるよう 15 バイト以内にする
    used.update(name for name in names if name not in table and name_bytes(name) <= NAME_LIMIT
                and not (prefix and name.startswith(prefix)))
    for name in names:
        if name in table:
            continue
        if name_bytes(name) <= NAME_LIMIT and not (prefix and name.startswith(prefix)):
            continue
        short = shorten(name, used, prefix)
        used.add(short)
        table[name] = short
    return table


def load_bone_map(path):
    """{"FBXのボーン名": "MMDのボーン名"} 形式の JSON を読み込む。"""
    with open(path, "r", encoding="utf-8") as stream:
        data = json.load(stream)
    if not isinstance(data, dict):
        raise ValueError("ボーン名マップは辞書形式の JSON にしてください: %s" % path)
    return {str(key): str(value) for key, value in data.items()}
