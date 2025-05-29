#include "PBRLit.hlsli"

// ノーマルマップ
Texture2D<float4> normalMap : register(t1);
// キューブマップ
TextureCube<float4> cubeMap : register(t2);
// シャドウマップテクスチャ
Texture2D ShadowMapTexture : register(t3);


// PI
static const float F_PI = 3.1415926f;


// 誘電体の反射率（F0）は4%とする
static const float DIELECTRIC = 0.04f;

// Cook-TorranceのD項をGGXで求める
float D_GGX(float ndoth, float alpha)
{
    float a2 = alpha * alpha;
    float d = (ndoth * a2 - ndoth) * ndoth + 1.0f;
    return a2 / (d * d + 0.0000001) * (1.0 / F_PI);
}
float D_GGX(float perceptualRoughness, float ndoth, float3 normalWS, float3 halfDir)
{
    float3 NcrossH = cross(normalWS, halfDir);
    float a = ndoth * perceptualRoughness;
    float k = perceptualRoughness / (dot(NcrossH, NcrossH) + a * a);
    float d = k * k * (1.0f / F_PI);
    return min(d, 65504.0);
}

// Cook-TorranceのV項をHeight-Correlated Smithモデルで求める
float V_SmithGGXCorrelated(float ndotl, float ndotv, float alpha)
{
    float lambdaV = ndotl * (ndotv * (1 - alpha) + alpha);
    float lambdaL = ndotv * (ndotl * (1 - alpha) + alpha);
    return 0.5f / (lambdaV + lambdaL + 0.0001);
}

// Cook-TorranceのF項をSchlickの近似式で求める
float3 F_Schlick(float3 f0, float cos)
{
    return f0 + (1 - f0) * pow(1 - cos, 5);
}


// Disneyのモデルで拡散反射を求める
float Fd_Burley(float ndotv, float ndotl, float ldoth, float roughness)
{
    float fd90 = 0.5 + 2 * ldoth * ldoth * roughness;
    float lightScatter = (1 + (fd90 - 1) * pow(1 - ndotl, 5));
    float viewScatter = (1 + (fd90 - 1) * pow(1 - ndotv, 5));

    float diffuse = lightScatter * viewScatter;
	// diffuse /= F_PI;
    return diffuse;
}

// BRDF式
float4 BRDF(
	float3 albedo,
	float metallic,
	float perceptualRoughness,
	float3 normalWS,
	float3 viewDir,
	float3 lightDir,
	float3 lightColor,
	float3 indirectSpecular
)
{
    float3 halfDir = normalize(lightDir + viewDir);
    float NdotV = abs(dot(normalWS, viewDir));
    float NdotL = max(0, dot(normalWS, lightDir));
    float NdotH = max(0, dot(normalWS, halfDir));
    float LdotH = max(0, dot(lightDir, halfDir));
    float reflectivity = lerp(DIELECTRIC, 1, metallic);
    float3 f0 = lerp(DIELECTRIC, albedo, metallic);

	// 拡散反射
    float diffuseTerm = Fd_Burley(
		NdotV,
		NdotL,
		LdotH,
		perceptualRoughness
	) * NdotL;
    float3 diffuse = albedo * (1 - reflectivity) * lightColor * diffuseTerm;
	// 関節拡散反射
    diffuse += albedo * (1 - reflectivity) * c_ambientLightColor.rgb * f_ambientLightIntensity;
	
	// 鏡面反射
    float alpha = perceptualRoughness * perceptualRoughness;
    float V = V_SmithGGXCorrelated(NdotL, NdotV, alpha);
    float D = D_GGX(NdotH, alpha);
    float3 F = F_Schlick(f0, LdotH);
    float3 specular = V * D * F * NdotL * lightColor;
    specular *= F_PI;
    specular = max(0, specular);
	// 環境反射光
    float surfaceReduction = 1.0 / (alpha * alpha + 1);
    float f90 = saturate((1 - perceptualRoughness) + reflectivity);
    specular += surfaceReduction * indirectSpecular * lerp(f0, f90, pow(1 - NdotV, 5));
	
	
    float3 color = diffuse + specular;
    return float4(color, 1);
}


float4 main(PS_Input input) : SV_TARGET
{
    // ベースカラーを使用する場合サンプリングを行う
    float4 baseColor = lerp(c_baseColor,Texture.Sample(Sampler,input.uv),t_useBaseMap);
    
     // ノーマルマップをサンプリング
    float3 normalMapSample = normalMap.Sample(Sampler, input.uv).rgb;
    
    normalMapSample = normalMapSample * 2.0 - 1.0;
    // TBN行列の作成
    float3x3 TBN = float3x3(input.tangentWS, input.binormalWS, input.normalWS);
    
    // ノーマルマップを使用する場合接線空間の法線をワールド空間に変換
    float3 normalWS = lerp(input.normalWS, normalize(mul(normalMapSample, TBN)), t_useNormalMap);
   
    // 視線ベクトルを計算
    float3 viewDir = normalize(EyePosition - input.positionWS);
    
    // 鏡面反射光の色をサンプリング
    float3 refVec = reflect(viewDir, normalWS);
    refVec.y *= -1;
    float3 indirectSpecular = cubeMap.SampleLevel(Sampler, refVec, f_smoothness * 12).rgb;
    
    // ディレクショナルライトの情報（DirectXTK の標準ライト）
    float3 lightDir = LightDirection[0];      // ライトの方向
    float3 lightColor = LightDiffuseColor[0]; // ライトの色

     // PBRのBRDFを計算
    float4 color = BRDF(
        baseColor.rgb,   // ベースカラー
        f_matallic,      // メタリック
        f_smoothness,    // 粗さ
        normalWS,        // 法線ベクトル
        viewDir,         // 視線ベクトル
        lightDir,        // ライトの方向
        lightColor,      // ライトの色
        indirectSpecular // 環境光による間接鏡面反射
    );
    
    // アルファ値を設定
    color.a = baseColor.a;

    // 最終的なピクセルカラーを返す
    return color;
}