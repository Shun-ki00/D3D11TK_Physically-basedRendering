#include "pch.h"
#include "Game/Scene.h"
#include "Framework/CommonResources.h"
#include "Framework/DebugCamera.h"
#include "imgui/ImGuizmo.h"
#include "Framework/Microsoft/ReadData.h"
#include "WICTextureLoader.h"
#include "DDSTextureLoader.h"
#include "Framework/ConstantBuffer.h"
#include <Model.h>


/// <summary>
/// �R���X�g���N�^
/// </summary>
Scene::Scene()
{
	// �C���X�^���X���擾����
	m_commonResources = CommonResources::GetInstance();
	m_device          = CommonResources::GetInstance()->GetDeviceResources()->GetD3DDevice();
	m_context         = CommonResources::GetInstance()->GetDeviceResources()->GetD3DDeviceContext();
	m_commonStates    = CommonResources::GetInstance()->GetCommonStates();
}

/// <summary>
/// ����������
/// </summary>
void Scene::Initialize()
{
	// �J�����̍쐬
	m_camera = std::make_unique<DebugCamera>();
	m_camera->Initialize(1280, 720);

	// ����������
	PBRLitConstantBuffer pbr{
		DirectX::SimpleMath::Vector4::One,
		1.0f,
		0.0f,
		1.0f,
		1.0f
	};

	m_pbrConstantBuffer = pbr;

	AmbientLightParameters ambient{
		DirectX::SimpleMath::Vector3::One,
		1.0f
	};

	// �萔�o�b�t�@�̍쐬
	m_PBRLitConstantBuffer = std::make_unique<ConstantBuffer<PBRLitConstantBuffer>>();
	m_PBRLitConstantBuffer->Initialize(m_device);
	m_PBRLitConstantBuffer->Update(m_context,pbr);
	m_ambientLightParameters = std::make_unique<ConstantBuffer<AmbientLightParameters>>();
	m_ambientLightParameters->Initialize(m_device);
	m_ambientLightParameters->Update(m_context, ambient);


	// ���f�������[�h����@============================================

	// �G�t�F�N�g�t�@�N�g���𐶐�����
	std::unique_ptr<DirectX::EffectFactory> effectFactory = std::make_unique<DirectX::EffectFactory>(m_device);
	// �f�B���N�g����ݒ肷��
	effectFactory->SetDirectory(L"Resources/Model");
	// ���f���̃��[�h
	m_model = DirectX::Model::CreateFromCMO(m_device, L"Resources/Model/torus.cmo", *effectFactory);

	// ���f���̃G�t�F�N�g�����X�V����
	m_model->UpdateEffects([](DirectX::IEffect* effect) {
		// �x�[�V�b�N�G�t�F�N�g��ݒ肷��
		DirectX::BasicEffect* basicEffect = dynamic_cast<DirectX::BasicEffect*>(effect);
		if (basicEffect)
		{
			// �g�U���ˌ�
			DirectX::SimpleMath::Color diffuseColor = DirectX::SimpleMath::Color(1.0f, 0.95f, 0.9f);
			// ���C�g���Ƃ炷����
			DirectX::SimpleMath::Vector3 lightDirection(0.0f, 1.0f, 0.0f);

			basicEffect->SetLightEnabled(1, false);
			basicEffect->SetLightEnabled(2, false);

			// �[���Ԃ̃��C�g�Ɋg�U���ˌ���ݒ肷��
			basicEffect->SetLightDiffuseColor(0, diffuseColor);
			// �[���Ԃ̃��C�g���Ƃ炷������ݒ肷��
			basicEffect->SetLightDirection(0, lightDirection);
		}
		});

	
	// �e�N�X�`�������[�h���� ===========================================

	// �x�[�X�e�N�X�`��
	DirectX::CreateWICTextureFromFile(
	m_device, L"Resources/Textures/wood.png", nullptr, m_baseTexture.ReleaseAndGetAddressOf());
	// �m�[�}���}�b�v
	DirectX::CreateWICTextureFromFile(
		m_device, L"Resources/Textures/woodNormal.png", nullptr, m_normalMap.ReleaseAndGetAddressOf());
	// �L���[�u�}�b�v
	DirectX::CreateDDSTextureFromFile(
		m_device, L"Resources/Textures/DirectXTKCubeMap.dds", nullptr, m_cubeMap.ReleaseAndGetAddressOf());


	// �V�F�[�_�[�A�o�b�t�@�̍쐬
	this->CreateShaderAndBuffer();
	// �u�����h�X�e�[�g�̍쐬
	this->CreateBlendState();
	// �[�x�X�e���V���X�e�[�g�̍쐬
	this->CreateDepthStencilState();
	// ���X�^���C�U�[�X�e�[�g�̍쐬
	this->CreateRasterizerState();
}

/// <summary>
/// �X�V����
/// </summary>
/// <param name="elapsedTime">�o�ߎ���</param>
void Scene::Update(const float& elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	m_camera->Update();
	m_commonResources->SetViewMatrix(m_camera->GetViewMatrix());

}

/// <summary>
/// �`�揈��
/// </summary>
void Scene::Render()
{
	static bool chackBoxB = false;
	static bool chackBoxN = false;

	// ImGui �E�B���h�E���Ɉȉ���ǉ�
	if (ImGui::Begin("PBR Material Editor")) {
		// Base Color�iVector4: RGBA�j
		ImGui::ColorEdit4("Base Color", reinterpret_cast<float*>(&m_pbrConstantBuffer.baseColor));

		// Metallic�i0.0 �` 1.0�j
		ImGui::SliderFloat("Metallic", &m_pbrConstantBuffer.matallic, 0.0f, 1.0f);

		// Smoothness�i0.0 �` 1.0�j
		ImGui::SliderFloat("Smoothness", &m_pbrConstantBuffer.smoothness, 0.0f, 1.0f);

		// Use Base Map�i0 or 1�j
		ImGui::Checkbox("Use Base Map", &chackBoxB);

		// Use Normal Map�i0 or 1�j
		ImGui::Checkbox("Use Normal Map", &chackBoxN);
	}
	ImGui::End();

	m_pbrConstantBuffer.useBaseMap = static_cast<int>(chackBoxB);
	m_pbrConstantBuffer.useNormalMap = static_cast<int>(chackBoxN);


	m_PBRLitConstantBuffer->Update(m_context, m_pbrConstantBuffer);


	DirectX::SimpleMath::Matrix world = DirectX::SimpleMath::Matrix::CreateScale(2.0f);
	DirectX::SimpleMath::Matrix view = m_camera->GetViewMatrix();
	DirectX::SimpleMath::Matrix proj = m_commonResources->GetProjectionMatrix();

	// ���f���`��
	m_model->Draw(m_context, *m_commonStates, world, view, proj, false, [&]
		{
			// �萔�o�b�t�@���w�肷��
			ID3D11Buffer* cbuf[] = { m_ambientLightParameters->GetBuffer() };
			m_context->VSSetConstantBuffers(1, 1, cbuf);
			m_context->PSSetConstantBuffers(1, 1, cbuf);

			// �u�����h�X�e�[�g��ݒ� (�������`��p)
			m_context->OMSetBlendState(m_commonStates->AlphaBlend(), nullptr, 0xFFFFFFFF);

			m_context->IASetInputLayout(m_inputLayout.Get());

			// �萔�o�b�t�@���w�肷��
			cbuf[0] = { m_PBRLitConstantBuffer->GetBuffer() };
			m_context->VSSetConstantBuffers(2, 1, cbuf);
			m_context->PSSetConstantBuffers(2, 1, cbuf);

			// �e�N�X�`���̐ݒ�
			std::vector<ID3D11ShaderResourceView*> tex = {
				m_baseTexture.Get(),
				m_normalMap.Get(),
				m_cubeMap.Get(),
			};

			m_context->VSSetShaderResources(0, (UINT)tex.size(), tex.data());
			m_context->PSSetShaderResources(0, (UINT)tex.size(), tex.data());

			// �V�F�[�_�[��ݒ�
			m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
			m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			// �T���v���[�X�e�[�g���w�肷��
			ID3D11SamplerState* sampler[] = { m_commonStates->LinearWrap() };
			m_context->VSSetSamplers(0, 1, sampler);
			m_context->PSSetSamplers(0, 1, sampler);
		});

	// �V�F�[�_�̉��
	m_context->VSSetShader(nullptr, nullptr, 0);
	m_context->PSSetShader(nullptr, nullptr, 0);
	// �e�N�X�`�����\�[�X�̉��
	ID3D11ShaderResourceView* nullsrv[] = { nullptr };
	m_context->PSSetShaderResources(0, 1, nullsrv);
	m_context->PSSetShaderResources(1, 1, nullsrv);
	m_context->PSSetShaderResources(2, 1, nullsrv);
}

/// <summary>
/// �I������
/// </summary>
void Scene::Finalize() {}


/// <summary>
/// �V�F�[�_�[�A�o�b�t�@�̍쐬
/// </summary>
void Scene::CreateShaderAndBuffer()
{
	// �V�F�[�_��ǂݍ��ނ��߂�blob
	std::vector<uint8_t> blob;

	// ���_�V�F�[�_�����[�h����
	blob = DX::ReadData(L"Resources/Shaders/cso/PBRLit_VS.cso");
	DX::ThrowIfFailed(
		m_device->CreateVertexShader(blob.data(), blob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf())
	);

	//	�C���v�b�g���C�A�E�g�̍쐬
	m_device->CreateInputLayout(
		DirectX::VertexPositionNormalTangentColorTexture::InputElements,
		DirectX::VertexPositionNormalTangentColorTexture::InputElementCount,
		blob.data(), blob.size(),
		m_inputLayout.GetAddressOf());


	// �s�N�Z���V�F�[�_�����[�h����
	blob = DX::ReadData(L"Resources/Shaders/cso/PBRLit_PS.cso");
	DX::ThrowIfFailed(
		m_device->CreatePixelShader(blob.data(), blob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf())
	);

}


/// <summary>
/// �u�����h�X�e�[�g�̍쐬
/// </summary>
void Scene::CreateBlendState()
{
	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;  // �J�o���b�W���A���t�@�Ɋ�Â��ėL��������
	blendDesc.IndependentBlendEnable = FALSE; // �����̃����_�[�^�[�Q�b�g��Ɨ����Đݒ肷��

	D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = TRUE;              // �u�����h��L����
	rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;        // �\�[�X�̃A���t�@
	rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;    // �t�A���t�@
	rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;           // ���Z�u�����h
	rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;              // �A���t�@�l�̃\�[�X
	rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;             // �A���t�@�l�̃f�X�e�B�l�[�V����
	rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;           // �A���t�@�l�̉��Z
	rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL; // RGBA�S�Ă�L��

	blendDesc.RenderTarget[0] = rtBlendDesc;

	// �u�����h�X�e�[�g���쐬
	m_device->CreateBlendState(&blendDesc, &m_blendState);
}

/// <summary>
/// �[�x�X�e���V���X�e�[�g�̍쐬
/// </summary>
void Scene::CreateDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                          // �[�x�e�X�g��L����
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // �[�x�o�b�t�@�̏������݂�L����
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;      // �[�x�e�X�g���� (�������ꍇ�̂ݕ`��)
	depthStencilDesc.StencilEnable = FALSE;                       // �X�e���V���e�X�g�𖳌���

	// �[�x�X�e���V���X�e�[�g���쐬
	m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
}

/// <summary>
/// ���X�^���C�U�[�X�e�[�g�̍쐬
/// </summary>
void Scene::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;      // �h��Ԃ� (�܂��� D3D11_FILL_WIREFRAME)D3D11_FILL_SOLID
	rasterDesc.CullMode = D3D11_CULL_NONE; // �J�����O�Ȃ� (�܂��� D3D11_CULL_FRONT / D3D11_CULL_BACK)
	rasterDesc.FrontCounterClockwise = FALSE;    // ���v���̒��_������\�ʂƂ��ĔF��
	rasterDesc.DepthClipEnable = TRUE;           // �[�x�N���b�s���O��L����

	// ���X�^���C�U�[�X�e�[�g�̍쐬
	m_device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);
}