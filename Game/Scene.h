#pragma once
#include <future>
#include "Framework/DebugCamera.h"
#include "Framework/ConstantBuffer.h"

class CommonResources;
class DebugCamera;

class Scene
{
private:

	// PBR�萔�o�b�t�@
	struct PBRLitConstantBuffer
	{
		DirectX::SimpleMath::Vector4 baseColor; // ��{�F
		float matallic;                         // �����x
		float smoothness;                       // �\�ʂ̊��炩��
		float useBaseMap;                       // �x�[�X�J���[�e�N�X�`�����g�p���邩
		float useNormalMap;                     // �@���}�b�v���g�p���邩
	};

	struct AmbientLightParameters
	{
		DirectX::SimpleMath::Vector3 ambientLightColor;
		float ambientLightIntensity;
	};

public:

	// �R���X�g���N�^
	Scene();
	// �f�X�g���N�^
	~Scene() = default;

public:

	// ����������
	void Initialize();
	// �X�V����
	void Update(const float& elapsedTime);
	// �`�揈��
	void Render();
	// �I������
	void Finalize();

private:

	// �V�F�[�_�[�A�o�b�t�@�̍쐬
	void CreateShaderAndBuffer();

private:

	// ���L���\�[�X
	CommonResources* m_commonResources;

	// �f�o�b�O�J����
	std::unique_ptr<DebugCamera> m_camera;

	// �f�o�C�X
	ID3D11Device1* m_device;
	// �R���e�L�X�g
	ID3D11DeviceContext1* m_context;
	// �R�����X�e�[�g
	DirectX::CommonStates* m_commonStates;

	// �萔�o�b�t�@
	PBRLitConstantBuffer m_pbrConstantBuffer;

	// ���f��
	std::unique_ptr<DirectX::Model> m_model;
	
	// ���_�V�F�[�_�[
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	// �s�N�Z���V�F�[�_�[
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

	// ���̓��C�A�E�g
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

	// �萔�o�b�t�@
	std::unique_ptr<ConstantBuffer<PBRLitConstantBuffer>> m_PBRLitConstantBuffer;
	// �����C�g
	std::unique_ptr<ConstantBuffer<AmbientLightParameters>> m_ambientLightParameters;


	// �x�[�X�e�N�X�`��
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_baseTexture;
	// �m�[�}���}�b�v
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMap;
	// �L���[�u�}�b�v
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeMap;
};