# -*- coding: utf-8 -*-
"""MMD のモーションファイル(VMD)の読み書き。

ボーンとモーフ(表情)のキーフレームを扱う。カメラ・照明・セルフ影は 0 件で書き出す。
名前は Shift-JIS で 15 バイトまで(超える名前は MMD 側でも一致しない)。
"""

from __future__ import annotations

import struct

VMD_SIGNATURE = b"Vocaloid Motion Data 0002"
NAME_BYTES = 15

# 補間曲線(線形)。X・Y・Z・回転の 4 チャンネル分を MMD と同じ並びで 4 行持つ
_LINEAR_ROW = [20] * 8 + [107] * 8
_LINEAR_INTERPOLATION = bytes(
    _LINEAR_ROW
    + _LINEAR_ROW[1:] + [1]
    + _LINEAR_ROW[2:] + [1, 0]
    + _LINEAR_ROW[3:] + [1, 0, 0])


class BoneFrame:
    __slots__ = ("name", "frame", "position", "rotation")

    def __init__(self, name, frame, position=(0.0, 0.0, 0.0), rotation=(0.0, 0.0, 0.0, 1.0)):
        self.name = name
        self.frame = frame
        self.position = position
        self.rotation = rotation    # クォータニオン (x, y, z, w)


class MorphFrame:
    __slots__ = ("name", "frame", "weight")

    def __init__(self, name, frame, weight):
        self.name = name
        self.frame = frame
        self.weight = weight


class VmdMotion:
    def __init__(self, model_name=""):
        self.model_name = model_name
        self.bone_frames = []
        self.morph_frames = []

    @property
    def last_frame(self):
        frames = [f.frame for f in self.bone_frames] + [f.frame for f in self.morph_frames]
        return max(frames) if frames else 0


def encode_name(text, size=NAME_BYTES):
    """Shift-JIS で size バイトに詰める。収まらない名前は切り詰める。"""
    raw = text.encode("cp932", errors="replace")[:size]
    return raw + b"\x00" * (size - len(raw))


def name_fits(text):
    """VMD の名前欄(15 バイト)にそのまま収まるか。"""
    try:
        return len(text.encode("cp932")) <= NAME_BYTES
    except UnicodeEncodeError:
        return False


def _decode_name(raw):
    return raw.split(b"\x00", 1)[0].decode("cp932", errors="replace")


def write_vmd(motion, path):
    data = bytearray()
    data += VMD_SIGNATURE + b"\x00" * (30 - len(VMD_SIGNATURE))
    data += encode_name(motion.model_name, 20)
    data += struct.pack("<I", len(motion.bone_frames))
    for frame in motion.bone_frames:
        data += encode_name(frame.name)
        data += struct.pack("<I3f4f", frame.frame, *frame.position, *frame.rotation)
        data += _LINEAR_INTERPOLATION
    data += struct.pack("<I", len(motion.morph_frames))
    for frame in motion.morph_frames:
        data += encode_name(frame.name)
        data += struct.pack("<If", frame.frame, frame.weight)
    data += struct.pack("<III", 0, 0, 0)   # カメラ・照明・セルフ影
    with open(path, "wb") as stream:
        stream.write(bytes(data))
    return len(data)


def read_vmd(path):
    with open(path, "rb") as stream:
        data = stream.read()
    if not data.startswith(b"Vocaloid Motion Data"):
        raise ValueError("VMD ファイルではありません: %s" % path)
    motion = VmdMotion(_decode_name(data[30:50]))
    pos = 50
    count = struct.unpack_from("<I", data, pos)[0]
    pos += 4
    for _ in range(count):
        name = _decode_name(data[pos:pos + 15])
        values = struct.unpack_from("<I3f4f", data, pos + 15)
        motion.bone_frames.append(BoneFrame(name, values[0], values[1:4], values[4:8]))
        pos += 111
    if pos + 4 <= len(data):
        count = struct.unpack_from("<I", data, pos)[0]
        pos += 4
        for _ in range(count):
            name = _decode_name(data[pos:pos + 15])
            frame, weight = struct.unpack_from("<If", data, pos + 15)
            motion.morph_frames.append(MorphFrame(name, frame, weight))
            pos += 23
    return motion
