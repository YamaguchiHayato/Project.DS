# -*- coding: utf-8 -*-
"""4x4 行列と 3 次元ベクトルの最小ユーティリティ。

行列は行優先 (row-major) の長さ 16 のタプルで保持し、列ベクトル規約
(p' = M * p) で扱う。外部ライブラリには依存しない。
"""

from __future__ import annotations

import math

IDENTITY = (
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
)


def multiply(a, b):
    """行列 a と b の積 (a * b) を返す。"""
    r = [0.0] * 16
    for i in range(4):
        ai = i * 4
        a0, a1, a2, a3 = a[ai], a[ai + 1], a[ai + 2], a[ai + 3]
        for j in range(4):
            r[ai + j] = (
                a0 * b[j]
                + a1 * b[4 + j]
                + a2 * b[8 + j]
                + a3 * b[12 + j]
            )
    return tuple(r)


def multiply_all(*mats):
    """複数の行列を左から順に掛け合わせる。"""
    result = IDENTITY
    for m in mats:
        result = multiply(result, m)
    return result


def translation(x, y, z):
    return (
        1.0, 0.0, 0.0, float(x),
        0.0, 1.0, 0.0, float(y),
        0.0, 0.0, 1.0, float(z),
        0.0, 0.0, 0.0, 1.0,
    )


def scaling(x, y, z):
    return (
        float(x), 0.0, 0.0, 0.0,
        0.0, float(y), 0.0, 0.0,
        0.0, 0.0, float(z), 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


def rotation_x(rad):
    c, s = math.cos(rad), math.sin(rad)
    return (
        1.0, 0.0, 0.0, 0.0,
        0.0, c, -s, 0.0,
        0.0, s, c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


def rotation_y(rad):
    c, s = math.cos(rad), math.sin(rad)
    return (
        c, 0.0, s, 0.0,
        0.0, 1.0, 0.0, 0.0,
        -s, 0.0, c, 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


def rotation_z(rad):
    c, s = math.cos(rad), math.sin(rad)
    return (
        c, -s, 0.0, 0.0,
        s, c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    )


# FBX の RotationOrder 列挙値 -> 回転を適用する軸の順番。
_ROTATION_ORDERS = (
    "XYZ",  # 0 : eEulerXYZ
    "XZY",  # 1 : eEulerXZY
    "YZX",  # 2 : eEulerYZX
    "YXZ",  # 3 : eEulerYXZ
    "ZXY",  # 4 : eEulerZXY
    "ZYX",  # 5 : eEulerZYX
    "XYZ",  # 6 : eSphericXYZ (近似として XYZ 扱い)
)

_AXIS_FUNCS = {"X": rotation_x, "Y": rotation_y, "Z": rotation_z}


def euler_degrees(x_deg, y_deg, z_deg, order=0):
    """オイラー角(度)から回転行列を作る。

    order は FBX の RotationOrder 列挙値。"XYZ" は X -> Y -> Z の順に
    適用する意味なので、列ベクトル規約では Rz * Ry * Rx となる。
    """
    axes = _ROTATION_ORDERS[order if 0 <= order < len(_ROTATION_ORDERS) else 0]
    angles = {
        "X": math.radians(x_deg),
        "Y": math.radians(y_deg),
        "Z": math.radians(z_deg),
    }
    m = IDENTITY
    for axis in axes:  # 先に適用する軸ほど右側に来るよう左から掛ける
        m = multiply(_AXIS_FUNCS[axis](angles[axis]), m)
    return m


def transform_point(m, p):
    x, y, z = p
    return (
        m[0] * x + m[1] * y + m[2] * z + m[3],
        m[4] * x + m[5] * y + m[6] * z + m[7],
        m[8] * x + m[9] * y + m[10] * z + m[11],
    )


def transform_vector(m, v):
    """平行移動を無視して方向ベクトルを変換する。"""
    x, y, z = v
    return (
        m[0] * x + m[1] * y + m[2] * z,
        m[4] * x + m[5] * y + m[6] * z,
        m[8] * x + m[9] * y + m[10] * z,
    )


def get_translation(m):
    return (m[3], m[7], m[11])


def inverse(m):
    """4x4 行列の逆行列。特異な場合は単位行列を返す。"""
    a = list(m)
    inv = [0.0] * 16

    inv[0] = (a[5] * a[10] * a[15] - a[5] * a[11] * a[14] - a[9] * a[6] * a[15]
              + a[9] * a[7] * a[14] + a[13] * a[6] * a[11] - a[13] * a[7] * a[10])
    inv[4] = (-a[4] * a[10] * a[15] + a[4] * a[11] * a[14] + a[8] * a[6] * a[15]
              - a[8] * a[7] * a[14] - a[12] * a[6] * a[11] + a[12] * a[7] * a[10])
    inv[8] = (a[4] * a[9] * a[15] - a[4] * a[11] * a[13] - a[8] * a[5] * a[15]
              + a[8] * a[7] * a[13] + a[12] * a[5] * a[11] - a[12] * a[7] * a[9])
    inv[12] = (-a[4] * a[9] * a[14] + a[4] * a[10] * a[13] + a[8] * a[5] * a[14]
               - a[8] * a[6] * a[13] - a[12] * a[5] * a[10] + a[12] * a[6] * a[9])
    inv[1] = (-a[1] * a[10] * a[15] + a[1] * a[11] * a[14] + a[9] * a[2] * a[15]
              - a[9] * a[3] * a[14] - a[13] * a[2] * a[11] + a[13] * a[3] * a[10])
    inv[5] = (a[0] * a[10] * a[15] - a[0] * a[11] * a[14] - a[8] * a[2] * a[15]
              + a[8] * a[3] * a[14] + a[12] * a[2] * a[11] - a[12] * a[3] * a[10])
    inv[9] = (-a[0] * a[9] * a[15] + a[0] * a[11] * a[13] + a[8] * a[1] * a[15]
              - a[8] * a[3] * a[13] - a[12] * a[1] * a[11] + a[12] * a[3] * a[9])
    inv[13] = (a[0] * a[9] * a[14] - a[0] * a[10] * a[13] - a[8] * a[1] * a[14]
               + a[8] * a[2] * a[13] + a[12] * a[1] * a[10] - a[12] * a[2] * a[9])
    inv[2] = (a[1] * a[6] * a[15] - a[1] * a[7] * a[14] - a[5] * a[2] * a[15]
              + a[5] * a[3] * a[14] + a[13] * a[2] * a[7] - a[13] * a[3] * a[6])
    inv[6] = (-a[0] * a[6] * a[15] + a[0] * a[7] * a[14] + a[4] * a[2] * a[15]
              - a[4] * a[3] * a[14] - a[12] * a[2] * a[7] + a[12] * a[3] * a[6])
    inv[10] = (a[0] * a[5] * a[15] - a[0] * a[7] * a[13] - a[4] * a[1] * a[15]
               + a[4] * a[3] * a[13] + a[12] * a[1] * a[7] - a[12] * a[3] * a[5])
    inv[14] = (-a[0] * a[5] * a[14] + a[0] * a[6] * a[13] + a[4] * a[1] * a[14]
               - a[4] * a[2] * a[13] - a[12] * a[1] * a[6] + a[12] * a[2] * a[5])
    inv[3] = (-a[1] * a[6] * a[11] + a[1] * a[7] * a[10] + a[5] * a[2] * a[11]
              - a[5] * a[3] * a[10] - a[9] * a[2] * a[7] + a[9] * a[3] * a[6])
    inv[7] = (a[0] * a[6] * a[11] - a[0] * a[7] * a[10] - a[4] * a[2] * a[11]
              + a[4] * a[3] * a[10] + a[8] * a[2] * a[7] - a[8] * a[3] * a[6])
    inv[11] = (-a[0] * a[5] * a[11] + a[0] * a[7] * a[9] + a[4] * a[1] * a[11]
               - a[4] * a[3] * a[9] - a[8] * a[1] * a[7] + a[8] * a[3] * a[5])
    inv[15] = (a[0] * a[5] * a[10] - a[0] * a[6] * a[9] - a[4] * a[1] * a[10]
               + a[4] * a[2] * a[9] + a[8] * a[1] * a[6] - a[8] * a[2] * a[5])

    det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12]
    if abs(det) < 1e-20:
        return IDENTITY
    inv_det = 1.0 / det
    return tuple(v * inv_det for v in inv)


def normalize(v):
    x, y, z = v
    length = math.sqrt(x * x + y * y + z * z)
    if length < 1e-12:
        return (0.0, 0.0, 0.0)
    return (x / length, y / length, z / length)


def transpose(m):
    """転置行列を返す。FBX は行ベクトル規約なので読み込み時に使う。"""
    return (
        m[0], m[4], m[8], m[12],
        m[1], m[5], m[9], m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15],
    )
