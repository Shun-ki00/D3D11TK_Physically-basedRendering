#include "Common.hlsli"


// PBR�萔�o�b�t�@
cbuffer PBRLitConstantBuffer : register(b2)
{
    float4 c_baseColor   : packoffset(c0);   // ��{�F
    float f_matallic     : packoffset(c1.x); // �����x
    float f_smoothness   : packoffset(c1.y); // �\�ʂ̊��炩��
    float t_useBaseMap   : packoffset(c1.z); // �x�[�X�J���[�e�N�X�`�����g�p���邩
    float t_useNormalMap : packoffset(c1.w); // �@���}�b�v���g�p���邩
}

// ���_�V�F�[�_���͗p
struct VS_Input
{
    float4 positionOS : SV_Position;
    float3 normalOS   : NORMAL;
    float4 tangentOS  : TANGENT;
    float2 uv         : TEXCOORD;
    float4 color      : COLOR;
};

// �s�N�Z���V�F�[�_���͗p
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


