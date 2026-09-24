#include "stdafx.h"
#include "Src/System/ModelBounds.h"

namespace nsApp
{
	namespace nsSystem
	{
		float ModelBounds::GetLongestEdge() const
		{
			/* 測れていなければ大きさは無い。*/
			if (!bIsValid_)
				return 0.0f;

			const Vector3 vSize = GetSize();
			float fLongest = vSize.x;
			if (vSize.y > fLongest)
				fLongest = vSize.y;

			if (vSize.z > fLongest)
				fLongest = vSize.z;

			return fLongest;
		}


		ModelBounds MeasureModelBounds(Model& model)
		{
			ModelBounds stBounds;

			/* 最小は大きな値、最大は小さな値から始めて、頂点で狭めていく。*/
			Vector3 vMin = { 1e30f, 1e30f, 1e30f };
			Vector3 vMax = { -1e30f, -1e30f, -1e30f };

			model.GetTkmFile().QueryMeshParts(
				[&](const TkmFile::SMesh& mesh)
				{
					for (const auto& vertex : mesh.vertexBuffer)
					{
						const Vector3& p = vertex.pos;
						if (p.x < vMin.x) vMin.x = p.x;
						if (p.y < vMin.y) vMin.y = p.y;
						if (p.z < vMin.z) vMin.z = p.z;
						if (p.x > vMax.x) vMax.x = p.x;
						if (p.y > vMax.y) vMax.y = p.y;
						if (p.z > vMax.z) vMax.z = p.z;
					}
				});

			/* 頂点が1つも無ければ最小が最大を越えたまま残る。*/
			if (vMax.x < vMin.x)
				return stBounds;

			stBounds.vMin_ = vMin;
			stBounds.vMax_ = vMax;
			stBounds.bIsValid_ = true;
			return stBounds;
		}
	}
}
