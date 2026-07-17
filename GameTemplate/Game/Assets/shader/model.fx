

////////////////////////////////////////////////
//モデルシェーダー
////////////////////////////////////////////////
//スキニング用の頂点データをひとまとめ。
struct SSkinVSIn
{
    int4 Indices : BLENDINDICES0;
    float4 Weights : BLENDWEIGHT0;
};
//頂点シェーダーの入力
struct SVSIn
{
    float4 pos : POSITION; //位置
    float3 normal : NORMAL; //法線
    float2 uv : TEXCOORD0; //UV
    float3 tangent : TANGENT; //接ベクトル
    float3 biNormal : BINORMAL; //従ベクトル
    SSkinVSIn skinVert; //スキン用のデータ
};
//ピクセルシェーダーの入力
struct SPSIn
{
    float4 pos : SV_POSITION; //位置
    float3 normal : NORMAL; //法線
    float2 uv : TEXCOORD0; //UV
    float3 tangent : TANGENT; //接ベクトル
    float3 biNormal : BINORMAL; //従ベクトル
    float3 worldPos : TEXCOORD1; //ワールド空間座標
    float3 normalInView : TEXCOORD2; //カメラ空間の法線
    float4 posInLVP : TEXCOORD3; //ライトビュースクリーン空間でのピクセルの座標
   
};

struct SPSOut
{
    float4 color : SV_Target0;
};


//ディレクションライト
struct DirectionLight
{
    float3 direction; //ライトの方向
    float3 color; //ライトのカラー
};

//ポイントライト
struct PointLight
{
    float3 position; //ライトの地
    float3 color; //ライトのカラー
    float range; //ライトの影響範囲
};

//スポットライト
struct SpotLight
{
    float3 position; //ライトの位置
    float3 color; //ライトのカラー
    float range; //ライトの影響範囲
    float3 direction; //ライトの放射方向
    float angle; //ライトの放射角度
};

//半球ライト
struct HemLight
{
    float3 groundColor; //地面色
    float3 skyColor; //天球色
    float3 groundNormal; //地面の法線
};

//モデル用の定数バッファ
cbuffer ModelCb : register(b0)
{
    float4x4 mWorld;
    float4x4 mView;
    float4x4 mProj;
};


//ライトの定数バッファ
cbuffer DirectionLightCb : register(b1)
{
    DirectionLight directionLight; //ディレクションライト
    float3 eyePos; //視点の位置
    float3 ambientLig; //環境光
    PointLight pointLight[10]; //ポイントライト
    SpotLight spotLight[10]; //スポットライト
    HemLight hemLight; //半球ライト
    float4x4 mLVP; //ライトビュースクリーン
}
////////////////////////////////////////////////
// 讒矩菴・
////////////////////////////////////////////////

////////////////////////////////////////////////
// 繧ｰ繝ｭ繝ｼ繝舌Ν螟画焚縲・
////////////////////////////////////////////////
Texture2D<float4> g_albedo : register(t0); //アルベドマップ
Texture2D<float4> g_normalMap : register(t1); //法線マップ
Texture2D<float4> g_specularMap : register(t2);//スペキュラマップ
//Texture2D<float4> g_aoMap : register(t10); //AOマップ
Texture2D<float4> g_shadowMap : register(t10); //シャドウマップ
StructuredBuffer<float4x4> g_boneMatrix : register(t3); //ボーン行列
sampler g_sampler : register(s0);	

////////////////////////////////////////////////
// 髢｢謨ｰ螳夂ｾｩ縲・
////////////////////////////////////////////////
float3 CalcLambertDiffuse(float3 lightDirection, float3 lightColor, float3 normal);
float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal,float specular);
float3 CalcLigFromDirectionLight(SPSIn psIn, float3 normal, float specular); //繝・ぅ繝ｬ繧ｯ繧ｷ繝ｧ繝ｳ繝ｩ繧､繝医・
float3 CalcLigFromPointLight(SPSIn psIn, int num, float3 normal, float specular);
float3 CalcLigFromSpotLight(SPSIn psIn, int num, float3 normal, float specular);
float3 CalcLimPower(SPSIn psIn);//繝ｪ繝繝ｩ繧､繝医・
float3 CalcLigFromHemiSphereLight(SPSIn psIn);//蜊顔帥繝ｩ繧､繝医・
float3 CalcNormalMap(SPSIn psIn);//豕慕ｷ壹・繝・・縺ｮ險育ｮ励・
float CalcSpeculerMap(SPSIn psIn);//繧ｹ繝壹く繝･繝ｩ繝槭ャ繝励・險育ｮ励・
//float3 CalcAnbientPower(SPSIn psIn);//AO繝槭ャ繝励°繧臥腸蠅・・縺ｮ蠑ｷ縺輔ｒ險育ｮ励・
float CalcShadowMap(SPSIn psIn); 
/// <summary>
//繧ｹ繧ｭ繝ｳ陦悟・繧定ｨ育ｮ励☆繧九・
/// </summary>
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

/// <summary>
///  頂点シェーダーのコア関数。
/// </summary>
SPSIn VSMainCore(SVSIn vsIn, uniform bool hasSkin)
{
	SPSIn psIn;
	float4x4 m;
	if( hasSkin ){
		m = CalcSkinMatrix(vsIn.skinVert);
	}else{
		m = mWorld;
	}
	psIn.pos = mul(m, vsIn.pos);//繝｢繝・Ν縺ｮ鬆らせ繧偵Ρ繝ｼ繝ｫ繝牙ｺｧ讓咏ｳｻ縺ｫ螟画鋤縲・
    psIn.worldPos = psIn.pos;
    float4 worldPos = psIn.pos;
	psIn.pos = mul(mView, psIn.pos);//繝ｯ繝ｼ繝ｫ繝牙ｺｧ讓咏ｳｻ縺九ｉ繧ｫ繝｡繝ｩ蠎ｧ讓咏ｳｻ縺ｫ螟画鋤縲・
	psIn.pos = mul(mProj, psIn.pos);//繧ｫ繝｡繝ｩ蠎ｧ讓咏ｳｻ縺九ｉ繧ｹ繧ｯ繝ｪ繝ｼ繝ｳ蠎ｧ讓咏ｳｻ縺ｫ螟画鋤縲・

    psIn.normal = normalize(mul(m, vsIn.normal));
    psIn.tangent = normalize(mul(m, vsIn.tangent));
    psIn.biNormal = normalize(mul(m, vsIn.biNormal));
	psIn.uv = vsIn.uv;
    
    psIn.normalInView = mul(mView, psIn.normal); //繧ｫ繝｡繝ｩ遨ｺ髢薙・豕慕ｷ壹ｒ豎ゅａ繧九・
    psIn.posInLVP = mul(mLVP, worldPos);

	return psIn;
}

/// <summary>
/// スキンなしメッシュ用の頂点シェーダーのエントリー関数。
/// </summary>
SPSIn VSMain(SVSIn vsIn)
{
	return VSMainCore(vsIn, false);
}
/// <summary>
/// スキンありメッシュの頂点シェーダーのエントリー関数。
/// </summary>
SPSIn VSSkinMain( SVSIn vsIn ) 
{
	return VSMainCore(vsIn, true);
}



/// <summary>
/// ピクセルシェーダーのエントリー関数。
/// </summary>
SPSOut PSMain( SPSIn psIn,int isShadowReceiver ) : SV_Target0
{
    SPSOut shadowColor;
    
    //法線マップの計算
    float3 normalMap = CalcNormalMap(psIn);
  
     //スペキュラマップの計算
    float speculerMap = CalcSpeculerMap(psIn);
    
    //ディレクションライトの計算
    float3 directionLig = CalcLigFromDirectionLight(psIn, normalMap, speculerMap);

     //複数個のライティング計算
    float3 pointLig[10];
    float3 spotLig[10];
  /*  for (int i = 0; i < 10; i++)
    {
        //ポイントライトの計算
        pointLig[i] = CalcLigFromPointLight(psIn, i, normalMap, speculerMap);
        //スポットライトの計算
        spotLig[i] = CalcLigFromSpotLight(psIn, i, normalMap, speculerMap);
    }*/
    
    //リムライトの計算
    float3 limColor = CalcLimPower(psIn);
    //半球ライトの計算
    float3 hemiLight = CalcLigFromHemiSphereLight(psIn);
   
    
	//最終的な光を求める
    float3 finalLig = directionLig + ambientLig+ hemiLight/*+normalMap+speculerMap*/;
   
    /*for (int j = 0; j < 10; j++)
    {
        finalLig += pointLig[j];
        finalLig += spotLig[j];
    }*/
   
    finalLig += limColor;
   
	float4 albedoColor = g_albedo.Sample(g_sampler, psIn.uv);
	
    ///最終出力カラーに光を乗算する
    albedoColor.xyz *= finalLig;
 
    float shadow = 1.0f;
    //影を受け取るなら。
    if (isShadowReceiver == 1)
    {
        shadow = CalcShadowMap(psIn);
    }
    
    //影のカラーを乗算。
    albedoColor.xyz *= shadow;
    
    // ==========================================
    // ★追加: フォグの計算
    // ==========================================
    
    // 1. カメラとピクセルの距離を測る
    // (psIn.worldPos と eyePos を使います)
    float dist = distance(psIn.worldPos, eyePos);

    // 2. フォグの設定 (シーンの広さに合わせて数値をいじってください)
    float fogStart = 100.0f; // 霧がかかり始める距離
    float fogEnd = 400.0f; // 真っ白になる距離
    float3 fogColor = float3(0.8f, 0.8f, 0.9f); // 霧の色（白っぽい青）

    // 3. 霧の濃さを計算 (0.0～1.0)
    float fogFactor = saturate((dist - fogStart) / (fogEnd - fogStart));

    // 4. 元の色と霧の色を混ぜる
    albedoColor.rgb = lerp(albedoColor.rgb, fogColor, fogFactor);

    // ==========================================
    
    shadowColor.color = albedoColor;
       
    return shadowColor;
}

SPSOut PSShadowRecieverMain(SPSIn psIn):SV_Target0
{
    return PSMain(psIn, 1);
}

SPSOut PSNormalMain(SPSIn psIn):SV_Target0
{
    return PSMain(psIn, 0);

}

//拡散反射光
float3 CalcLambertDiffuse(float3 lightDire, float3 lightColor, float3 normal)
{
    float t = dot(normal, lightDire) * -1.0f;

    t = saturate( t);

    return lightColor * t;
}

//鏡面反射光
float3 CalcPhongSpecular(float3 lightDirection, float3 lightColor, float3 worldPos, float3 normal, float specular)
{
    //反射ベクトルを求める
    float3 refVec = reflect(lightDirection, normal);

    //光が当たったサーフェイスから視点に伸びるベクトルを求める
    float3 toEye = eyePos - worldPos;
    toEye = normalize(toEye);

    //鏡面反射の強さを求める
    float t = dot(refVec, toEye);

    //鏡面反射の強さを0以上の数値にする
    t = max(0.0f, t);

    //鏡面反射の強さを絞る
    t = pow(t, 5.0f);

    float3 specLig = lightColor * t;
    
    //鏡面反射光
    return specLig;
}

//ディレクションライト
float3 CalcLigFromDirectionLight(SPSIn psIn, float3 normal, float specular)
{
    //拡散反射光を求める
    float3 diffDirection = CalcLambertDiffuse(directionLight.direction, directionLight.color, normal);

    //スポットライトを求める
    float3 specDirection = CalcPhongSpecular(directionLight.direction, directionLight.color, psIn.worldPos, normal, specular);
    
    //最終的な光
    return diffDirection /*+ specDirection*/;
}
//ポイントライト
float3 CalcLigFromPointLight(SPSIn psIn, int num, float3 normal, float specular)
{
    //このサーフェイスに入射しているポイントライトの光の向きを計算する
    float3 ligDir = psIn.worldPos - pointLight[num].position;
    //正規化
    ligDir = normalize(ligDir);
    
    //減衰なしの拡散反射光を計算する
    float3 diffusePoint = CalcLambertDiffuse(ligDir, pointLight[num].color, normal);
    
    //減衰なしの鏡面反射光を計算する
    float3 specularPoint = CalcPhongSpecular(ligDir, pointLight[num].color, psIn.worldPos, normal, specular);
    
    //ポイントライトとの距離を計算する
    float distance = length(psIn.worldPos - pointLight[num].position);
    
    //影響率は距離に比例して小さくなっていく
    float affect = 1.0f - 1.0f / pointLight[num].range * distance;
    
    //影響力がマイナスにならないように補正をかける
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }
    
    //影響を指数関数的にする
    affect = pow(affect, 3.0f);
    
    //拡散反射光と鏡面反射光に減衰率を乗算して影響を弱める
    diffusePoint *= affect;
    specularPoint *= affect;
    
    //最終的な光
    return diffusePoint + specularPoint;
}

//スポットライト
float3 CalcLigFromSpotLight(SPSIn psIn, int num, float3 normal, float specular)
{
    //サーフェイスに入射するスポットライトの光の向きを計算する
    float3 ligDir = psIn.worldPos - spotLight[num].position;
    //正規化
    ligDir = normalize(ligDir);
    
    //減衰なしの拡散反射光を計算する
    float3 diffuseSpot = CalcLambertDiffuse(ligDir, spotLight[num].color, normal);
    
    //減衰なしの鏡面反射光を計算する
    float3 specularSpot = CalcPhongSpecular(ligDir, spotLight[num].color, psIn.worldPos, normal, specular);
    
    //スポットライトとの距離を計算する
    float distance = length(psIn.worldPos - spotLight[num].position);
    
    //影響率は距離に比例して小さくなっていく
    float affect = 1.0f - 1.0f / spotLight[num].range * distance;
    
    //影響力がマイナスにならないように補正をかける
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }
    
    //影響を指数関数的にする
    affect = pow(affect, 3.0f);
    
    //拡散反射光と鏡面反射光に減衰率を乗算して影響を弱める
    diffuseSpot *= affect;
    specularSpot *= affect;
    
    //入射光と射出方向の角度を求める
    float angle = dot(ligDir, spotLight[num].direction);
    //dot()で求めた値をacos()に渡して角度を求める
    angle = abs(acos(angle));
    
    //角度に比例して小さくなっていく影響率を計算する
    affect = 1.0f - 1.0f / spotLight[num].angle * angle;
    
    //影響力がマイナスにならないように補正をかける
    if (affect < 0.0f)
    {
        affect = 0.0f;
    }
    
    //影響を指数関数的にする
    affect = pow(affect, 0.5f);
    
    //角度による影響率を反射光に乗算して、影響を弱める
    diffuseSpot *= affect;
    specularSpot *= affect;
    
    //最終的な光
    return diffuseSpot + specularSpot;
}
//リムライト
float3 CalcLimPower(SPSIn psIn)
{
    //サーフェイスの法線と光の入射方向に依存するリムの強さを求める
    float power1 = 1.0f - max(0.0f, dot(directionLight.direction, psIn.normal));
    
    //サーフェイスの法線と視線の方向に依存するリムの強さを求める
    float power2 = 1.0f - max(0.0f, psIn.normalInView.z * -1.0f);
    
    //最終的なリムの強さを求める
    float limPower = power1 /** power2*/;
    
    //pow()を使用し強さの変化を指数関数的にする
    limPower = pow(limPower, 1.3f);
    
    //最終的な光
    float3 limColor = limPower * directionLight.color;
    
    return limColor;
}

//半球ライト。
float3 CalcLigFromHemiSphereLight(SPSIn psIn)
{

    //サーフェイスの法線と地面の法線との内積を計算する
    float t = dot(psIn.normal, hemLight.groundNormal);
    
    //内積の結果を0～1の範囲に変換
    t = (t + 1.0f) / 2.0f;
    
    //最終的な光
    float3 hemiLight = lerp(hemLight.groundColor, hemLight.skyColor, t);

    return hemiLight;
}



//法線マップ
float3 CalcNormalMap(SPSIn psIn)
{
    
    float3 normal = psIn.normal;

    //法線マップからタンジェントスペースの法線をサンプリングする
    float3 localNormal = g_normalMap.Sample(g_sampler, psIn.uv).xyz;

    //タンジェントスペースの法線を0～1の範囲から-1～1の範囲に復元する
    localNormal = (localNormal - 0.5f) * 2.0f;

    //タンジェントスペースの法線をワールドスペースに変換する
    normal = psIn.tangent * localNormal.x
            + psIn.biNormal * localNormal.y
            + normal * localNormal.z;
    
    //法線マップ
    return normal;
}

//スペキュラマップ
float CalcSpeculerMap(SPSIn psIn)
{
    //スペキュラマップからスペキュラ反射の強さをサンプリング
    float specPower = g_specularMap.Sample(g_sampler, psIn.uv).r;
    
    //スペキュラマップ
    return specPower;
}



//shadow
float CalcShadowMap(SPSIn psIn)
{
    float2 shadowUV = psIn.posInLVP.xy / psIn.posInLVP.w;
    shadowUV *= float2(0.5f, -0.5f);
    shadowUV += 0.5f;
    
    float shadowMap = 1.0f;
    float zInLVP = psIn.posInLVP.z / psIn.posInLVP.w;
    if(shadowUV.x >= 0.0f && shadowUV.x <= 1.0f && shadowUV.y >= 0.0f && shadowUV.y <= 1.0f)
    {
        //シャドウマップから深度をサンプリング
        float zInShadowMap = g_shadowMap.Sample(g_sampler, shadowUV).r;
        if (zInLVP > zInShadowMap)
        {
            shadowMap = 0.5f; //影が落ちている。
        }
    }
    
    //シャドウマップ
    return shadowMap;
}