#include "PBRLit.hlsli"

// �m�[�}���}�b�v
Texture2D<float4> normalMap : register(t1);
// �L���[�u�}�b�v
TextureCube<float4> cubeMap : register(t2);
// �V���h�E�}�b�v�e�N�X�`��
Texture2D ShadowMapTexture : register(t3);


// PI
static const float F_PI = 3.1415926f;


// �U�d�̂̔��˗��iF0�j��4%�Ƃ���
static const float DIELECTRIC = 0.04f;

// Cook-Torrance��D����GGX�ŋ��߂�
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

// Cook-Torrance��V����Height-Correlated Smith���f���ŋ��߂�
float V_SmithGGXCorrelated(float ndotl, float ndotv, float alpha)
{
    float lambdaV = ndotl * (ndotv * (1 - alpha) + alpha);
    float lambdaL = ndotv * (ndotl * (1 - alpha) + alpha);
    return 0.5f / (lambdaV + lambdaL + 0.0001);
}

// Cook-Torrance��F����Schlick�̋ߎ����ŋ��߂�
float3 F_Schlick(float3 f0, float cos)
{
    return f0 + (1 - f0) * pow(1 - cos, 5);
}


// Disney�̃��f���Ŋg�U���˂����߂�
float Fd_Burley(float ndotv, float ndotl, float ldoth, float roughness)
{
    float fd90 = 0.5 + 2 * ldoth * ldoth * roughness;
    float lightScatter = (1 + (fd90 - 1) * pow(1 - ndotl, 5));
    float viewScatter = (1 + (fd90 - 1) * pow(1 - ndotv, 5));

    float diffuse = lightScatter * viewScatter;
	// diffuse /= F_PI;
    return diffuse;
}

// BRDF��
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

	// �g�U����
    float diffuseTerm = Fd_Burley(
		NdotV,
		NdotL,
		LdotH,
		perceptualRoughness
	) * NdotL;
    float3 diffuse = albedo * (1 - reflectivity) * lightColor * diffuseTerm;
	// �֐ߊg�U����
    diffuse += albedo * (1 - reflectivity) * c_ambientLightColor.rgb * f_ambientLightIntensity;
	
	// ���ʔ���
    float alpha = perceptualRoughness * perceptualRoughness;
    float V = V_SmithGGXCorrelated(NdotL, NdotV, alpha);
    float D = D_GGX(NdotH, alpha);
    float3 F = F_Schlick(f0, LdotH);
    float3 specular = V * D * F * NdotL * lightColor;
    specular *= F_PI;
    specular = max(0, specular);
	// �����ˌ�
    float surfaceReduction = 1.0 / (alpha * alpha + 1);
    float f90 = saturate((1 - perceptualRoughness) + reflectivity);
    specular += surfaceReduction * indirectSpecular * lerp(f0, f90, pow(1 - NdotV, 5));
	
	
    float3 color = diffuse + specular;
    return float4(color, 1);
}


float4 main(PS_Input input) : SV_TARGET
{
    // �x�[�X�J���[���g�p����ꍇ�T���v�����O���s��
    float4 baseColor = lerp(c_baseColor,Texture.Sample(Sampler,input.uv),t_useBaseMap);
    
     // �m�[�}���}�b�v���T���v�����O
    float3 normalMapSample = normalMap.Sample(Sampler, input.uv).rgb;
    
    normalMapSample = normalMapSample * 2.0 - 1.0;
    // TBN�s��̍쐬
    float3x3 TBN = float3x3(input.tangentWS, input.binormalWS, input.normalWS);
    
    // �m�[�}���}�b�v���g�p����ꍇ�ڐ���Ԃ̖@�������[���h��Ԃɕϊ�
    float3 normalWS = lerp(input.normalWS, normalize(mul(normalMapSample, TBN)), t_useNormalMap);
   
    // �����x�N�g�����v�Z
    float3 viewDir = normalize(EyePosition - input.positionWS);
    
    // ���ʔ��ˌ��̐F���T���v�����O
    float3 refVec = reflect(viewDir, normalWS);
    refVec.y *= -1;
    float3 indirectSpecular = cubeMap.SampleLevel(Sampler, refVec, f_smoothness * 12).rgb;
    
    // �f�B���N�V���i�����C�g�̏��iDirectXTK �̕W�����C�g�j
    float3 lightDir = LightDirection[0];      // ���C�g�̕���
    float3 lightColor = LightDiffuseColor[0]; // ���C�g�̐F

     // PBR��BRDF���v�Z
    float4 color = BRDF(
        baseColor.rgb,   // �x�[�X�J���[
        f_matallic,      // ���^���b�N
        f_smoothness,    // �e��
        normalWS,        // �@���x�N�g��
        viewDir,         // �����x�N�g��
        lightDir,        // ���C�g�̕���
        lightColor,      // ���C�g�̐F
        indirectSpecular // �����ɂ��Ԑڋ��ʔ���
    );
    
    // �A���t�@�l��ݒ�
    color.a = baseColor.a;

    // �ŏI�I�ȃs�N�Z���J���[��Ԃ�
    return color;
}