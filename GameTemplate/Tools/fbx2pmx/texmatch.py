# -*- coding: utf-8 -*-
"""マテリアルとテクスチャ画像の対応付け。

FBX に書かれた参照どおりの場所に画像が無いことはよくある(作者の PC の
絶対パスのまま、拡張子だけ違う、そもそも参照が無い など)。そこで次の
手がかりで画像を探す。

1. 同じファイル名(大文字小文字の違いを無視)
2. 同じ名前で拡張子だけ違うもの(MMD が読める形式を優先)
3. FBX に埋め込まれた画像
4. マテリアル名などと画像ファイル名の近さ(法線・スペキュラなどは除外)
"""

from __future__ import annotations

import base64
import json
import os
import re

import fbxscene

# 候補として扱う画像の拡張子
IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".psd",
                    ".tif", ".tiff", ".gif", ".webp", ".spa", ".sph")
# MMD がそのまま読める拡張子(同名候補が複数あるときに優先する)
_READABLE = (".png", ".tga", ".bmp", ".jpg", ".jpeg", ".spa", ".sph")

# 名前の比較で無視する語
_NOISE = {"mat", "material", "materials", "mtl", "tex", "texture", "textures", "map",
          "maps", "img", "image", "file", "default", "fbx", "png", "jpg", "tga", "dds",
          "t", "m", "mi", "tx"}
# 拡散色テクスチャらしさを示す語
_DIFFUSE = {"diffuse", "diff", "dif", "albedo", "alb", "basecolor", "base", "color",
            "colour", "col", "maintex", "main", "bc"}
_DIFFUSE_SUFFIX = {"d", "c"}
# 拡散色以外のテクスチャを示す語(これを含む画像は推定の候補から外す)
_NON_DIFFUSE = {"normal", "normals", "nomal", "nrm", "nor", "norm", "nm", "bump",
                "spec", "specular", "spc", "rough", "roughness", "rgh", "metal",
                "metallic", "metalness", "met", "ao", "occlusion", "occ", "emission",
                "emissive", "emit", "emi", "glow", "mask", "msk", "height", "disp",
                "displacement", "opacity", "gloss", "glossiness", "smooth", "smoothness",
                "orm", "mra", "mrao", "rma", "arm", "sss", "matcap", "sph", "spa",
                "toon", "shadow", "rim", "cavity", "curvature", "thickness", "id"}
_NON_DIFFUSE_SUFFIX = {"n", "s"}  # body_n(法線) / body_s(スペキュラ)。_r や _l は左右なので含めない

# 推定で採用する最低スコア
MATCH_THRESHOLD = 0.34

_TOKEN_SPLIT = re.compile(r"[^0-9a-z぀-ヿ㐀-鿿]+")


def tokens(name):
    """名前を比較用の語に分ける(キャメルケース・区切り文字・拡張子を考慮)。"""
    base = os.path.basename(name or "")
    stem, extension = os.path.splitext(base)
    if extension.lower() not in IMAGE_EXTENSIONS:
        stem = base
    stem = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", stem)
    stem = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", stem)
    return [part for part in _TOKEN_SPLIT.split(stem.lower()) if part]


def is_non_diffuse(name):
    """法線・スペキュラなど拡散色以外のテクスチャらしい名前か。"""
    words = tokens(name)
    if not words:
        return False
    if any(word in _NON_DIFFUSE for word in words):
        return True
    return len(words) > 1 and words[-1] in _NON_DIFFUSE_SUFFIX


def _has_diffuse_marker(words):
    if any(word in _DIFFUSE for word in words):
        return True
    return len(words) > 1 and words[-1] in _DIFFUSE_SUFFIX


def _core(words):
    """比較の本体になる語(装飾語・拡散色を示す語を除いたもの)。"""
    result = [w for w in words if w not in _NOISE and w not in _DIFFUSE]
    if len(result) > 1 and result[-1] in _DIFFUSE_SUFFIX:
        result = result[:-1]
    return result


def similarity(key, candidate):
    """キー(マテリアル名など)と画像ファイル名の近さを 0〜1.5 程度で返す。"""
    key_core = set(_core(tokens(key)))
    candidate_words = tokens(candidate)
    candidate_core = set(_core(candidate_words))
    if not key_core or not candidate_core:
        return 0.0
    common = key_core & candidate_core
    score = len(common) / float(len(key_core | candidate_core))
    if not any(not word.isdigit() for word in common):
        score = 0.0  # 「001」のような数字だけの一致は手がかりにしない
    joined_key = "".join(sorted(w for w in key_core if not w.isdigit()))
    joined_candidate = "".join(sorted(w for w in candidate_core if not w.isdigit()))
    if score == 0.0 and min(len(joined_key), len(joined_candidate)) >= 3 \
            and (joined_key in joined_candidate or joined_candidate in joined_key):
        score = 0.35
    if score > 0.0 and _has_diffuse_marker(candidate_words):
        score += 0.1
    return score


class TexturePool:
    """テクスチャの候補になる画像ファイルの集まり。"""

    def __init__(self):
        self.files = []
        self.explicit = set()      # 利用者が直接渡したもの
        self._seen = set()

    def add_file(self, path, explicit=False):
        if not os.path.isfile(path):
            return
        if os.path.splitext(path)[1].lower() not in IMAGE_EXTENSIONS:
            return
        full = os.path.normcase(os.path.abspath(path))
        if full not in self._seen:
            self._seen.add(full)
            self.files.append(os.path.abspath(path))
        if explicit:
            self.explicit.add(full)

    def add_dir(self, directory, depth=3, explicit=False):
        if not os.path.isdir(directory):
            return
        base_depth = os.path.abspath(directory).rstrip("\\/").count(os.sep)
        for root, dirs, names in os.walk(directory):
            if os.path.abspath(root).count(os.sep) - base_depth >= depth:
                dirs[:] = []
            dirs.sort()
            for name in sorted(names):
                self.add_file(os.path.join(root, name), explicit)

    def is_explicit(self, path):
        return os.path.normcase(os.path.abspath(path)) in self.explicit

    def _prefer(self, paths):
        """同じ名前の候補が複数あるとき、MMD が読める形式・利用者指定を優先する。"""
        def key(path):
            extension = os.path.splitext(path)[1].lower()
            readable = _READABLE.index(extension) if extension in _READABLE else len(_READABLE)
            return (0 if self.is_explicit(path) else 1, readable, len(path), path)
        return sorted(paths, key=key)[0] if paths else None

    def find_name(self, name):
        """同じファイル名(大文字小文字は無視)の画像を返す。"""
        target = os.path.basename(name or "").lower()
        if not target:
            return None
        return self._prefer([p for p in self.files if os.path.basename(p).lower() == target])

    def find_stem(self, name):
        """拡張子違いの同名画像を返す。"""
        target = os.path.splitext(os.path.basename(name or ""))[0].lower()
        if not target:
            return None
        return self._prefer([p for p in self.files
                             if os.path.splitext(os.path.basename(p))[0].lower() == target])

    def guess(self, keys):
        """キーの並びに最も近い拡散色テクスチャを (パス, スコア) で返す。"""
        best = (None, 0.0)
        for path in self.files:
            name = os.path.basename(path)
            if is_non_diffuse(name):
                continue
            score = max((similarity(key, name) for key in keys if key), default=0.0)
            if score <= 0.0:
                continue
            if best[0] is None or score > best[1] + 1e-9 or (
                    abs(score - best[1]) <= 1e-9 and self._prefer([path, best[0]]) == path):
                best = (path, score)
        if best[1] < MATCH_THRESHOLD:
            return None, best[1]
        return best

    def single_explicit_diffuse(self):
        """利用者が渡した拡散色らしい画像がちょうど 1 枚ならそれを返す。"""
        candidates = [p for p in self.files
                      if self.is_explicit(p) and not is_non_diffuse(os.path.basename(p))]
        stems = {os.path.splitext(os.path.basename(p))[0].lower() for p in candidates}
        if len(stems) == 1:
            return self._prefer(candidates)
        return None


def build_pool(fbx_path, extra_files=(), extra_dirs=()):
    """FBX の周辺と、利用者が渡したファイル・フォルダから候補を集める。"""
    pool = TexturePool()
    for path in extra_files:
        pool.add_file(path, explicit=True)
    for directory in extra_dirs:
        pool.add_dir(directory, depth=3, explicit=True)

    base = os.path.dirname(os.path.abspath(fbx_path))
    stem = os.path.splitext(os.path.basename(fbx_path))[0]
    pool.add_dir(base, depth=1)
    for name in ("textures", "Textures", "texture", "tex", "Tex", stem + ".fbm",
                 stem + "_textures"):
        pool.add_dir(os.path.join(base, name), depth=2)
    return pool


# --------------------------------------------------------------------------
# 埋め込みテクスチャ
# --------------------------------------------------------------------------

_MAGIC = (
    (b"\x89PNG\r\n\x1a\n", ".png"),
    (b"\xff\xd8\xff", ".jpg"),
    (b"DDS ", ".dds"),
    (b"BM", ".bmp"),
    (b"8BPS", ".psd"),
    (b"GIF8", ".gif"),
    (b"II*\x00", ".tif"),
    (b"MM\x00*", ".tif"),
)


def _sniff_extension(data):
    for magic, extension in _MAGIC:
        if data.startswith(magic):
            return extension
    return ".tga"  # TGA には決まった先頭バイトが無い


def embedded_image(scene, texture):
    """Texture に繋がった Video の埋め込み画像を (ファイル名, バイト列) で返す。"""
    for video in scene.child_objects(texture.id, "Video"):
        content = video.node.child_value("Content")
        data = None
        if isinstance(content, (bytes, bytearray)) and len(content) > 16:
            data = bytes(content)
        elif isinstance(content, str) and len(content) > 64:
            try:
                data = base64.b64decode(re.sub(r"[^A-Za-z0-9+/=]", "", content))
            except (ValueError, TypeError):
                data = None
        if not data:
            continue
        name = None
        for key in ("RelativeFilename", "Filename", "FileName"):
            value = video.node.child_value(key)
            if isinstance(value, str) and value.strip():
                name = os.path.basename(value.replace("\\", "/"))
                break
        if not name:
            name = fbxscene.texture_path(texture) or video.name or texture.name or "embedded"
            name = os.path.basename(name.replace("\\", "/"))
        stem, extension = os.path.splitext(name)
        sniffed = _sniff_extension(data)
        if extension.lower() not in IMAGE_EXTENSIONS:
            name = (stem or "embedded") + sniffed
        name = re.sub(r'[<>:"/\\|?*]', "_", name)
        return name, data
    return None


def write_embedded(directory, name, data):
    """埋め込み画像を書き出す。同名で中身が違うファイルがあれば別名にする。"""
    os.makedirs(directory, exist_ok=True)
    stem, extension = os.path.splitext(name)
    candidate = os.path.join(directory, name)
    number = 1
    while os.path.isfile(candidate):
        with open(candidate, "rb") as stream:
            if stream.read() == data:
                return candidate
        candidate = os.path.join(directory, "%s_%d%s" % (stem, number, extension))
        number += 1
    with open(candidate, "wb") as stream:
        stream.write(data)
    return candidate


# --------------------------------------------------------------------------
# 割り当て表(JSON)
# --------------------------------------------------------------------------

def load_texture_map(path):
    """{"材質名": "画像パス" または null} の JSON を読む。相対パスは JSON の場所基準。"""
    with open(path, "r", encoding="utf-8") as stream:
        data = json.load(stream)
    if not isinstance(data, dict):
        raise ValueError("テクスチャ割り当て表は辞書形式の JSON にしてください: %s" % path)
    base = os.path.dirname(os.path.abspath(path))
    result = {}
    for material, value in data.items():
        if value is None or value == "":
            result[str(material)] = None
            continue
        value = str(value)
        resolved = value if os.path.isabs(value) else os.path.normpath(os.path.join(base, value))
        result[str(material)] = resolved if os.path.isfile(resolved) else value
    return result


def save_texture_map(path, assignments):
    """割り当て結果を、手で直して --texture-map に渡せる JSON として書き出す。"""
    base = os.path.dirname(os.path.abspath(path))
    data = {}
    for material, source in assignments:
        if source is None:
            data[material] = None
            continue
        try:
            relative = os.path.relpath(source, base)
        except ValueError:
            relative = source
        data[material] = relative.replace("\\", "/")
    with open(path, "w", encoding="utf-8") as stream:
        json.dump(data, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
