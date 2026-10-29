# -*- coding: utf-8 -*-
"""材質とテクスチャの取り違えを、UV の重なりから見つけて直す。

1 枚のテクスチャ(アトラス)の中では、ふつう材質ごとの UV は重ならないように並べてある。
ある材質の UV が、同じテクスチャを使う他の材質の UV と大きく重なっているのに、別の
テクスチャではどの材質とも重ならず、しかも絵が描かれている場所に収まるなら、元データの
材質の指定が間違っている(tkm の書き出しで画像の名前がずれた など)とみなして付け替える。

- 同じ UV の複製(左右に同じ部品を置いた材質など)は重なりとして数えない
- 0〜1 の外へはみ出す UV(繰り返し模様)の材質は判定しない
- 描かれているかは、テクスチャの画素が透明でなく、塗りつぶしの余白でもないことで見る
  (Pillow が無いときは、候補が 1 つに絞れる場合だけ付け替える)
"""

from __future__ import annotations

import copy
import os

RESOLUTION = 128          # UV を塗る格子の細かさ
WRONG_OVERLAP = 0.3       # これ以上ほかの材質と重なっていたら取り違えを疑う
FREE_OVERLAP = 0.05       # 付け替え先ではこれ以下しか重ならないこと
TWIN_IOU = 0.9            # これ以上同じ場所を塗る材質は複製とみなす
MIN_CELLS = 8             # 小さすぎる UV の島は判定しない
DRAWN_RATIO = 0.6         # 付け替え先で絵が描かれている割合の下限


def _islands(model):
    """材質ごとに、頂点を共有してつながった面のかたまり(UV の島)に分ける。

    [(材質番号, [面の番号], 覆うマスの集合 または はみ出すなら None)] を返す。
    """
    result = []
    start = 0
    for k, material in enumerate(model.materials):
        count = material.surface_count // 3
        numbers = list(range(start, start + count))
        start += count
        parent = {}

        def find(a):
            while parent.setdefault(a, a) != a:
                parent[a] = parent[parent[a]]
                a = parent[a]
            return a

        for n in numbers:
            a, b, c = model.faces[n]
            parent[find(a)] = find(b)
            parent[find(c)] = find(b)
        groups = {}
        for n in numbers:
            groups.setdefault(find(model.faces[n][0]), []).append(n)
        for faces in groups.values():
            uvs = [model.vertices[i].uv for n in faces for i in model.faces[n]]
            if any(not (-0.01 <= u <= 1.01 and -0.01 <= v <= 1.01) for u, v in uvs):
                result.append((k, faces, None))
                continue
            covered = set()
            for n in faces:
                points = [(model.vertices[i].uv[0] * RESOLUTION, model.vertices[i].uv[1] * RESOLUTION)
                          for i in model.faces[n]]
                covered.update(_raster(points))
            result.append((k, faces, covered))
    return result


def _raster(points):
    """三角形が中心を含むマスの番号(三角形が小さくて 1 つも無ければ重心のマス)。"""
    (x0, y0), (x1, y1), (x2, y2) = points
    area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
    found = []
    if abs(area) > 1e-12:
        for gy in range(max(int(min(y0, y1, y2)), 0), min(int(max(y0, y1, y2)) + 1, RESOLUTION)):
            for gx in range(max(int(min(x0, x1, x2)), 0), min(int(max(x0, x1, x2)) + 1, RESOLUTION)):
                px, py = gx + 0.5, gy + 0.5
                w1 = ((px - x0) * (y2 - y0) - (x2 - x0) * (py - y0)) / area
                w2 = ((x1 - x0) * (py - y0) - (px - x0) * (y1 - y0)) / area
                if w1 >= 0.0 and w2 >= 0.0 and w1 + w2 <= 1.0:
                    found.append(gy * RESOLUTION + gx)
    if not found:
        cx = min(int((x0 + x1 + x2) / 3.0), RESOLUTION - 1)
        cy = min(int((y0 + y1 + y2) / 3.0), RESOLUTION - 1)
        found.append(max(cy, 0) * RESOLUTION + max(cx, 0))
    return found


def _drawn_cells(path):
    """テクスチャのうち、絵が描かれているマスの集合。読めなければ None。"""
    try:
        from PIL import Image
    except ImportError:
        return None
    try:
        image = Image.open(path).convert("RGBA").resize((RESOLUTION * 2, RESOLUTION * 2))
    except Exception:
        return None
    pixels = image.load()
    drawn = set()
    size = RESOLUTION * 2
    for gy in range(RESOLUTION):
        for gx in range(RESOLUTION):
            block = [pixels[gx * 2 + dx, gy * 2 + dy] for dx in (0, 1) for dy in (0, 1)]
            if min(p[3] for p in block) < 128:
                continue
            # 周りと同じ色の平らな余白は「描かれていない」とみなす
            around = [pixels[min(max(gx * 2 + dx, 0), size - 1), min(max(gy * 2 + dy, 0), size - 1)]
                      for dx in (-2, 0, 2, 3) for dy in (-2, 0, 2, 3)]
            spread = max(max(p[k] for p in around) - min(p[k] for p in around) for k in range(3))
            if spread > 6:
                drawn.add(gy * RESOLUTION + gx)
    return drawn


def find_swaps(model, base_dir):
    """付け替える UV の島の [(材質番号, [面の番号], 新しいテクスチャ番号, 元の重なり)]。

    元データで別々だった材質が 1 つにまとめられている(tkm の書き出しなど)こともあるので、
    材質ごとではなく UV の島ごとに判定する。
    """
    islands = _islands(model)
    drawn_cache = {}

    def drawn(texture):
        if texture not in drawn_cache:
            path = os.path.join(base_dir, model.textures[texture].replace("\\", os.sep))
            drawn_cache[texture] = _drawn_cells(path)
        return drawn_cache[texture]

    by_texture = {}
    tiled = set()                                 # 繰り返し模様の画像(はみ出す UV だけが使う)
    for index, (k, _faces, cells) in enumerate(islands):
        texture = model.materials[k].texture_index
        if cells:
            by_texture.setdefault(texture, []).append(index)
        else:
            tiled.add(texture)
    tiled -= set(by_texture)
    swaps = []
    for index, (k, faces, mine) in enumerate(islands):
        current = model.materials[k].texture_index
        if mine is None or len(mine) < MIN_CELLS or current < 0:
            continue
        overlap = {}
        for texture, members in by_texture.items():
            others = set()
            for j in members:
                other = islands[j][2]
                if j == index or len(mine & other) / len(mine | other) > TWIN_IOU:
                    continue                      # 自分自身と、同じ UV の複製は数えない
                others |= other
            overlap[texture] = len(mine & others) / len(mine)
        for texture in range(len(model.textures)):
            overlap.setdefault(texture, 0.0)
        if overlap.get(current, 0.0) < WRONG_OVERLAP:
            continue
        candidates = [t for t in overlap
                      if t != current and t not in tiled and overlap[t] <= FREE_OVERLAP]
        scored = []
        for texture in candidates:
            area = drawn(texture)
            if area is not None:
                scored.append((len(mine & area) / len(mine), texture))
        if scored:
            ratio, best = max(scored)
            if ratio < DRAWN_RATIO:
                continue
        elif len(candidates) == 1:
            best = candidates[0]
        else:
            continue
        swaps.append((k, faces, best, overlap[current]))
    return swaps


def fix_swapped_textures(model, base_dir, log=None):
    """取り違えた UV の島のテクスチャを付け替える。

    材質の面がすべて同じテクスチャへ移るなら材質のテクスチャを差し替え、一部だけなら
    その面を新しい材質(元の材質の写し)へ分けて、元の材質のすぐ後ろに置く。
    付け替えた [(材質名, 元のパス, 新しいパス, 面の数)] を返す。
    """
    log = log or (lambda message: None)
    swaps = find_swaps(model, base_dir)
    if not swaps:
        return []
    moves = {}                                    # 材質番号 -> {新しいテクスチャ: [面]}
    for k, faces, texture, overlap in swaps:
        moves.setdefault(k, {}).setdefault(texture, []).extend(faces)
    fixed = []
    new_faces = []
    new_materials = []
    start = 0
    for k, material in enumerate(model.materials):
        count = material.surface_count // 3
        numbers = list(range(start, start + count))
        start += count
        targets = moves.get(k, {})
        moved = {n for faces in targets.values() for n in faces}
        old_path = model.textures[material.texture_index]
        if len(targets) == 1 and len(moved) == count:
            texture = next(iter(targets))
            material.texture_index = texture
            new_materials.append(material)
            new_faces.extend(model.faces[n] for n in numbers)
            fixed.append((material.name, old_path, model.textures[texture], count))
            continue
        keep = [n for n in numbers if n not in moved]
        material.surface_count = len(keep) * 3
        new_materials.append(material)
        new_faces.extend(model.faces[n] for n in keep)
        for texture, faces in sorted(targets.items()):
            split = copy.copy(material)
            split.name = "%s_%s" % (material.name, os.path.splitext(
                os.path.basename(model.textures[texture]))[0])
            split.texture_index = texture
            split.surface_count = len(faces) * 3
            new_materials.append(split)
            new_faces.extend(model.faces[n] for n in sorted(faces))
            fixed.append((split.name, old_path, model.textures[texture], len(faces)))
    model.materials = new_materials
    model.faces = new_faces
    for name, old_path, new_path, count in fixed:
        log("  テクスチャの取り違えを直しました: 材質「%s」(%d 面) %s → %s"
            "(UV がほかの部品と重なっていたため)" % (name, count, old_path, new_path))
    return fixed
