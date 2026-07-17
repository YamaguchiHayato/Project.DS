/*!
 * @brief	モデルシェーダー。
 */

////////////////////////////////////////////////
// ライト構造体
////////////////////////////////////////////////
struct DirectionLight
{
    float3 dirDirection; //ディレクションライトの方向
    float3 dirColor; //ディレクションライトのカラー
};
struct PointLight
{
    float3 ptPosition; //ポイントライトの位置
    float3 ptColor; //ポイントライトのカラー
    float ptRange; //ポイントライトの影響範囲
};
struct SpotLight
{
    float3 spPosition; //スポットライトの位置
    float3 spColor; //スポットライトのカラー
    float spRange; //スポットライトの射出範囲
    float3 spDirection; //スポットライトの射出方向
    float spAngle; //スポットライトの射出角度
};
struct HemLight
{
    float3 heGroundColor; //照り返しのライト
    float3 heSkyColor; //天球ライト
    float3 heGroundNormal; //地面の法線
};

////////////////////////////////////////////////
// 定数バッファ。
////////////////////////////////////////////////
//モデル用の定数バッファ
cbuffer ModelCb : register(b0)
{
    float4x4 mWorld;
    float4x4 mView;
    float4x4 mProj;
};
//ディレクションライト用の定数バッファ
cbuffer directionLightCb : register(b1)
{
    ////ディレクションライト用
    //DirectionLight directionLight;

	
    ////ポイントライト用
    //PointLight pointLight;
	
    ////スポットライト用
    //SpotLight spotLight;
	
    ////ライト用
    //float3 eyePos; //視点のデータ

    //float3 ambientLight; //アンビエントライトの強さ
   	
    ////半球ライト用
    //HemLight hemLight;

   ////ライトビュースクリーン用
   // float4x4 mLVP;
    
   // //グレースケール設定用
   // bool setGrayScale;
    
    float3 dirDirection; //ディレクションライトの方向
    float3 dirColor; //ディレクションライトのカラー
    
    float3 ptPosition; //ポイントライトの位置
    float3 ptColor; //ポイントライトのカラー
    float ptRange; //ポイントライトの影響範囲
    
    float3 spPosition; //スポットライトの位置
    float3 spColor; //スポットライトのカラー
    float spRange; //スポットライトの射出範囲
    float3 spDirection; //スポットライトの射出方向
    float spAngle; //スポットライトの射出角度
    
     float3 eyePos; //視点のデータ
    float3 ambientLight; //アンビエントライトの強さ
   	
    float3 heGroundColor; //照り返しのライト
    float3 heSkyColor; //天球ライト
    float3 heGroundNormal; //地面の法線
}


////////////////////////////////////////////////
// 構造体
////////////////////////////////////////////////
//スキニング用の頂点データをひとまとめ。
struct SSkinVSIn
{
    int4 Indices : BLENDINDICES0;
    float4 Weights : BLENDWEIGHT0;
};
//頂点シェーダーへの入力。
struct SVSIn
{
    float4 pos : POSITION; //モデルの頂点座標。
    float2 uv : TEXCOORD0; //UV座標。
    SSkinVSIn skinVert; //スキン用のデータ。
    float3 normal : NORMAL; //法線
    float3 tangent : TANGENT; //接ベクトル
    float3 biNormal : BINORMAL; //従ベクトル
};
//ピクセルシェーダーへの入力。
struct SPSIn
{
    float4 pos : SV_POSITION; //スクリーン空間でのピクセルの座標
    float2 uv : TEXCOORD0; //uv座標
    float3 worldPos : TEXCOORD1; //ワールド座標
    float3 normal : NORMAL; //法線
    float3 tangent : TANGENT; //接ベクトル
    float3 biNormal : BINORMAL; //従ベクトル
    float3 normalInView : TEXCOORD2; //カメラ空間の法線
    float4 posInLVP : TEXCOORD3; //ライトビュースクリーン行列
};

////////////////////////////////////////////////
// グローバル変数。
////////////////////////////////////////////////
Texture2D<float4> g_albedo : register(t0); //アルベドマップ
Texture2D<float4> g_normalMap : register(t1); //法線マップにアクセスするための変数を追加
Texture2D<float4> g_specularMap : register(t2); //スペキュラーマップにアクセスするための変数を追加
StructuredBuffer<float4x4> g_boneMatrix : register(t3); //ボーン行列。
sampler g_sampler : register(s0); //サンプラステート。
//Texture2D<float4> g_shadowMap : register(t11); // シャドウマップ


////////////////////////////////////////////////
// 関数定義。
////////////////////////////////////////////////
float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal);
float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal);
float3 CalcLigFromDirectionLight(SPSIn psIn);
float3 CalcLigFromPointLight(SPSIn psIn, int num);
float3 CalcLigFromSpotLight(SPSIn psIn, int num);
float3 CalcLimPower(SPSIn psIn);
float3 CalcLigFromHemLight(SPSIn psIn);
float3 CalcNormalCalcNormal(float3 normal, float3 tangent, float3 biNormal, float2 uv);
float3 CalcNormalMap(SPSIn psIn);
float3 CalcSpecularMap(SPSIn psIn);

//スキン行列を計算する。
float4x4 CalcSkinMatrix(SSkinVSIn skinVert)
{
    float4x4 skinning = 0;
    float w = 0.0f;
	[unroll]
    for (int i = 0; i < 3; i++)
    {
        skinning += g_boneMatrix[skinVert.Indices[i]] * skinVert.Weights[i];
        w += skinVert.Weights[i];
    }
    
    skinning += g_boneMatrix[skinVert.Indices[3]] * (1.0f - w);
	
    return skinning;
}

/// 頂点シェーダーのコア関数。
SPSIn VSMainCore(SVSIn vsIn, uniform bool hasSkin)
{
    SPSIn psIn;
    
    float4x4 m;
    if (hasSkin)
    {
        m = CalcSkinMatrix(vsIn.skinVert);
    }
    else
    {
        m = mWorld;
    }
    
    
    psIn.pos = mul(m, vsIn.pos); // モデルの頂点をワールド座標系に変換
    
    psIn.worldPos = psIn.pos; // ワールド座標
    float4 s_worldPos = psIn.pos; // シャドウの計算用ワールド座標
    
    psIn.pos = mul(mView, psIn.pos); // ワールド座標系からカメラ座標系に変換
    psIn.pos = mul(mProj, psIn.pos); // カメラ座標系からスクリーン座標系に変換

    
	//頂点法線をピクセルシェーダーに渡す
    psIn.normal = normalize(mul(m, vsIn.normal)); //法線を回転させる
    //接ベクトルと従ベクトルをワールド空間に変換する
    psIn.tangent = normalize(mul(m, vsIn.tangent));
    psIn.biNormal = normalize(mul(m, vsIn.biNormal));

    psIn.uv = vsIn.uv;
	    
    //psIn.posInLVP = mul(mLVP, s_worldPos); //ライトビュースクリーン空間の座標を計算する
    psIn.normalInView = mul(mView, psIn.normal); //カメラ空間の法線を求める
    
    return psIn;
}


// スキンなしメッシュ用の頂点シェーダーのエントリー関数。
SPSIn VSMain(SVSIn vsIn)
{
    return VSMainCore(vsIn, false);
}
// スキンありメッシュの頂点シェーダーのエントリー関数。
SPSIn VSSkinMain(SVSIn vsIn)
{
    return VSMainCore(vsIn, true);
}

/// ピクセルシェーダーのエントリー関数。
float4 PSMain(SPSIn psIn) : SV_Target0
{
	//ディレクションライト(鏡面拡散どっちも)によるライティングを計算
    float3 directionLig = CalcLigFromDirectionLight(psIn);
	
    //ポイントライト(鏡面拡散どっちも)によるライティングを計算
    //スポットライト(鏡面拡散どっちも)によるライティングを計算
    float3 pointLig[10];
    float3 spotLig[10];
    for (int i = 0; i < 10; i++)
    {
        pointLig[i] = CalcLigFromPointLight(psIn, i);
        spotLig[i] = CalcLigFromSpotLight(psIn, i);
    }
	
	//リムの計算
    float3 limColor = CalcLimPower(psIn);
	
	//半球ライトの計算
    float3 hemLig = CalcLigFromHemLight(psIn);
	
	//法線マップの計算
    float3 normalMap = CalcNormalMap(psIn);
    
    //スペキュラーマップの計算
    float3 specularMap = CalcSpecularMap(psIn);
    
	
	//ディレクションライト、ポイントライト、スポットライト、
	//アンビエントライト、半球ライト、法線マップ、スペキュラマップを合算して最終的な光を求める
    float3 lig = directionLig + ambientLight + hemLig + normalMap + specularMap;
    for (int j = 0; j < 10; j++)
    {
        lig += pointLig[j];
        lig += spotLig[j];
    }
    
	//最終的な反射光にリムの反射光を合算する
    lig += limColor;
	
	//テクスチャからカラーをフェッチ
    float4 albedoColor = g_albedo.Sample(g_sampler, psIn.uv);
    
 //   //シャドウの処理
 //   /////////////////////////////////////////////////////////////////////////////
 //   // ライトビュースクリーン空間からUV空間に座標変換
 //   float2 shadowMapUV = psIn.posInLVP.xy / psIn.posInLVP.w;
 //   shadowMapUV *= float2(0.5f, -0.5f);
 //   shadowMapUV += 0.5f;

 //   // ライトビュースクリーン空間でのZ値を計算する
 //   float zInLVP = psIn.posInLVP.z / psIn.posInLVP.w;

 //   //ライトビュースクリーン空間でのUV座標の内側なら
 //   if (shadowMapUV.x > 0.0f && shadowMapUV.x < 1.0f && shadowMapUV.y > 0.0f && shadowMapUV.y < 1.0f)
 //   {
 //       // シャドウマップに描き込まれているZ値と比較し
 //       // 計算したUV座標を使って、シャドウマップから深度値をサンプリング
 //       float zInShadowMap = g_shadowMap.Sample(g_sampler, shadowMapUV).r;
        
 //       //ライトカメラ目線時,描画予定のピクセル(zInLVP)より手前にオブジェクト(zInShadowMap)があるなら
 //       if (zInLVP > zInShadowMap)
 //       {
 //           // 遮蔽されている
 //           albedoColor.xyz *= 0.5f;
 //       }
 //   }
 //   ////////////////////////////////////////////////////////////////////////////////
    
	////最終出力カラーに光を乗算する
    albedoColor.xyz *= lig;
    
 //    //グレースケールを設定する
 //   if (setGrayScale == true)
 //   {
 //       float Y = 0.299f * albedoColor.r + 0.587f * albedoColor.g + 0.114f * albedoColor.b;
        
 //       return Y;
 //   }

    return albedoColor;
}


//ココから関数の定義
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Lambert拡散反射光を計算する
float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal)
{
	// ピクセルの法線とライトの方向の内積を計算する
    float t = dot(normal, lightDirection) * -1.0f;
	// 内積の値を0以上の値にする
    if (t < 0.0f)
    {
        t = 0.0f;
    }

	// 拡散反射光を計算する
    return lightColor * t;
}

//Phong鏡面反射光を計算する
float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal)
{
	// 反射ベクトルを求める
    float3 refVec = reflect(lightDirection, normal);

	// 光が当たったサーフェイスから視点に伸びるベクトルを求める
    float3 toEye = eyePos - worldPos;
    toEye = normalize(toEye);

	// 鏡面反射の強さを求める
    float t = dot(refVec, toEye);
	// 鏡面反射の強さを0以上の数値にする
    if (t < 0.0f)
    {
        t = 0.0f;
    }

	// 鏡面反射の強さを絞る
    t = pow(t, 5.0f);

	// 鏡面反射光を求める
    return lightColor * t;
}

//ポイントライトによる反射光を計算する
float3 CalcLigFromDirectionLight(SPSIn psIn)
{
	//ディレクションライトによるLambert拡散反射光を計算する
    float3 diffDirection = CalcLambertDiffuse(dirDirection, dirColor, psIn.normal);
    //ディレクションライトによるPhong鏡面反射光を計算する
    float3 specDirection = CalcPhongSpecular(dirDirection, dirColor, psIn.worldPos, psIn.normal);

    return diffDirection + specDirection;
}

//ポイントライトによる反射光を計算する
float3 CalcLigFromPointLight(SPSIn psIn, int num)
{
	// このサーフェイスに入射しているポイントライトの光の向きを計算する
    float3 ligDir = psIn.worldPos - ptPosition;

    // 正規化して大きさ1のベクトルにする
    ligDir = normalize(ligDir);

    // 減衰なしのLambert拡散反射光を計算する
    float3 diffPoint = CalcLambertDiffuse(
        ligDir, // ライトの方向
        ptColor, // ライトのカラー
        psIn.normal			// サーフェイスの法線
    );

    // 減衰なしのPhong鏡面反射光を計算する
    float3 specPoint = CalcPhongSpecular(
        ligDir, // ライトの方向
       ptColor, // ライトのカラー
        psIn.worldPos, // サーフェイズのワールド座標
        psIn.normal			// サーフェイズの法線
    );

    // 距離による影響率を計算する
    // ポイントライトとの距離を計算する
    float3 distance = length(psIn.worldPos -ptPosition);

    // 影響率は距離に比例して小さくなっていく
    float affect = 1.0f - 1.0f / ptRange * distance;
    // 影響力がマイナスにならないように補正をかける
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }

    // 影響の仕方を指数関数的にする。今回のサンプルでは3乗している
    affect = pow(affect, 3.0f);

    // 拡散反射光と鏡面反射光に減衰率を乗算して影響を弱める
    diffPoint *= affect;
    specPoint *= affect;

    return diffPoint + specPoint;
}

//スポットライトによる反射光を計算する
float3 CalcLigFromSpotLight(SPSIn psIn, int num)
{
	//サーフェイスに入射するスポットライトの光の向きを計算する
    float3 LigDir = psIn.worldPos - spPosition;
	//正規化
    LigDir = normalize(LigDir);
	//減衰なしのLambert拡散反射光を計算する
    float3 diffSpot = CalcLambertDiffuse(
		LigDir, //スポットライトの方向
		spColor, //スポットライトのカラー
		psIn.normal			//サーフェイスの法線
	);
	//減衰なしのPhong鏡面反射の計算
    float3 specSpot = CalcPhongSpecular(
		LigDir, //ライトの方向
		spColor, //ライトのカラー
		psIn.worldPos, //サーフェイスのワールド座標
		psIn.normal			//サーフェイスの法線
	);
	//スポットライトとの距離を計算する
    float distance = length(psIn.worldPos - spPosition);
	//影響率は距離に比例して小さくなっていく
    float affect = 1.0f - 1.0f / spRange * distance;
	//影響力がマイナスにならないように
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }
	//影響を指数関数的にする
    affect = pow(affect, 3.0f);
	//拡散反射光と鏡面反射光に減衰率を乗算して影響を弱める
    diffSpot *= affect;
    specSpot *= affect;
	//入射光と射出方向の角度を求める
    float angle = dot(LigDir, spDirection);
	//dot()で求めた値をacos()に渡して角度を求める
    angle = abs(acos(angle));
	//角度に比例して小さくなっていく影響率を計算する
    affect = 1.0f - 1.0f / spAngle * angle;
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }
	//影響を指数関数的にする
    affect = pow(affect, 0.5f);
	//角度による影響率を反射光に乗算して、影響を弱める
    diffSpot *= affect;
    specSpot *= affect;
	
    return diffSpot + specSpot;
}

float3 CalcLimPower(SPSIn psIn)
{
	//サーフェイスの法線と光の入射方向に依存するリムの強さを求める
    float power1 = 1.0f - max(0.0f, dot(dirDirection, psIn.normal));
	//サーフェイスの法線と視線の方向に依存するリムの強さを求める
    float power2 = 1.0f - max(0.0f, psIn.normalInView.z * -1.0f);
	//最終的なリムの強さを求める
    float limPower = power1 * power2;
	//pow()を使用し強さの変化を指数関数的にする
    limPower = pow(limPower, 1.3f);
	
	//リムライトのカラーを計算し、返す
    return limPower * dirColor;
}

float3 CalcLigFromHemLight(SPSIn psIn)
{
	//サーフェイスの法線と地面の法線との内積を計算する
    float t = dot(psIn.normal,heGroundNormal);
	//内積の結果を0～1の範囲に変換
    t = (t + 1.0f) / 2.0f;
	//地面と天球色を補完率tで線形補完し、返す
    return lerp(heGroundColor,heSkyColor, t);
}

float3 CalcNormal(float3 normal, float3 tangent, float3 biNormal, float2 uv)
{
    float3 binSpaceNormal = g_normalMap.SampleLevel(g_sampler, uv, 0.0f).xyz;
    binSpaceNormal = (binSpaceNormal * 2.0f) - 1.0f;
    
    float3 newNormal = tangent * binSpaceNormal.x + biNormal * binSpaceNormal.y + normal * binSpaceNormal.z;

    return newNormal;
}

float3 CalcNormalMap(SPSIn psIn)
{
    //ディフュ－ズマップをサンプリング
    float4 diffuseMap = g_albedo.Sample(g_sampler, psIn.uv);
    float3 normal = psIn.normal;
    //法線マップからタンジェントスペースの法線をサンプリングする
    float3 localNormal = g_normalMap.Sample(g_sampler, psIn.uv).xyz;
    //タンジェントスペースの法線を0～1の範囲から-1～1の範囲に復元する
    localNormal = (localNormal - 0.5f) * 2.0f;
    //タンジェントスペースの法線をワールドスペースに変換する
    normal = psIn.tangent * localNormal.x
           + psIn.biNormal * localNormal.y
           + normal * localNormal.z;
    //拡散反射を計算し、返す
    return max(0.0f, dot(normal, -dirDirection)) * dirColor;
}

float3 CalcSpecularMap(SPSIn psIn)
{
    //ディフューズマップをサンプリング
    float4 diffuseMap = g_albedo.Sample(g_sampler, psIn.uv);
    //ノーマルマップを元に法線を計算
    float3 normal = CalcNormal(psIn.normal, psIn.tangent, psIn.biNormal, psIn.uv);
    //拡散反射光を計算
    float3 diffuseLig = max(0.0f, dot(normal, -dirDirection)) * dirColor;
    
    //鏡面反射光を計算
    //反射ベクトルを求める
    float3 refVec = reflect(dirDirection, normal);
    //光が当たったサーフェイスから視点に伸びるベクトルを求める
    float3 toEye = eyePos - psIn.worldPos;
    toEye = normalize(toEye);
    //鏡面反射の強さを求める
    float t = saturate(dot(refVec, toEye));
    //鏡面反射の強さを絞る
    t = pow(t, 5.0f);
    //鏡面反射光を求める
    float3 specLig = dirColor * t;
    
    //スペキュラマップからスペキュラ反射の強さをサンプリング
    float specPower = g_specularMap.Sample(g_sampler, psIn.uv).r;
    //鏡面反射の強さを鏡面反射光に乗算する
    specLig *= specPower * 50.0f; // ＜ー ここの倍率変更で光り方が変わる
    
    return specLig;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////