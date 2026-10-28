# -*- coding: utf-8 -*-
"""FBX ファイルを汎用のノードツリーとして読み込むモジュール。

バイナリ FBX (7.x) と ASCII FBX の両方に対応する。外部ライブラリや
Autodesk FBX SDK には依存しない。
"""

from __future__ import annotations

import array
import struct
import sys
import zlib

BINARY_MAGIC = b"Kaydara FBX Binary  \x00"


class FbxError(Exception):
    """FBX の解析に失敗したときに送出する例外。"""


class FbxNode:
    """FBX のノードレコード 1 件。"""

    __slots__ = ("name", "props", "children")

    def __init__(self, name, props=None, children=None):
        self.name = name
        self.props = props if props is not None else []
        self.children = children if children is not None else []

    def find(self, name):
        """名前が一致する最初の子ノードを返す。無ければ None。"""
        for child in self.children:
            if child.name == name:
                return child
        return None

    def find_all(self, name):
        """名前が一致する子ノードをすべて返す。"""
        return [child for child in self.children if child.name == name]

    def prop(self, index, default=None):
        """index 番目のプロパティを返す。範囲外なら default。"""
        if 0 <= index < len(self.props):
            return self.props[index]
        return default

    def child_value(self, name, index=0, default=None):
        """子ノード name の index 番目のプロパティを返す。"""
        child = self.find(name)
        if child is None:
            return default
        return child.prop(index, default)

    def child_array(self, name):
        """子ノード name が持つ配列プロパティを返す。無ければ None。"""
        child = self.find(name)
        if child is None:
            return None
        value = child.prop(0)
        if value is None:
            return None
        if isinstance(value, (array.array, list, tuple)):
            return value
        return [value]

    def __repr__(self):
        return "FbxNode(%r, props=%d, children=%d)" % (
            self.name, len(self.props), len(self.children))


def decode_string(raw):
    """FBX の文字列バイト列を str にする。"""
    if isinstance(raw, str):
        return raw
    for encoding in ("utf-8", "cp932"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return raw.decode("latin-1")


# --------------------------------------------------------------------------
# バイナリ FBX
# --------------------------------------------------------------------------

_ARRAY_TYPES = {
    b"f": ("f", 4),
    b"d": ("d", 8),
    b"l": ("q", 8),
    b"i": ("i", 4),
    b"b": ("b", 1),
}

_SCALAR_TYPES = {
    b"Y": ("<h", 2),
    b"C": ("<?", 1),
    b"I": ("<i", 4),
    b"F": ("<f", 4),
    b"D": ("<d", 8),
    b"L": ("<q", 8),
}


class _BinaryReader:
    """バイナリ FBX のレコードを読み進めるヘルパ。"""

    def __init__(self, data, version):
        self.data = data
        self.version = version
        self.wide = version >= 7500  # FBX 7.5 以降はレコードヘッダが 64bit

    def read_property(self, pos):
        data = self.data
        type_code = data[pos:pos + 1]
        pos += 1

        scalar = _SCALAR_TYPES.get(type_code)
        if scalar is not None:
            fmt, size = scalar
            return struct.unpack_from(fmt, data, pos)[0], pos + size

        if type_code in (b"S", b"R"):
            length = struct.unpack_from("<I", data, pos)[0]
            pos += 4
            raw = data[pos:pos + length]
            pos += length
            return (raw if type_code == b"R" else decode_string(raw)), pos

        array_type = _ARRAY_TYPES.get(type_code)
        if array_type is not None:
            type_char, item_size = array_type
            length, encoding, compressed_len = struct.unpack_from("<III", data, pos)
            pos += 12
            raw = data[pos:pos + compressed_len]
            pos += compressed_len
            if encoding == 1:
                raw = zlib.decompress(raw)
            values = array.array(type_char)
            values.frombytes(raw[:length * item_size])
            if sys.byteorder != "little":
                values.byteswap()
            return values, pos

        raise FbxError("未知のプロパティ型: %r (offset=%d)" % (type_code, pos - 1))

    def read_node(self, pos):
        """ノードを 1 件読む。終端レコードなら (None, 次の位置) を返す。"""
        data = self.data
        if self.wide:
            end_offset, num_props, _prop_len = struct.unpack_from("<QQQ", data, pos)
            pos += 24
        else:
            end_offset, num_props, _prop_len = struct.unpack_from("<III", data, pos)
            pos += 12
        name_len = data[pos]
        pos += 1
        if end_offset == 0:
            return None, pos + name_len

        name = decode_string(data[pos:pos + name_len])
        pos += name_len

        props = []
        for _ in range(num_props):
            value, pos = self.read_property(pos)
            props.append(value)

        children = []
        while pos < end_offset:
            child, pos = self.read_node(pos)
            if child is None:
                break
            children.append(child)

        return FbxNode(name, props, children), end_offset


def _read_binary(data):
    version = struct.unpack_from("<I", data, 23)[0]
    reader = _BinaryReader(data, version)
    root = FbxNode("", [], [])
    pos = 27
    limit = len(data) - 16  # 末尾のフッタは読まない
    while pos < limit:
        node, pos = reader.read_node(pos)
        if node is None:
            break
        root.children.append(node)
    return root, version


# --------------------------------------------------------------------------
# ASCII FBX
# --------------------------------------------------------------------------

def _tokenize_ascii(text):
    """ASCII FBX をトークン列にする。"""
    tokens = []
    index = 0
    length = len(text)
    while index < length:
        ch = text[index]
        if ch in " \t\r\n":
            index += 1
            continue
        if ch == ";":  # 行コメント
            while index < length and text[index] != "\n":
                index += 1
            continue
        if ch in "{},":
            tokens.append(ch)
            index += 1
            continue
        if ch == '"':
            index += 1
            start = index
            while index < length and text[index] != '"':
                index += 1
            tokens.append(("str", text[start:index]))
            index += 1
            continue
        start = index
        while index < length and text[index] not in ' \t\r\n{},;"':
            index += 1
        word = text[start:index]
        if word.endswith(":"):
            tokens.append(("key", word[:-1]))
        elif word.startswith("*"):
            tokens.append(("count", word[1:]))
        else:
            tokens.append(("word", word))
    return tokens


def _ascii_value(word):
    """ASCII FBX の裸トークンを数値 or 文字列に変換する。"""
    try:
        return int(word)
    except ValueError:
        pass
    try:
        return float(word)
    except ValueError:
        return word


def _fold_array_node(props, children):
    """"*N { a: ... }" 形式を配列プロパティ 1 個に畳み込む。"""
    has_count = any(isinstance(p, tuple) and p[0] == "__count__" for p in props)
    props = [p for p in props if not (isinstance(p, tuple) and p[0] == "__count__")]
    if has_count and len(children) == 1 and children[0].name == "a":
        values = children[0].props
        if values and all(isinstance(v, int) for v in values):
            props.append(array.array("q", values))
        else:
            props.append(array.array("d", [float(v) for v in values]))
        children = []
    return props, children


def _parse_ascii_nodes(tokens, pos, stop_at_brace):
    """ノード列を解析して (ノードのリスト, 次の位置) を返す。"""
    nodes = []
    while pos < len(tokens):
        token = tokens[pos]
        if token == "}":
            if stop_at_brace:
                return nodes, pos + 1
            pos += 1
            continue
        if token in (",", "{"):
            pos += 1
            continue
        if not isinstance(token, tuple) or token[0] != "key":
            pos += 1
            continue

        name = token[1]
        pos += 1
        props = []
        while pos < len(tokens):
            token = tokens[pos]
            if token == ",":
                pos += 1
                continue
            if token in ("{", "}"):
                break
            kind, value = token
            if kind == "key":
                break
            if kind == "str":
                props.append(value)
            elif kind == "count":
                props.append(("__count__", int(value)))
            else:
                props.append(_ascii_value(value))
            pos += 1

        children = []
        if pos < len(tokens) and tokens[pos] == "{":
            children, pos = _parse_ascii_nodes(tokens, pos + 1, True)

        props, children = _fold_array_node(props, children)
        nodes.append(FbxNode(name, props, children))

    return nodes, pos


def _read_ascii(data):
    text = data.decode("utf-8", errors="replace")
    nodes, _ = _parse_ascii_nodes(_tokenize_ascii(text), 0, False)
    root = FbxNode("", [], nodes)
    version = 7400
    header = root.find("FBXHeaderExtension")
    if header is not None:
        value = header.child_value("FBXVersion")
        if isinstance(value, int):
            version = value
    return root, version


# --------------------------------------------------------------------------
# 公開 API
# --------------------------------------------------------------------------

def read_fbx(path):
    """FBX ファイルを読み込み (ルートノード, バージョン) を返す。"""
    with open(path, "rb") as stream:
        data = stream.read()
    if len(data) < 32:
        raise FbxError("FBX ファイルが小さすぎます: %s" % path)
    if data.startswith(BINARY_MAGIC):
        return _read_binary(data)
    if data[:512].lstrip().startswith(b";") or b"FBXHeaderExtension" in data[:4096]:
        return _read_ascii(data)
    raise FbxError("FBX ファイルとして認識できません: %s" % path)


def dump(node, depth=0, max_array=8, stream=None):
    """ノードツリーを人間が読める形で出力する(デバッグ用)。"""
    stream = stream or sys.stdout
    if node.name:
        parts = []
        for value in node.props:
            if isinstance(value, array.array):
                head = ", ".join(str(v) for v in value[:max_array])
                suffix = ", ..." if len(value) > max_array else ""
                parts.append("[%d: %s%s]" % (len(value), head, suffix))
            elif isinstance(value, bytes):
                parts.append("<%d bytes>" % len(value))
            else:
                parts.append(repr(value))
        stream.write("%s%s: %s\n" % ("  " * depth, node.name, ", ".join(parts)))
        depth += 1
    for child in node.children:
        dump(child, depth, max_array, stream)
