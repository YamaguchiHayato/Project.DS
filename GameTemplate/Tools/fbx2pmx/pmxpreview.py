# -*- coding: utf-8 -*-
"""PMX(と VMD の指定フレーム)の姿勢を、正面と横からの画像に描いて確認するツール。

MMD を開かずに、変換したモデルの形・向き・モーションの当たり方を確かめる用途。
IK・回転付与・表情モーフのほか、髪・スカートなどの揺れものを簡易物理演算で動かす
(--no-physics で止められる)。描画に Pillow を使う。

    python pmxpreview.py model.pmx
    python pmxpreview.py model.pmx dance.vmd --frame 120 -o pose.png
    python pmxpreview.py model.pmx --bodies        # 剛体を重ねて表示
"""

from __future__ import annotations

import argparse
import array
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import physics    # noqa: E402
import physsim    # noqa: E402
import pmxfile    # noqa: E402
import posing     # noqa: E402
import texconv    # noqa: E402
import vmdfile    # noqa: E402


SUPERSAMPLE = 2                  # 縦横 2 倍で描いてから縮小し、輪郭のギザギザを消す
BACKGROUND = (245, 245, 248)
GRAY = (0.82, 0.80, 0.77, 1.0)   # --no-texture のときの色
LIGHT = (-0.35, 0.55, -0.75)     # 光の来る向き(画面の右・上・奥で見た向き。左上手前から照らす)
AMBIENT = 0.72                   # 光の当たらない側の明るさ
GRAY_AMBIENT = 0.45              # 灰色で描くときは形が分かるよう陰影を強めにする
TEXTURE_LIMIT = 2048             # テクスチャはこれより大きければ縮小して使う
WARMUP_FRAMES = 60               # 物理演算は指定フレームのこれだけ前から流して揺れを作る
SETTLE_FRAMES = 45               # モーションが無いときは静止姿勢でこれだけ流して落ち着かせる


def _body_endpoints(body):
    if body.shape != pmxfile.SHAPE_CAPSULE:
        return body.position, body.position
    axis = physics.rotation_axis_y(body.rotation)
    half = body.size[1] * 0.5
    return (tuple(body.position[k] - axis[k] * half for k in range(3)),
            tuple(body.position[k] + axis[k] * half for k in range(3)))


def load_textures(model, base_dir):
    """PMX のテクスチャ表の画像を読む。({番号: (RGBA バイト列, 幅, 高さ)}, 読めなかった画像) を返す。"""
    from PIL import Image

    textures = {}
    missing = []
    for index, relative in enumerate(model.textures):
        path = os.path.join(base_dir, relative.replace("\\", os.sep))
        try:
            try:
                image = Image.open(path).convert("RGBA")
            except Exception:
                if os.path.splitext(path)[1].lower() != ".dds":
                    raise
                width, height, rgba = texconv.read_dds(path)
                image = Image.frombytes("RGBA", (width, height), bytes(rgba))
        except Exception:
            missing.append(relative)
            continue
        if max(image.size) > TEXTURE_LIMIT:
            image.thumbnail((TEXTURE_LIMIT, TEXTURE_LIMIT))
        textures[index] = (image.tobytes(), image.width, image.height)
    return textures, missing


def _rasterize(model, points, normals, full, textures):
    """1 面ぶんを Z バッファで画素ごとに塗り、RGB のバイト列を返す。

    points は画面座標 (x, y 下向き, 奥行き)、normals は画面の向き (右, 上, 奥) の法線。
    テクスチャは画素ごとに UV を補間して拾い、明るさは頂点ごとに計算して補間する
    (面の境目が目立たないなめらかな陰影)。材質は MMD と同じく PMX の順に描き、
    半透明の部分は下の色に重ねる。両面描画でない材質は裏側を描かない。
    """
    inf = float("inf")
    depth = array.array("d", [inf]) * (full * full)
    color = bytearray(BACKGROUND) * (full * full)
    length = math.sqrt(sum(c * c for c in LIGHT))
    lx, ly, lz = (c / length for c in LIGHT)
    ambient = AMBIENT if textures is not None else GRAY_AMBIENT
    front = []
    back = []
    for n in normals:
        nl = math.sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]) or 1.0
        d = (n[0] * lx + n[1] * ly + n[2] * lz) / nl
        front.append(ambient + (1.0 - ambient) * max(0.0, d))
        back.append(ambient + (1.0 - ambient) * max(0.0, -d))
    limit = full - 1
    eps = 1e-7

    start = 0
    for material in model.materials:
        count = material.surface_count // 3
        faces = model.faces[start:start + count]
        start += count
        if textures is None:
            dr, dg, db, da = GRAY
            texture = None
        else:
            dr, dg, db, da = (min(1.0, max(0.0, c)) for c in material.diffuse)
            texture = textures.get(material.texture_index)
        if da <= 0.0 or not faces:
            continue
        double_sided = material.flags & pmxfile.MATERIAL_DOUBLE_SIDED
        if texture is not None:
            pixels, tw, th = texture
            alpha_scale = da / 255.0
        else:
            flat = (255.0 * dr, 255.0 * dg, 255.0 * db)

        for a, b, c in faces:
            x0, y0, z0 = points[a]
            x1, y1, z1 = points[b]
            x2, y2, z2 = points[c]
            area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
            if area == 0.0:
                continue
            if area > 0.0:      # 画面上で時計回り = 表(PMX は左手系・時計回りが表)
                shade = front
            elif double_sided:
                shade = back
            else:
                continue
            min_x = max(int(min(x0, x1, x2)), 0)
            max_x = min(int(max(x0, x1, x2)), limit)
            min_y = max(int(min(y0, y1, y2)), 0)
            max_y = min(int(max(y0, y1, y2)), limit)
            if min_x > max_x or min_y > max_y:
                continue
            inv = 1.0 / area
            # 重心座標 w1(b の重み)・w2(c の重み)の画面 x, y についての傾き
            w1x, w1y = (y2 - y0) * inv, (x0 - x2) * inv
            w2x, w2y = (y0 - y1) * inv, (x1 - x0) * inv
            w0x, w0y = -w1x - w2x, -w1y - w2y

            def slope(f0, f1, f2):
                return ((f1 - f0) * w1x + (f2 - f0) * w2x, (f1 - f0) * w1y + (f2 - f0) * w2y)

            zx, zy = slope(z0, z1, z2)
            s0 = shade[a]
            sx, sy = slope(s0, shade[b], shade[c])
            if texture is not None:
                ua, va = model.vertices[a].uv
                ub, vb = model.vertices[b].uv
                uc, vc = model.vertices[c].uv
                ux, uy = slope(ua, ub, uc)
                vx, vy = slope(va, vb, vc)

            for py in range(min_y, max_y + 1):
                cy = py + 0.5 - y0
                # この行で 3 つの重心座標がすべて 0 以上になる x の範囲
                lo, hi = min_x, max_x
                for k, m in ((1.0 + w0y * cy - w0x * x0, w0x),
                             (w1y * cy - w1x * x0, w1x),
                             (w2y * cy - w2x * x0, w2x)):
                    if m > 1e-12:
                        edge = math.ceil((-eps - k) / m - 0.5)
                        if edge > lo:
                            lo = edge
                    elif m < -1e-12:
                        edge = math.floor((-eps - k) / m - 0.5)
                        if edge < hi:
                            hi = edge
                    elif k < -eps:
                        hi = lo - 1
                if lo > hi:
                    continue
                dx = lo + 0.5 - x0
                z = z0 + zx * dx + zy * cy
                s = s0 + sx * dx + sy * cy
                index = py * full + lo
                if texture is None:
                    for _ in range(hi - lo + 1):
                        if z < depth[index]:
                            depth[index] = z
                            p = index * 3
                            color[p] = int(flat[0] * s)
                            color[p + 1] = int(flat[1] * s)
                            color[p + 2] = int(flat[2] * s)
                        z += zx
                        s += sx
                        index += 1
                    continue
                u = ua + ux * dx + uy * cy
                v = va + vx * dx + vy * cy
                for _ in range(hi - lo + 1):
                    if z < depth[index]:
                        o = ((int(v * th) % th) * tw + int(u * tw) % tw) * 4
                        alpha = pixels[o + 3] * alpha_scale
                        p = index * 3
                        if alpha >= 0.9:
                            depth[index] = z
                            color[p] = int(pixels[o] * s * dr)
                            color[p + 1] = int(pixels[o + 1] * s * dg)
                            color[p + 2] = int(pixels[o + 2] * s * db)
                        elif alpha > 0.04:     # 半透明: 下の色に重ねる
                            if alpha >= 0.5:
                                depth[index] = z
                            keep = 1.0 - alpha
                            color[p] = int(color[p] * keep + pixels[o] * s * dr * alpha)
                            color[p + 1] = int(color[p + 1] * keep + pixels[o + 1] * s * dg * alpha)
                            color[p + 2] = int(color[p + 2] * keep + pixels[o + 2] * s * db * alpha)
                    z += zx
                    s += sx
                    u += ux
                    v += vx
                    index += 1
    return color


def render(model, positions, normals, path, size=520, bodies=False, textures=None):
    """正面(MMD のカメラ側)と左側面を並べた画像を書き出す。

    textures(load_textures の結果)を渡すとテクスチャで塗る。省略すると灰色で塗る。
    """
    from PIL import Image, ImageDraw

    xs = [p[0] for p in positions] or [0.0]
    ys = [p[1] for p in positions] or [0.0]
    zs = [p[2] for p in positions] or [0.0]
    span = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs), 1e-6) * 1.08
    center_y = (max(ys) + min(ys)) / 2.0

    def to_view(p, view):
        """(画面の右, 上, 奥) の向きへ直す。正面は MMD のカメラ側(-Z)から、側面は左から見る。"""
        return (p[0], p[1], p[2]) if view == 0 else (p[2], p[1], -p[0])

    def project(p, view):
        sx, sy, depth = to_view(p, view)
        offset = size / 2.0 + size * view
        return (offset + sx / span * size, size / 2.0 - (sy - center_y) / span * size), depth

    image = Image.new("RGB", (size * 2, size), BACKGROUND)
    full = size * SUPERSAMPLE
    for view in (0, 1):
        points = []
        for p in positions:
            sx, sy, depth = to_view(p, view)
            points.append(((size / 2.0 + sx / span * size) * SUPERSAMPLE,
                           (size / 2.0 - (sy - center_y) / span * size) * SUPERSAMPLE, depth))
        view_normals = [to_view(n, view) for n in normals]
        pixels = _rasterize(model, points, view_normals, full, textures)
        picture = Image.frombytes("RGB", (full, full), bytes(pixels)).reduce(SUPERSAMPLE)
        image.paste(picture, (size * view, 0))
    draw = ImageDraw.Draw(image)
    draw.line([(size, 0), (size, size)], fill=(180, 180, 190))

    if bodies and model.rigid_bodies:
        overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
        layer = ImageDraw.Draw(overlay)
        for view in (0, 1):
            for body in model.rigid_bodies:
                a, b = _body_endpoints(body)
                pa, pb = project(a, view)[0], project(b, view)[0]
                r = body.size[0] / span * size
                color = (40, 110, 230, 110) if body.mode == pmxfile.BODY_FOLLOW_BONE \
                    else (240, 120, 20, 130)
                layer.line([pa, pb], fill=color, width=max(1, int(r * 2)))
                for p in (pa, pb):
                    layer.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=color)
            for joint in model.joints:
                p = project(joint.position, view)[0]
                layer.ellipse([p[0] - 2.5, p[1] - 2.5, p[0] + 2.5, p[1] + 2.5],
                              fill=(200, 0, 60, 255))
        image = Image.alpha_composite(image.convert("RGBA"), overlay).convert("RGB")
    image.save(path)


# モーションでよく使う標準のボーン・モーフ(当たらないと動きや表情が欠ける)
_STANDARD_KEYS = {"センター", "グルーブ", "上半身", "上半身2", "下半身", "首", "頭", "両目",
                  "まばたき", "あ", "い", "う", "え", "お", "ウィンク", "ウィンク右", "笑い"}
for _side in ("左", "右"):
    _STANDARD_KEYS.update(_side + name for name in (
        "肩", "腕", "ひじ", "手首", "足", "ひざ", "足首", "足ＩＫ", "つま先ＩＫ", "目"))


def _report_missing(missing):
    """モデルに無くて当たらなかったキーを知らせる。

    配布モーションには元のモデル専用のボーン(髪・スカートなど)や表情が大量に入って
    いるので、標準のものだけ名前を出し、残りは数だけ出す。
    """
    names = sorted(set(missing))
    standard = [n for n in names if n.replace("(モーフ)", "") in _STANDARD_KEYS]
    others = len(names) - len(standard)
    if standard:
        print("モデルに無い標準のボーン・モーフ(この部分は動きません): %s" % "、".join(standard))
    if others:
        print("元のモデル専用のボーン・表情など %d 個はこのモデルに無いので当たりません"
              "(髪・スカート・独自の表情など。普通のことです)" % others)


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="pmxpreview", description="PMX(と VMD)の姿勢を画像に描いて確認します。")
    parser.add_argument("model", metavar="PMX")
    parser.add_argument("motion", nargs="?", metavar="VMD", help="当てるモーション(省略可)")
    parser.add_argument("--frame", type=float, default=0.0, help="VMD のフレーム番号 (既定 0)")
    parser.add_argument("-o", "--output", metavar="PNG", help="出力画像 (既定: <モデル名>_preview.png)")
    parser.add_argument("--size", type=int, default=520, help="1 面あたりの大きさ(ピクセル)")
    parser.add_argument("--bodies", action="store_true", help="剛体とジョイントを重ねて描く")
    parser.add_argument("--no-texture", action="store_true",
                        help="テクスチャを使わず灰色で描く")
    parser.add_argument("--no-physics", action="store_true",
                        help="揺れもの(髪・スカート)の物理演算をしない")
    args = parser.parse_args(argv)
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    try:
        import PIL  # noqa: F401
    except ImportError:
        print("画像の描画に Pillow が必要です: pip install Pillow", file=sys.stderr)
        return 2

    model = pmxfile.read_pmx(args.model)
    if args.motion:
        sampler = posing.MotionSampler(model, vmdfile.read_vmd(args.motion))
        _report_missing(sampler.missing)
        pose, morphs = sampler.pose(args.frame)
        start = max(0.0, args.frame - WARMUP_FRAMES)
        poses = [sampler.pose(start + i)[0] for i in range(int(args.frame - start))] + [pose]
    else:
        pose = posing.Pose(model)
        pose.solve()
        morphs = {}
        poses = [pose] * SETTLE_FRAMES
    if not args.no_physics:
        physsim.simulate(model, poses)
    positions, normals = pose.skinned_vertices(morphs)
    output = args.output or os.path.splitext(args.model)[0] + "_preview.png"
    textures = None
    if not args.no_texture:
        textures, missing = load_textures(model, os.path.dirname(os.path.abspath(args.model)))
        if missing:
            print("読めなかったテクスチャ(その材質は材質の色で塗ります): %s" % "、".join(missing))
    render(model, positions, normals, output, args.size, args.bodies, textures)
    print("描画しました: %s" % output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
