# -*- coding: utf-8 -*-
"""FBX のボーン名を MMD の標準ボーン名へ寄せるための変換表。

Mixamo (mixamorig:Hips...)、Autodesk の Character1_*、3ds Max Biped
(Bip001 *)、Unity Humanoid / VRM 風の名前に対応する。表に無い名前は
変換せず元の名前のまま残す。
"""

from __future__ import annotations

import json
import re

# 名前の前に付きがちな装飾
_STRIP_PREFIXES = ("character1_", "character2_", "mixamorig", "bip001", "bip01", "biped")

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
    "upperchest": "上半身3",
    "neck": "首",
    "neck1": "首",
    "head": "頭",
    "headtop": "頭先",
    "headtopend": "頭先",
}

# 左右を持つボーン ({s} に 左 / 右 が入る)
_SIDED_BONES = {
    "eye": "{s}目",
    "shoulder": "{s}肩",
    "clavicle": "{s}肩",
    "arm": "{s}腕",
    "upperarm": "{s}腕",
    "forearm": "{s}ひじ",
    "lowerarm": "{s}ひじ",
    "elbow": "{s}ひじ",
    "hand": "{s}手首",
    "wrist": "{s}手首",
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

# 指 (MMD は親指だけ ０ 始まり)
_FINGER_BONES = {
    "thumb": ("{s}親指", ("０", "１", "２", "２")),
    "index": ("{s}人指", ("１", "２", "３", "３")),
    "middle": ("{s}中指", ("１", "２", "３", "３")),
    "ring": ("{s}薬指", ("１", "２", "３", "３")),
    "pinky": ("{s}小指", ("１", "２", "３", "３")),
    "little": ("{s}小指", ("１", "２", "３", "３")),
}


def _canonical(name):
    """比較用に名前を小文字の英数字だけへ均す。"""
    text = name.lower()
    for separator in ("|", ":"):
        if separator in text:
            text = text.rsplit(separator, 1)[-1]
    for prefix in _STRIP_PREFIXES:
        if text.startswith(prefix):
            text = text[len(prefix):]
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


def _match_finger(core, side):
    """指ボーンなら MMD 名を返す。"""
    if core.startswith("hand") and len(core) > 4:
        core = core[4:]
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


def to_mmd_name(name):
    """FBX のボーン名に対応する MMD 標準ボーン名を返す。無ければ None。"""
    core = _canonical(name)
    if not core:
        return None

    direct = _CENTER_BONES.get(core)
    if direct is not None:
        return direct

    for side, remainder in _side_candidates(core):
        if not side:
            continue
        sided = _SIDED_BONES.get(remainder)
        if sided is not None:
            return sided.format(s=side)
        finger = _match_finger(remainder, side)
        if finger is not None:
            return finger
        center = _CENTER_BONES.get(remainder)
        if center is not None:
            return center
    return None


# 同じ MMD 名を取り合ったときに優先するボーン(数字が小さいほど優先)
_ASSIGN_PRIORITY = {
    "hips": 0, "hip": 0, "pelvis": 0,
    "center": 5, "waist": 5, "root": 5, "reference": 6,
}


def build_name_table(names, extra_map=None):
    """ボーン名のリストから {元の名前: MMD 名} を作る。

    同じ MMD 名が複数のボーンに割り当たる場合は、最初の 1 本だけ採用して
    残りは元の名前のままにする(MMD 側で名前が衝突しないようにするため)。
    """
    table = {}
    used = set()
    order = sorted(range(len(names)),
                   key=lambda i: (_ASSIGN_PRIORITY.get(_canonical(names[i]), 2), i))
    for name in [names[i] for i in order]:
        mapped = None
        if extra_map:
            mapped = extra_map.get(name)
        if mapped is None:
            mapped = to_mmd_name(name)
        if not mapped or mapped in used:
            continue
        used.add(mapped)
        table[name] = mapped
    return table


def load_bone_map(path):
    """{"FBXのボーン名": "MMDのボーン名"} 形式の JSON を読み込む。"""
    with open(path, "r", encoding="utf-8") as stream:
        data = json.load(stream)
    if not isinstance(data, dict):
        raise ValueError("ボーン名マップは辞書形式の JSON にしてください: %s" % path)
    return {str(key): str(value) for key, value in data.items()}
