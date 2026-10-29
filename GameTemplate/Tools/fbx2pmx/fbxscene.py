# -*- coding: utf-8 -*-
"""FBX のノードツリーからシーン情報(メッシュ・スキン・マテリアル等)を取り出す。

fbxfile.py が読み込んだ生のノードツリーを、変換処理が扱いやすい形へ
整理するだけの層。PMX 固有の事情はここには持ち込まない。
"""

from __future__ import annotations

import os

import mat4
from fbxfile import FbxError, read_fbx

# FBX のオブジェクト名は "名前\x00\x01クラス名" の形で格納される
_NAME_SEPARATOR = "\x00\x01"

# GlobalSettings の軸番号 -> 単位ベクトル
_AXIS_VECTORS = ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))


def _split_object_name(raw):
    """オブジェクト名を取り出す。

    バイナリ FBX は 名前・区切り・クラス名、ASCII FBX は
    "クラス名::名前" の形で格納されている。
    """
    if not isinstance(raw, str):
        return ""
    if _NAME_SEPARATOR in raw:
        return raw.split(_NAME_SEPARATOR, 1)[0]
    if "::" in raw:
        return raw.split("::", 1)[1]
    return raw


class FbxObject:
    """Objects セクションのオブジェクト 1 件。"""

    __slots__ = ("id", "class_name", "name", "sub_class", "node", "props")

    def __init__(self, obj_id, class_name, name, sub_class, node):
        self.id = obj_id
        self.class_name = class_name
        self.name = name
        self.sub_class = sub_class
        self.node = node
        self.props = _read_properties70(node)

    def prop(self, name, default=None):
        """Properties70 の値リストを返す。"""
        return self.props.get(name, default)

    def prop_vec3(self, name, default=(0.0, 0.0, 0.0)):
        values = self.props.get(name)
        if not values or len(values) < 3:
            return default
        return (float(values[0]), float(values[1]), float(values[2]))

    def prop_float(self, name, default=0.0):
        values = self.props.get(name)
        if not values:
            return default
        try:
            return float(values[0])
        except (TypeError, ValueError):
            return default

    def prop_int(self, name, default=0):
        values = self.props.get(name)
        if not values:
            return default
        try:
            return int(values[0])
        except (TypeError, ValueError):
            return default

    def __repr__(self):
        return "FbxObject(%s/%s %r)" % (self.class_name, self.sub_class, self.name)


def _read_properties70(node):
    """Properties70 を {プロパティ名: [値, ...]} にする。"""
    result = {}
    container = node.find("Properties70") or node.find("Properties60")
    if container is None:
        return result
    for prop in container.children:
        if not prop.props:
            continue
        key = prop.props[0]
        if not isinstance(key, str):
            continue
        result[key] = list(prop.props[4:]) if len(prop.props) > 4 else []
    return result


class SkinCluster:
    """スキンのクラスタ(ボーン 1 本分の影響)。"""

    __slots__ = ("bone", "indexes", "weights", "transform", "transform_link")

    def __init__(self, bone, indexes, weights, transform, transform_link):
        self.bone = bone
        self.indexes = indexes
        self.weights = weights
        self.transform = transform
        self.transform_link = transform_link


class ShapeData:
    """ブレンドシェイプ(モーフ)1 件。"""

    __slots__ = ("name", "indexes", "deltas")

    def __init__(self, name, indexes, deltas):
        self.name = name
        self.indexes = indexes
        self.deltas = deltas


class MeshData:
    """メッシュ 1 個分の形状・スキン・マテリアル情報。"""

    def __init__(self, name, model=None):
        self.name = name
        self.model = model          # 元になった Model オブジェクト
        self.control_points = []     # [(x, y, z), ...] ジオメトリ座標系
        self.polygons = []           # [[制御点インデックス, ...], ...]
        self.corner_normals = None   # ポリゴン頂点ごとの法線
        self.corner_uvs = None       # ポリゴン頂点ごとの UV
        self.polygon_materials = None  # ポリゴンごとのマテリアル番号(メッシュ内)
        self.materials = []          # [FbxObject, ...]
        self.clusters = []           # [SkinCluster, ...]
        self.shapes = []             # [ShapeData, ...]
        self.bind_matrix = mat4.IDENTITY  # 制御点 -> バインド時ワールド

    @property
    def corner_count(self):
        return sum(len(poly) for poly in self.polygons)


class FbxScene:
    """FBX シーン全体。"""

    def __init__(self, path):
        self.path = path
        self.root, self.version = read_fbx(path)
        self.objects = {}
        self.child_links = {}   # 親 id -> [(子 id, プロパティ名 or None)]
        self.parent_links = {}  # 子 id -> [(親 id, プロパティ名 or None)]
        self._global_cache = {}
        self._build_objects()
        self._build_connections()
        self._read_global_settings()

    # ------------------------------------------------------------------
    # 構築
    # ------------------------------------------------------------------
    def _build_objects(self):
        container = self.root.find("Objects")
        if container is None:
            raise FbxError("Objects セクションが見つかりません")
        for node in container.children:
            if not node.props:
                continue
            obj_id = node.props[0]
            if not isinstance(obj_id, int):
                continue
            name = _split_object_name(node.prop(1, ""))
            sub_class = node.prop(2, "")
            self.objects[obj_id] = FbxObject(obj_id, node.name, name, sub_class, node)

    def _build_connections(self):
        container = self.root.find("Connections")
        if container is None:
            return
        for node in container.children:
            props = node.props
            if len(props) < 3:
                continue
            kind, child_id, parent_id = props[0], props[1], props[2]
            prop_name = props[3] if (kind == "OP" and len(props) > 3) else None
            self.child_links.setdefault(parent_id, []).append((child_id, prop_name))
            self.parent_links.setdefault(child_id, []).append((parent_id, prop_name))

    def _read_global_settings(self):
        settings = self.root.find("GlobalSettings")
        props = _read_properties70(settings) if settings is not None else {}

        def value(name, default):
            values = props.get(name)
            if not values:
                return default
            try:
                return type(default)(values[0])
            except (TypeError, ValueError):
                return default

        self.up_axis = value("UpAxis", 1)
        self.up_sign = value("UpAxisSign", 1)
        self.front_axis = value("FrontAxis", 2)
        self.front_sign = value("FrontAxisSign", 1)
        self.coord_axis = value("CoordAxis", 0)
        self.coord_sign = value("CoordAxisSign", 1)
        self.unit_scale_factor = value("UnitScaleFactor", 1.0)  # 1 単位あたりの cm 数

    # ------------------------------------------------------------------
    # 参照
    # ------------------------------------------------------------------
    def children_of(self, obj_id, class_name=None, sub_class=None):
        """obj_id に接続された子オブジェクトを接続順に返す。"""
        result = []
        for child_id, prop_name in self.child_links.get(obj_id, ()):
            child = self.objects.get(child_id)
            if child is None:
                continue
            if class_name is not None and child.class_name != class_name:
                continue
            if sub_class is not None and child.sub_class != sub_class:
                continue
            result.append((child, prop_name))
        return result

    def child_objects(self, obj_id, class_name=None, sub_class=None):
        return [obj for obj, _ in self.children_of(obj_id, class_name, sub_class)]

    def parent_model(self, obj_id):
        """モデル階層上の親 Model を返す。無ければ None。"""
        for parent_id, prop_name in self.parent_links.get(obj_id, ()):
            if prop_name is not None:
                continue
            parent = self.objects.get(parent_id)
            if parent is not None and parent.class_name == "Model":
                return parent
        return None

    def models(self, sub_class=None):
        return [obj for obj in self.objects.values()
                if obj.class_name == "Model"
                and (sub_class is None or obj.sub_class == sub_class)]

    # ------------------------------------------------------------------
    # 変換行列
    # ------------------------------------------------------------------
    def local_matrix(self, model, take=None, time=None):
        """FBX の変換式に従ってローカル行列を組み立てる。

        take と time を渡すと、アニメーションのある値はその時刻の値を使う。
        """
        translation = model.prop_vec3("Lcl Translation")
        rotation = model.prop_vec3("Lcl Rotation")
        scaling = model.prop_vec3("Lcl Scaling", (1.0, 1.0, 1.0))
        if take is not None:
            translation = take.vector(model.id, "Lcl Translation", time, translation)
            rotation = take.vector(model.id, "Lcl Rotation", time, rotation)
            scaling = take.vector(model.id, "Lcl Scaling", time, scaling)
        rotation_offset = model.prop_vec3("RotationOffset")
        rotation_pivot = model.prop_vec3("RotationPivot")
        pre_rotation = model.prop_vec3("PreRotation")
        post_rotation = model.prop_vec3("PostRotation")
        scaling_offset = model.prop_vec3("ScalingOffset")
        scaling_pivot = model.prop_vec3("ScalingPivot")
        order = model.prop_int("RotationOrder", 0)

        return mat4.multiply_all(
            mat4.translation(*translation),
            mat4.translation(*rotation_offset),
            mat4.translation(*rotation_pivot),
            mat4.euler_degrees(*pre_rotation, order=0),
            mat4.euler_degrees(*rotation, order=order),
            mat4.inverse(mat4.euler_degrees(*post_rotation, order=0)),
            mat4.inverse(mat4.translation(*rotation_pivot)),
            mat4.translation(*scaling_offset),
            mat4.translation(*scaling_pivot),
            mat4.scaling(*scaling),
            mat4.inverse(mat4.translation(*scaling_pivot)),
        )

    def global_matrix(self, model):
        """ルートからのグローバル行列(ジオメトリ変換は含まない)。"""
        cached = self._global_cache.get(model.id)
        if cached is not None:
            return cached
        local = self.local_matrix(model)
        parent = self.parent_model(model.id)
        result = local if parent is None else mat4.multiply(self.global_matrix(parent), local)
        self._global_cache[model.id] = result
        return result

    def global_matrix_at(self, model, take, time, cache):
        """アニメーションの time 時点でのグローバル行列。cache は時刻ごとの辞書。"""
        cached = cache.get(model.id)
        if cached is not None:
            return cached
        local = self.local_matrix(model, take, time)
        parent = self.parent_model(model.id)
        result = local if parent is None else mat4.multiply(
            self.global_matrix_at(parent, take, time, cache), local)
        cache[model.id] = result
        return result

    def bind_pose_matrices(self):
        """Pose(BindPose) に記録されたグローバル行列を {Model id: 行列} で返す。"""
        result = {}
        for pose in self.objects.values():
            if pose.class_name != "Pose" or pose.sub_class != "BindPose":
                continue
            for pose_node in pose.node.find_all("PoseNode"):
                node_id = pose_node.child_value("Node")
                matrix = pose_node.child_array("Matrix")
                if isinstance(node_id, int) and matrix is not None and len(matrix) == 16:
                    result.setdefault(node_id, mat4.transpose(tuple(float(v) for v in matrix)))
        return result

    def takes(self):
        """アニメーション(AnimationStack)の一覧。"""
        stacks = [obj for obj in self.objects.values() if obj.class_name == "AnimationStack"]
        return [AnimationTake(self, stack) for stack in stacks]

    def geometric_matrix(self, model):
        """ジオメトリにだけ掛かる変換(子ノードには伝播しない)。"""
        translation = model.prop_vec3("GeometricTranslation")
        rotation = model.prop_vec3("GeometricRotation")
        scaling = model.prop_vec3("GeometricScaling", (1.0, 1.0, 1.0))
        if translation == (0.0, 0.0, 0.0) and rotation == (0.0, 0.0, 0.0) \
                and scaling == (1.0, 1.0, 1.0):
            return mat4.IDENTITY
        return mat4.multiply_all(
            mat4.translation(*translation),
            mat4.euler_degrees(*rotation, order=0),
            mat4.scaling(*scaling),
        )

    # ------------------------------------------------------------------
    # 軸・単位
    # ------------------------------------------------------------------
    def axis_vectors(self):
        """(右方向, 上方向, 正面方向) を FBX 座標系の単位ベクトルで返す。"""
        def axis(index, sign):
            base = _AXIS_VECTORS[index if 0 <= index < 3 else 0]
            factor = -1.0 if sign < 0 else 1.0
            return tuple(v * factor for v in base)
        return (axis(self.coord_axis, self.coord_sign),
                axis(self.up_axis, self.up_sign),
                axis(self.front_axis, self.front_sign))

    def meters_per_unit(self):
        """1 FBX 単位が何メートルかを返す。"""
        return float(self.unit_scale_factor) / 100.0


# --------------------------------------------------------------------------
# アニメーション
# --------------------------------------------------------------------------

# FBX の時間単位(1 秒あたりのカウント)
FBX_TIME_PER_SECOND = 46186158000


class AnimationCurve:
    """1 チャンネル分のキー列。キーの間は線形(定数補間のキーは階段)で補間する。

    ネットで配布されている FBX モーションは毎フレームにキーが打たれている
    (ベイク済み)ことがほとんどなので、線形補間で十分な精度になる。
    """

    _CONSTANT = 0x00000002   # KeyAttrFlags の定数補間ビット

    def __init__(self, node):
        self.times = [int(t) for t in (node.child_array("KeyTime") or [])]
        self.values = [float(v) for v in (node.child_array("KeyValueFloat")
                                          or node.child_array("KeyValueDouble") or [])]
        count = min(len(self.times), len(self.values))
        self.times = self.times[:count]
        self.values = self.values[:count]
        flags = node.child_array("KeyAttrFlags") or [0]
        counts = node.child_array("KeyAttrRefCount") or [count]
        self.constant = []
        for flag, repeat in zip(flags, counts):
            self.constant.extend([bool(int(flag) & self._CONSTANT)] * int(repeat))

    def evaluate(self, time, default=0.0):
        times = self.times
        if not times:
            return default
        if time <= times[0]:
            return self.values[0]
        if time >= times[-1]:
            return self.values[-1]
        low, high = 0, len(times) - 1
        while high - low > 1:
            middle = (low + high) // 2
            if times[middle] <= time:
                low = middle
            else:
                high = middle
        if low < len(self.constant) and self.constant[low]:
            return self.values[low]
        span = times[high] - times[low]
        f = (time - times[low]) / float(span) if span else 0.0
        return self.values[low] + (self.values[high] - self.values[low]) * f


class AnimationTake:
    """AnimationStack 1 本分(最初のレイヤーを使う)。"""

    def __init__(self, scene, stack):
        self.name = stack.name
        self.channels = {}          # (Model id, プロパティ名) -> [X, Y, Z の曲線 or None, 既定値]
        self.morph_curves = {}      # BlendShapeChannel id -> 曲線(DeformPercent)
        times = []
        layers = scene.child_objects(stack.id, "AnimationLayer")
        for layer in layers[:1]:
            for curve_node in scene.child_objects(layer.id, "AnimationCurveNode"):
                curves = {}
                for curve, prop_name in scene.children_of(curve_node.id, "AnimationCurve"):
                    curves[prop_name or ""] = AnimationCurve(curve.node)
                    times.extend(curves[prop_name or ""].times[:1] + curves[prop_name or ""].times[-1:])
                for target_id, prop_name in scene.parent_links.get(curve_node.id, ()):
                    target = scene.objects.get(target_id)
                    if target is None or prop_name is None:
                        continue
                    if target.class_name == "Model":
                        defaults = (curve_node.prop_float("d|X"), curve_node.prop_float("d|Y"),
                                    curve_node.prop_float("d|Z"))
                        self.channels[(target.id, prop_name)] = [
                            curves.get("d|X"), curves.get("d|Y"), curves.get("d|Z"), defaults]
                    elif target.class_name == "Deformer" and "DeformPercent" in prop_name:
                        curve = curves.get("d|DeformPercent") or next(iter(curves.values()), None)
                        if curve is not None:
                            self.morph_curves[target.id] = curve
        start = stack.prop("LocalStart") or stack.prop("ReferenceStart")
        stop = stack.prop("LocalStop") or stack.prop("ReferenceStop")
        self.start = int(start[0]) if start else (min(times) if times else 0)
        self.stop = int(stop[0]) if stop else (max(times) if times else 0)

    @property
    def duration_seconds(self):
        return max(0.0, (self.stop - self.start) / float(FBX_TIME_PER_SECOND))

    def has_motion(self):
        """キーが 2 個以上ある曲線が 1 本でもあるか。"""
        for curves in self.channels.values():
            if any(curve is not None and len(curve.times) > 1 for curve in curves[:3]):
                return True
        return any(len(curve.times) > 1 for curve in self.morph_curves.values())

    def vector(self, model_id, prop_name, time, default):
        channel = self.channels.get((model_id, prop_name))
        if channel is None:
            return default
        result = []
        for axis in range(3):
            curve = channel[axis]
            if curve is None or not curve.times:
                result.append(default[axis])
            else:
                result.append(curve.evaluate(time, default[axis]))
        return tuple(result)


# --------------------------------------------------------------------------
# メッシュの取り出し
# --------------------------------------------------------------------------

def _read_layer_element(geometry_node, element_name, data_name, index_name):
    """LayerElement* を (mapping, reference, values, indices) で返す。"""
    element = geometry_node.find(element_name)
    if element is None:
        return None
    mapping = element.child_value("MappingInformationType", 0, "")
    reference = element.child_value("ReferenceInformationType", 0, "")
    values = element.child_array(data_name)
    indices = element.child_array(index_name) if index_name else None
    if values is None:
        return None
    return (mapping, reference, values, indices)


def _layer_index(layer, corner_index, polygon_index, control_point_index):
    """レイヤ要素の値配列を引くためのインデックスを求める。"""
    mapping, reference, _values, indices = layer
    if mapping == "ByPolygonVertex":
        base = corner_index
    elif mapping in ("ByVertice", "ByVertex", "ByControlPoint"):
        base = control_point_index
    elif mapping == "ByPolygon":
        base = polygon_index
    elif mapping == "AllSame":
        base = 0
    else:
        base = corner_index
    if reference in ("IndexToDirect", "Index") and indices is not None:
        if 0 <= base < len(indices):
            return indices[base]
        return 0
    return base


def _build_polygons(polygon_vertex_index):
    """PolygonVertexIndex を多角形ごとのインデックス列に分解する。"""
    polygons = []
    current = []
    for raw in polygon_vertex_index:
        if raw < 0:
            current.append(-raw - 1)
            polygons.append(current)
            current = []
        else:
            current.append(raw)
    if current:
        polygons.append(current)
    return polygons


def _read_matrix(values):
    """FBX の 16 要素行列(行ベクトル規約)を列ベクトル規約へ転置して返す。"""
    if not values or len(values) != 16:
        return None
    return mat4.transpose(tuple(float(v) for v in values))


def _collect_clusters(scene, geometry):
    """ジオメトリに付いたスキンのクラスタを集める。"""
    clusters = []
    for skin in scene.child_objects(geometry.id, "Deformer", "Skin"):
        for cluster in scene.child_objects(skin.id, "Deformer", "Cluster"):
            bones = scene.child_objects(cluster.id, "Model")
            if not bones:
                continue
            node = cluster.node
            indexes = node.child_array("Indexes") or []
            weights = node.child_array("Weights") or []
            transform = node.child_array("Transform")
            transform_link = node.child_array("TransformLink")
            clusters.append(SkinCluster(
                bones[0],
                indexes,
                weights,
                _read_matrix(transform),
                _read_matrix(transform_link),
            ))
    return clusters


def _collect_shapes(scene, geometry):
    """ジオメトリに付いたブレンドシェイプを集める。"""
    shapes = []
    for blend_shape in scene.child_objects(geometry.id, "Deformer", "BlendShape"):
        for channel in scene.child_objects(blend_shape.id, "Deformer", "BlendShapeChannel"):
            for shape in scene.child_objects(channel.id, "Geometry", "Shape"):
                indexes = shape.node.child_array("Indexes")
                vertices = shape.node.child_array("Vertices")
                if not indexes or not vertices:
                    continue
                deltas = [(float(vertices[i * 3]), float(vertices[i * 3 + 1]),
                           float(vertices[i * 3 + 2])) for i in range(len(indexes))]
                shapes.append(ShapeData(channel.name or shape.name,
                                        [int(v) for v in indexes], deltas))
    return shapes


def _matrices_match(a, b, tolerance=1e-3):
    return all(abs(x - y) <= tolerance * max(1.0, abs(x), abs(y)) for x, y in zip(a, b))


def _resolve_bind_matrix(scene, model, clusters):
    """制御点をバインド時のワールド座標へ移す行列を決める。

    FBX ファイル上のクラスタは Transform = inverse(TransformLink) * メッシュの
    バインド時ワールド行列 を持つのが一般的だが、Transform にメッシュのワールド
    行列をそのまま書き出すエクスポータもある。両方に対応するため、クラスタ間で
    一貫する解釈を採用し、どちらも破綻する場合はノードのグローバル行列を使う。
    """
    node_matrix = mat4.multiply(scene.global_matrix(model), scene.geometric_matrix(model))
    usable = [c for c in clusters if c.transform is not None and c.transform_link is not None]
    if not usable:
        return node_matrix

    combined = [mat4.multiply(c.transform_link, c.transform) for c in usable]
    if all(_matrices_match(combined[0], m) for m in combined):
        return combined[0]

    transforms = [c.transform for c in usable]
    if all(_matrices_match(transforms[0], m) for m in transforms):
        return transforms[0]

    return node_matrix


def build_mesh(scene, model):
    """Model(Mesh) 1 個から MeshData を作る。ジオメトリが無ければ None。"""
    geometries = scene.child_objects(model.id, "Geometry", "Mesh")
    if not geometries:
        return None
    geometry = geometries[0]
    node = geometry.node

    raw_vertices = node.child_array("Vertices")
    raw_indices = node.child_array("PolygonVertexIndex")
    if not raw_vertices or not raw_indices:
        return None

    mesh = MeshData(model.name or geometry.name, model)
    mesh.control_points = [
        (float(raw_vertices[i]), float(raw_vertices[i + 1]), float(raw_vertices[i + 2]))
        for i in range(0, len(raw_vertices) - 2, 3)
    ]
    mesh.polygons = _build_polygons(raw_indices)

    normal_layer = _read_layer_element(node, "LayerElementNormal", "Normals", "NormalsIndex")
    uv_layer = _read_layer_element(node, "LayerElementUV", "UV", "UVIndex")
    material_layer = _read_layer_element(node, "LayerElementMaterial", "Materials", None)

    corner_normals = [] if normal_layer else None
    corner_uvs = [] if uv_layer else None
    polygon_materials = [] if material_layer else None

    corner_index = 0
    for polygon_index, polygon in enumerate(mesh.polygons):
        for control_point_index in polygon:
            if normal_layer is not None:
                index = _layer_index(normal_layer, corner_index, polygon_index,
                                     control_point_index) * 3
                values = normal_layer[2]
                if index + 2 < len(values):
                    corner_normals.append((float(values[index]), float(values[index + 1]),
                                           float(values[index + 2])))
                else:
                    corner_normals.append((0.0, 1.0, 0.0))
            if uv_layer is not None:
                index = _layer_index(uv_layer, corner_index, polygon_index,
                                     control_point_index) * 2
                values = uv_layer[2]
                if index + 1 < len(values):
                    corner_uvs.append((float(values[index]), float(values[index + 1])))
                else:
                    corner_uvs.append((0.0, 0.0))
            corner_index += 1
        if material_layer is not None:
            values = material_layer[2]
            index = _layer_index(material_layer, corner_index - 1, polygon_index, polygon[0])
            polygon_materials.append(int(values[index]) if 0 <= index < len(values) else 0)

    mesh.corner_normals = corner_normals
    mesh.corner_uvs = corner_uvs
    mesh.polygon_materials = polygon_materials
    mesh.materials = scene.child_objects(model.id, "Material")
    mesh.clusters = _collect_clusters(scene, geometry)
    mesh.shapes = _collect_shapes(scene, geometry)

    mesh.bind_matrix = _resolve_bind_matrix(scene, model, mesh.clusters)
    return mesh


def build_meshes(scene):
    """シーン中のすべての Model(Mesh) から MeshData を作る。"""
    meshes = []
    for model in scene.models("Mesh"):
        mesh = build_mesh(scene, model)
        if mesh is not None and mesh.polygons:
            meshes.append(mesh)
    meshes.sort(key=lambda m: m.name)
    return meshes


# --------------------------------------------------------------------------
# テクスチャ
# --------------------------------------------------------------------------

_DIFFUSE_PROPERTIES = (
    "DiffuseColor", "Maya|baseColor", "base_color_map", "DiffuseFactor",
    "3dsMax|Parameters|base_color_map", "TransparentColor",
)


def texture_path(texture):
    """Texture オブジェクトから相対パス(無ければ絶対パス)を取り出す。"""
    node = texture.node
    for key in ("RelativeFilename", "FileName", "Filename"):
        value = node.child_value(key)
        if isinstance(value, str) and value.strip():
            return value.replace("\\", "/")
    values = texture.prop("Path") or texture.prop("RelPath")
    if values and isinstance(values[0], str):
        return values[0].replace("\\", "/")
    return None


def find_diffuse_texture(scene, material):
    """マテリアルに接続された拡散色テクスチャを返す。"""
    textures = scene.children_of(material.id, "Texture")
    if not textures:
        for layered, _ in scene.children_of(material.id, "LayeredTexture"):
            textures = scene.children_of(layered.id, "Texture")
            if textures:
                break
    if not textures:
        return None
    for texture, prop_name in textures:
        if prop_name and any(key.lower() in prop_name.lower() for key in _DIFFUSE_PROPERTIES):
            return texture
    for texture, prop_name in textures:
        if prop_name and "color" in prop_name.lower():
            return texture
    return textures[0][0]


def resolve_texture_file(fbx_path, relative):
    """FBX からの相対パスを実ファイルに解決する。見つからなければ None。"""
    if not relative:
        return None
    base = os.path.dirname(os.path.abspath(fbx_path))
    candidate = os.path.normpath(os.path.join(base, relative))
    if os.path.isfile(candidate):
        return candidate
    name = os.path.basename(relative)
    for directory in (base, os.path.join(base, "textures"), os.path.join(base, "tex")):
        candidate = os.path.join(directory, name)
        if os.path.isfile(candidate):
            return os.path.normpath(candidate)
    return None
