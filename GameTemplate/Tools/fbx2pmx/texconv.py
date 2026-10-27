# -*- coding: utf-8 -*-
"""DDS などのテクスチャを MMD が読める PNG / TGA へ変換する。

MMD が読めるのは BMP / PNG / JPEG / TGA (と спа/sph) だけで、DDS は読めない。
ゲーム用アセットの DDS をそのまま PMX から参照しても表示されないため、
このモジュールで変換する。

Pillow が入っていればそれを使い(BC7 などにも対応)、無い場合は内蔵の
デコーダで DXT1/DXT3/DXT5・BC4・BC5・無圧縮 DDS を読む。

    python texconv.py texture.DDS
    python texconv.py ..\\..\\Game\\Assets\\modelData -o out --format png
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
import zlib

# MMD がそのまま読めるファイル形式
MMD_READABLE_EXTENSIONS = frozenset((".bmp", ".png", ".jpg", ".jpeg", ".tga",
                                     ".spa", ".sph"))

_DDS_MAGIC = b"DDS "
_DDPF_ALPHAPIXELS = 0x1
_DDPF_FOURCC = 0x4
_DDPF_RGB = 0x40
_DDPF_LUMINANCE = 0x20000

# DX10 拡張ヘッダの DXGI_FORMAT のうち、内蔵デコーダが扱えるもの
_DXGI_TO_FOURCC = {
    70: b"DXT1", 71: b"DXT1", 72: b"DXT1",      # BC1
    73: b"DXT3", 74: b"DXT3", 75: b"DXT3",      # BC2
    76: b"DXT5", 77: b"DXT5", 78: b"DXT5",      # BC3
    79: b"ATI1", 80: b"ATI1", 81: b"ATI1",      # BC4
    82: b"ATI2", 83: b"ATI2", 84: b"ATI2",      # BC5
}
_DXGI_UNCOMPRESSED = {
    28: ("RGBA", False), 29: ("RGBA", False),   # R8G8B8A8_UNORM(_SRGB)
    87: ("BGRA", False), 91: ("BGRA", False),   # B8G8R8A8_UNORM(_SRGB)
}
_DXGI_NAMES = {94: "BC6H", 95: "BC6H", 96: "BC6H", 97: "BC7", 98: "BC7", 99: "BC7"}


class TextureError(Exception):
    """テクスチャの読み書きに失敗したときに送出する例外。"""


def has_pillow():
    try:
        import PIL  # noqa: F401
        return True
    except ImportError:
        return False


# --------------------------------------------------------------------------
# DDS の読み込み(内蔵デコーダ)
# --------------------------------------------------------------------------

def _color565(value):
    red = (value >> 11) & 0x1F
    green = (value >> 5) & 0x3F
    blue = value & 0x1F
    return ((red << 3) | (red >> 2), (green << 2) | (green >> 4),
            (blue << 3) | (blue >> 2))


def _decode_color_block(data, offset, opaque):
    """BC1 の色ブロック(8 バイト)を 16 画素の RGBA にする。"""
    color0, color1 = struct.unpack_from("<HH", data, offset)
    bits = struct.unpack_from("<I", data, offset + 4)[0]
    red0, green0, blue0 = _color565(color0)
    red1, green1, blue1 = _color565(color1)
    if color0 > color1 or opaque:
        palette = (
            (red0, green0, blue0, 255),
            (red1, green1, blue1, 255),
            ((2 * red0 + red1) // 3, (2 * green0 + green1) // 3,
             (2 * blue0 + blue1) // 3, 255),
            ((red0 + 2 * red1) // 3, (green0 + 2 * green1) // 3,
             (blue0 + 2 * blue1) // 3, 255),
        )
    else:
        palette = (
            (red0, green0, blue0, 255),
            (red1, green1, blue1, 255),
            ((red0 + red1) // 2, (green0 + green1) // 2, (blue0 + blue1) // 2, 255),
            (0, 0, 0, 0),
        )
    return [palette[(bits >> (2 * i)) & 0x3] for i in range(16)]


def _decode_alpha_block(data, offset):
    """BC3/BC4 のアルファブロック(8 バイト)を 16 画素分の値にする。"""
    alpha0 = data[offset]
    alpha1 = data[offset + 1]
    if alpha0 > alpha1:
        palette = [alpha0, alpha1] + [
            ((7 - i) * alpha0 + (i + 1) * alpha1) // 7 for i in range(6)]
    else:
        palette = [alpha0, alpha1] + [
            ((5 - i) * alpha0 + (i + 1) * alpha1) // 5 for i in range(4)] + [0, 255]
    bits = int.from_bytes(data[offset + 2:offset + 8], "little")
    return [palette[(bits >> (3 * i)) & 0x7] for i in range(16)]


def _blit(pixels, width, height, block_x, block_y, block):
    """4x4 ブロックを画像バッファへ書き込む。"""
    for index in range(16):
        x = block_x + (index & 3)
        y = block_y + (index >> 2)
        if x >= width or y >= height:
            continue
        red, green, blue, alpha = block[index]
        position = (y * width + x) * 4
        pixels[position] = red
        pixels[position + 1] = green
        pixels[position + 2] = blue
        pixels[position + 3] = alpha


def _decode_block_compressed(data, offset, width, height, fourcc):
    """DXT1/DXT3/DXT5/BC4/BC5 を RGBA バイト列へ展開する。"""
    pixels = bytearray(width * height * 4)
    block_size = 8 if fourcc in (b"DXT1", b"ATI1", b"BC4U", b"BC4S") else 16
    blocks_x = (width + 3) // 4
    blocks_y = (height + 3) // 4
    needed = blocks_x * blocks_y * block_size
    if len(data) - offset < needed:
        raise TextureError("DDS のデータが足りません(壊れている可能性があります)")

    for block_index in range(blocks_x * blocks_y):
        position = offset + block_index * block_size
        base_x = (block_index % blocks_x) * 4
        base_y = (block_index // blocks_x) * 4
        if fourcc == b"DXT1":
            block = _decode_color_block(data, position, False)
        elif fourcc == b"DXT3":
            colors = _decode_color_block(data, position + 8, True)
            raw = int.from_bytes(data[position:position + 8], "little")
            block = [(colors[i][0], colors[i][1], colors[i][2],
                      ((raw >> (4 * i)) & 0xF) * 17) for i in range(16)]
        elif fourcc in (b"DXT5", b"BC5U"):
            alphas = _decode_alpha_block(data, position)
            colors = _decode_color_block(data, position + 8, True)
            block = [(colors[i][0], colors[i][1], colors[i][2], alphas[i])
                     for i in range(16)]
        elif fourcc in (b"ATI1", b"BC4U", b"BC4S"):
            values = _decode_alpha_block(data, position)
            block = [(v, v, v, 255) for v in values]
        elif fourcc in (b"ATI2", b"BC5S"):
            reds = _decode_alpha_block(data, position)
            greens = _decode_alpha_block(data, position + 8)
            block = [(reds[i], greens[i], 255, 255) for i in range(16)]
        else:
            raise TextureError("未対応の圧縮形式です: %s" % fourcc.decode("latin-1"))
        _blit(pixels, width, height, base_x, base_y, block)
    return bytes(pixels)


def _mask_shift(mask):
    if mask == 0:
        return 0, 0
    shift = 0
    while not (mask >> shift) & 1:
        shift += 1
    bits = 0
    value = mask >> shift
    while (value >> bits) & 1:
        bits += 1
    return shift, bits


def _decode_uncompressed(data, offset, width, height, bit_count, masks):
    """マスク指定の無圧縮 DDS を RGBA バイト列へ展開する。"""
    bytes_per_pixel = bit_count // 8
    if bytes_per_pixel not in (1, 2, 3, 4):
        raise TextureError("未対応のビット深度です: %d" % bit_count)
    channels = []
    for mask in masks:
        shift, bits = _mask_shift(mask)
        channels.append((mask, shift, (1 << bits) - 1 if bits else 0))

    pixels = bytearray(width * height * 4)
    for index in range(width * height):
        position = offset + index * bytes_per_pixel
        raw = int.from_bytes(data[position:position + bytes_per_pixel], "little")
        out = index * 4
        for channel in range(3):
            mask, shift, maximum = channels[channel]
            pixels[out + channel] = ((raw & mask) >> shift) * 255 // maximum \
                if maximum else 0
        mask, shift, maximum = channels[3]
        pixels[out + 3] = (((raw & mask) >> shift) * 255 // maximum) if maximum else 255
    return bytes(pixels)


def read_dds(path):
    """DDS を読んで (width, height, RGBA バイト列) を返す(内蔵デコーダ)。"""
    with open(path, "rb") as stream:
        data = stream.read()
    if len(data) < 128 or data[:4] != _DDS_MAGIC:
        raise TextureError("DDS ファイルではありません: %s" % path)

    height, width = struct.unpack_from("<II", data, 12)
    pf_flags, fourcc_raw, bit_count = struct.unpack_from("<I4sI", data, 80)
    masks = struct.unpack_from("<4I", data, 92)
    offset = 128
    fourcc = fourcc_raw

    if pf_flags & _DDPF_FOURCC and fourcc == b"DX10":
        dxgi_format = struct.unpack_from("<I", data, 128)[0]
        offset = 148
        if dxgi_format in _DXGI_TO_FOURCC:
            fourcc = _DXGI_TO_FOURCC[dxgi_format]
        elif dxgi_format in _DXGI_UNCOMPRESSED:
            order, _ = _DXGI_UNCOMPRESSED[dxgi_format]
            bit_count = 32
            masks = ((0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)
                     if order == "RGBA"
                     else (0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000))
            pf_flags = _DDPF_RGB | _DDPF_ALPHAPIXELS
            fourcc = b""
        else:
            name = _DXGI_NAMES.get(dxgi_format, "DXGI_FORMAT=%d" % dxgi_format)
            raise TextureError(
                "%s は内蔵デコーダでは読めません。pip install Pillow を実行してください。"
                % name)

    if pf_flags & _DDPF_FOURCC and fourcc:
        return width, height, _decode_block_compressed(data, offset, width, height,
                                                       fourcc)
    if pf_flags & (_DDPF_RGB | _DDPF_LUMINANCE):
        if not (pf_flags & _DDPF_ALPHAPIXELS):
            masks = (masks[0], masks[1], masks[2], 0)
        if pf_flags & _DDPF_LUMINANCE and not masks[1]:
            masks = (masks[0], masks[0], masks[0], masks[3])
        return width, height, _decode_uncompressed(data, offset, width, height,
                                                   bit_count, masks)
    raise TextureError("未対応の DDS です: flags=0x%x fourcc=%r" % (pf_flags, fourcc))


# --------------------------------------------------------------------------
# 書き出し
# --------------------------------------------------------------------------

def _png_chunk(tag, payload):
    return (struct.pack(">I", len(payload)) + tag + payload
            + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))


def write_png(path, width, height, rgba):
    """RGBA バイト列を PNG として書き出す。"""
    stride = width * 4
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # フィルタ種別 None
        raw += rgba[y * stride:(y + 1) * stride]
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as stream:
        stream.write(b"\x89PNG\r\n\x1a\n")
        stream.write(_png_chunk(b"IHDR", header))
        stream.write(_png_chunk(b"IDAT", zlib.compress(bytes(raw), 6)))
        stream.write(_png_chunk(b"IEND", b""))


def write_tga(path, width, height, rgba):
    """RGBA バイト列を 32bit 無圧縮 TGA として書き出す。"""
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0,
                         width, height, 32, 0x28)  # 左上原点
    body = bytearray(len(rgba))
    body[0::4] = rgba[2::4]  # B
    body[1::4] = rgba[1::4]  # G
    body[2::4] = rgba[0::4]  # R
    body[3::4] = rgba[3::4]  # A
    with open(path, "wb") as stream:
        stream.write(header)
        stream.write(bytes(body))


def _resize_nearest(width, height, rgba, max_size):
    """最近傍で縮小する(Pillow が無いときの簡易版)。"""
    if max(width, height) <= max_size:
        return width, height, rgba
    scale = max(width, height) / float(max_size)
    new_width = max(1, int(width / scale))
    new_height = max(1, int(height / scale))
    out = bytearray(new_width * new_height * 4)
    for y in range(new_height):
        source_row = min(height - 1, int(y * scale)) * width
        for x in range(new_width):
            source = (source_row + min(width - 1, int(x * scale))) * 4
            target = (y * new_width + x) * 4
            out[target:target + 4] = rgba[source:source + 4]
    return new_width, new_height, bytes(out)


# --------------------------------------------------------------------------
# 変換
# --------------------------------------------------------------------------

def needs_conversion(path):
    """MMD が読めない形式かどうか。"""
    return os.path.splitext(path)[1].lower() not in MMD_READABLE_EXTENSIONS


def _convert_with_pillow(source, destination, image_format, max_size):
    """Pillow で変換する。失敗したら理由の文字列を返す(成功なら None)。"""
    try:
        from PIL import Image
        with Image.open(source) as opened:
            image = opened.convert("RGBA")
        if max_size and max(image.size) > max_size:
            scale = max(image.size) / float(max_size)
            image = image.resize((max(1, int(image.width / scale)),
                                  max(1, int(image.height / scale))),
                                 Image.LANCZOS)
        image.save(destination, "PNG" if image_format == "png" else "TGA")
        return None
    except Exception as error:  # 未対応形式などは内蔵デコーダに回す
        return "%s: %s" % (type(error).__name__, error)


def convert_image(source, destination, image_format="png", max_size=0,
                  use_pillow=True):
    """画像 1 枚を PNG / TGA へ変換する。出力したパスを返す。

    Pillow があればまず Pillow で読む(BC7 などに対応)。Pillow が無い、または
    Pillow が対応していない形式(B8G8R8A8_UNORM_SRGB など)の場合は内蔵の
    DDS デコーダで読み直す。
    """
    image_format = image_format.lower()
    if image_format not in ("png", "tga"):
        raise TextureError("対応していない出力形式です: %s" % image_format)

    reasons = []
    if use_pillow and has_pillow():
        reason = _convert_with_pillow(source, destination, image_format, max_size)
        if reason is None:
            return destination
        reasons.append("Pillow(%s)" % reason)

    try:
        width, height, rgba = read_dds(source)
    except Exception as error:
        reasons.append("内蔵デコーダ(%s)" % error)
        raise TextureError("変換できませんでした: %s / %s"
                           % (os.path.basename(source), " / ".join(reasons)))

    if max_size:
        width, height, rgba = _resize_nearest(width, height, rgba, max_size)
    if image_format == "png":
        write_png(destination, width, height, rgba)
    else:
        write_tga(destination, width, height, rgba)
    return destination


def convert_for_mmd(source, output_dir, image_format="png", max_size=0,
                    overwrite=False):
    """MMD が読めない画像を output_dir へ変換して出力先パスを返す。

    すでに MMD が読める形式ならそのまま None を返す(呼び出し側でコピーする)。
    """
    if not needs_conversion(source):
        return None
    name = os.path.splitext(os.path.basename(source))[0] + "." + image_format
    destination = os.path.join(output_dir, name)
    if not overwrite and os.path.isfile(destination) \
            and os.path.getmtime(destination) >= os.path.getmtime(source):
        return destination
    return convert_image(source, destination, image_format, max_size)


# --------------------------------------------------------------------------
# コマンドライン
# --------------------------------------------------------------------------

def _collect(paths):
    files = []
    for path in paths:
        if os.path.isdir(path):
            for root, _dirs, names in os.walk(path):
                for name in names:
                    if name.lower().endswith(".dds"):
                        files.append(os.path.join(root, name))
        else:
            files.append(path)
    return files


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="texconv",
        description="DDS などのテクスチャを MMD が読める PNG / TGA へ変換します。")
    parser.add_argument("inputs", nargs="+", metavar="FILE",
                        help="変換する画像、またはフォルダ(中の .DDS をまとめて変換)")
    parser.add_argument("-o", "--output", metavar="DIR",
                        help="出力フォルダ(既定は元ファイルと同じ場所)")
    parser.add_argument("--format", choices=("png", "tga"), default="png",
                        help="出力形式 (既定: png)")
    parser.add_argument("--max-size", type=int, default=0,
                        help="長辺がこのピクセル数を超える場合に縮小する")
    parser.add_argument("--no-pillow", action="store_true",
                        help="Pillow を使わず内蔵デコーダだけで変換する")
    parser.add_argument("-q", "--quiet", action="store_true", help="経過表示を抑える")
    args = parser.parse_args(argv)

    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass

    log = (lambda message: None) if args.quiet else print
    files = _collect(args.inputs)
    if not files:
        print("変換対象の画像が見つかりません。", file=sys.stderr)
        return 2

    failed = 0
    for source in files:
        directory = args.output or os.path.dirname(os.path.abspath(source))
        os.makedirs(directory, exist_ok=True)
        name = os.path.splitext(os.path.basename(source))[0] + "." + args.format
        destination = os.path.join(directory, name)
        try:
            convert_image(source, destination, args.format, args.max_size,
                          use_pillow=not args.no_pillow)
        except Exception as error:
            failed += 1
            print("失敗: %s (%s)" % (source, error), file=sys.stderr)
            continue
        log("%s -> %s (%.1f KB)"
            % (source, destination, os.path.getsize(destination) / 1024.0))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
