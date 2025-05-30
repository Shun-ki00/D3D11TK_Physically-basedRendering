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
/// コンストラクタ
/// </summary>
Scene::Scene()
{
	// インスタンスを取得する
	m_commonResources = CommonResources::GetInstance();
	m_device          = CommonResources::GetInstance()->GetDeviceResources()->GetD3DDevice();
	m_context         = CommonResources::GetInstance()->GetDeviceResources()->GetD3DDeviceContext();
	m_commonStates    = CommonResources::GetInstance()->GetCommonStates();
}

/// <summary>
/// 初期化処理
/// </summary>
void Scene::Initialize()
{
	// カメラの作成
	m_camera = std::make_unique<DebugCamera>();
	m_camera->Initialize(1280, 720);

	// 初期化処理
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

	// 定数バッファの作成
	m_PBRLitConstantBuffer = std::make_unique<ConstantBuffer<PBRLitConstantBuffer>>();
	m_PBRLitConstantBuffer->Initialize(m_device);
	m_PBRLitConstantBuffer->Update(m_context,pbr);
	m_ambientLightParameters = std::make_unique<ConstantBuffer<AmbientLightParameters>>();
	m_ambientLightParameters->Initialize(m_device);
	m_ambientLightParameters->Update(m_context, ambient);


	// モデルをロードする　============================================

	// エフェクトファクトリを生成する
	std::unique_ptr<DirectX::EffectFactory> effectFactory = std::make_unique<DirectX::EffectFactory>(m_device);
	// ディレクトリを設定する
	effectFactory->SetDirectory(L"Resources/Model");
	// モデルのロード
	m_model = DirectX::Model::CreateFromCMO(m_device, L"Resources/Model/torus.cmo", *effectFactory);

	// モデルのエフェクト情報を更新する
	m_model->UpdateEffects([](DirectX::IEffect* effect) {
		// ベーシックエフェクトを設定する
		DirectX::BasicEffect* basicEffect = dynamic_cast<DirectX::BasicEffect*>(effect);
		if (basicEffect)
		{
			// 拡散反射光
			DirectX::SimpleMath::Color diffuseColor = DirectX::SimpleMath::Color(1.0f, 0.95f, 0.9f);
			// ライトが照らす方向
			DirectX::SimpleMath::Vector3 lightDirection(0.0f, 1.0f, 0.0f);

			basicEffect->SetLightEnabled(1, false);
			basicEffect->SetLightEnabled(2, false);

			// ゼロ番のライトに拡散反射光を設定する
			basicEffect->SetLightDiffuseColor(0, diffuseColor);
			// ゼロ番のライトが照らす方向を設定する
			basicEffect->SetLightDirection(0, lightDirection);
		}
		});

	
	// テクスチャをロードする ===========================================

	// ベーステクスチャ
	DirectX::CreateWICTextureFromFile(
	m_device, L"Resources/Textures/wood.png", nullptr, m_baseTexture.ReleaseAndGetAddressOf());
	// ノーマルマップ
	DirectX::CreateWICTextureFromFile(
		m_device, L"Resources/Textures/woodNormal.png", nullptr, m_normalMap.ReleaseAndGetAddressOf());
	// キューブマップ
	DirectX::CreateDDSTextureFromFile(
		m_device, L"Resources/Textures/cubemap.dds", nullptr, m_cubeMap.ReleaseAndGetAddressOf());


	// シェーダー、バッファの作成
	this->CreateShaderAndBuffer();
}

/// <summary>
/// 更新処理
/// </summary>
/// <param name="elapsedTime">経過時間</param>
void Scene::Update(const float& elapsedTime)
{
	UNREFERENCED_PARAMETER(elapsedTime);

	// カメラの更新処理
	m_camera->Update();
	m_commonResources->SetViewMatrix(m_camera->GetViewMatrix());

}

/// <summary>
/// 描画処理
/// </summary>
void Scene::Render()
{
	static bool chackBoxB = true;
	static bool chackBoxN = true;

	// ImGui ウィンドウ内に以下を追加
	if (ImGui::Begin("PBR Material Editor")) {
		// Base Color（Vector4: RGBA）
		ImGui::ColorEdit4("Base Color", reinterpret_cast<float*>(&m_pbrConstantBuffer.baseColor));

		// Metallic（0.0 〜 1.0）
		ImGui::SliderFloat("Metallic", &m_pbrConstantBuffer.matallic, 0.0f, 1.0f);

		// Smoothness（0.0 〜 1.0）
		ImGui::SliderFloat("Smoothness", &m_pbrConstantBuffer.smoothness, 0.0f, 1.0f);

		// Use Base Map（0 or 1）
		ImGui::Checkbox("Use Base Map", &chackBoxB);

		// Use Normal Map（0 or 1）
		ImGui::Checkbox("Use Normal Map", &chackBoxN);
	}
	ImGui::End();

	m_pbrConstantBuffer.useBaseMap   = static_cast<float>(chackBoxB);
	m_pbrConstantBuffer.useNormalMap = static_cast<float>(chackBoxN);

	// 定数バッファの更新
	m_PBRLitConstantBuffer->Update(m_context, m_pbrConstantBuffer);


	DirectX::SimpleMath::Matrix world = DirectX::SimpleMath::Matrix::CreateScale(2.0f);
	DirectX::SimpleMath::Matrix view = m_camera->GetViewMatrix();
	DirectX::SimpleMath::Matrix proj = m_commonResources->GetProjectionMatrix();

	// モデル描画
	m_model->Draw(m_context, *m_commonStates, world, view, proj, false, [&]
		{
			// 定数バッファを指定する
			ID3D11Buffer* cbuf[] = { m_ambientLightParameters->GetBuffer() };
			m_context->VSSetConstantBuffers(1, 1, cbuf);
			m_context->PSSetConstantBuffers(1, 1, cbuf);

			// ブレンドステートを設定 (半透明描画用)
			m_context->OMSetBlendState(m_commonStates->AlphaBlend(), nullptr, 0xFFFFFFFF);

			m_context->IASetInputLayout(m_inputLayout.Get());

			// 定数バッファを指定する
			cbuf[0] = { m_PBRLitConstantBuffer->GetBuffer() };
			m_context->VSSetConstantBuffers(2, 1, cbuf);
			m_context->PSSetConstantBuffers(2, 1, cbuf);

			// テクスチャの設定
			std::vector<ID3D11ShaderResourceView*> tex = {
				m_baseTexture.Get(),
				m_normalMap.Get(),
				m_cubeMap.Get(),
			};

			m_context->VSSetShaderResources(0, (UINT)tex.size(), tex.data());
			m_context->PSSetShaderResources(0, (UINT)tex.size(), tex.data());

			// シェーダーを設定
			m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
			m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			// サンプラーステートを指定する
			ID3D11SamplerState* sampler[] = { m_commonStates->LinearWrap() };
			m_context->VSSetSamplers(0, 1, sampler);
			m_context->PSSetSamplers(0, 1, sampler);
		});

	// シェーダの解放
	m_context->VSSetShader(nullptr, nullptr, 0);
	m_context->PSSetShader(nullptr, nullptr, 0);
	// テクスチャリソースの解放
	ID3D11ShaderResourceView* nullsrv[] = { nullptr };
	m_context->PSSetShaderResources(0, 1, nullsrv);
	m_context->PSSetShaderResources(1, 1, nullsrv);
	m_context->PSSetShaderResources(2, 1, nullsrv);
}

/// <summary>
/// 終了処理
/// </summary>
void Scene::Finalize() {}


/// <summary>
/// シェーダー、バッファの作成
/// </summary>
void Scene::CreateShaderAndBuffer()
{
	// シェーダを読み込むためのblob
	std::vector<uint8_t> blob;

	// 頂点シェーダをロードする
	blob = DX::ReadData(L"Resources/Shaders/cso/PBRLit_VS.cso");
	DX::ThrowIfFailed(
		m_device->CreateVertexShader(blob.data(), blob.size(), nullptr, m_vertexShader.ReleaseAndGetAddressOf())
	);

	//	インプットレイアウトの作成
	m_device->CreateInputLayout(
		DirectX::VertexPositionNormalTangentColorTexture::InputElements,
		DirectX::VertexPositionNormalTangentColorTexture::InputElementCount,
		blob.data(), blob.size(),
		m_inputLayout.GetAddressOf());


	// ピクセルシェーダをロードする
	blob = DX::ReadData(L"Resources/Shaders/cso/PBRLit_PS.cso");
	DX::ThrowIfFailed(
		m_device->CreatePixelShader(blob.data(), blob.size(), nullptr, m_pixelShader.ReleaseAndGetAddressOf())
	);

}
