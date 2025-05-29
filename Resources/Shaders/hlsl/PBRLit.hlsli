#include "Common.hlsli"


// PBR定数バッファ
cbuffer PBRLitConstantBuffer : register(b2)
{
    float4 c_baseColor   : packoffset(c0);   // 基本色
    float f_matallic     : packoffset(c1.x); // 金属度
    float f_smoothness   : packoffset(c1.y); // 表面の滑らかさ
    float t_useBaseMap   : packoffset(c1.z); // ベースカラーテクスチャを使用するか
    float t_useNormalMap : packoffset(c1.w); // 法線マップを使用するか
}

// 頂点シェーダ入力用
struct VS_Input
{
    float4 positionOS : SV_Position;
    float3 normalOS   : NORMAL;
    float4 tangentOS  : TANGENT;
    float2 uv         : TEXCOORD;
    float4 color      : COLOR;
};

// ピクセルシェーダ入力用
struct PS_Input
{
    float4 positionCS : SV_Position;
    float3 normalWS   : NORMAL;
    float3 tangentWS  : TANGENT;
    float3 binormalWS : TEXCOORD1;
    float4  color     : COLOR;
    float2 uv         : TEXCOORD;
    
    float3 positionWS :TEXCOORD2;
};


