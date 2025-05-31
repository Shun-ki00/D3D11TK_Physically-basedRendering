#pragma once
#include <future>
#include "Framework/DebugCamera.h"
#include "Framework/ConstantBuffer.h"

class CommonResources;
class DebugCamera;

class Scene
{
private:

	// PBR定数バッファ
	struct PBRLitConstantBuffer
	{
		DirectX::SimpleMath::Vector4 baseColor; // 基本色
		float matallic;                         // 金属度
		float smoothness;                       // 表面の滑らかさ
		float useBaseMap;                       // ベースカラーテクスチャを使用するか
		float useNormalMap;                     // 法線マップを使用するか
	};

	struct AmbientLightParameters
	{
		DirectX::SimpleMath::Vector3 ambientLightColor;
		float ambientLightIntensity;
	};

public:

	// コンストラクタ
	Scene();
	// デストラクタ
	~Scene() = default;

public:

	// 初期化処理
	void Initialize();
	// 更新処理
	void Update(const float& elapsedTime);
	// 描画処理
	void Render();
	// 終了処理
	void Finalize();

private:

	// シェーダー、バッファの作成
	void CreateShaderAndBuffer();

private:

	// 共有リソース
	CommonResources* m_commonResources;

	// デバッグカメラ
	std::unique_ptr<DebugCamera> m_camera;

	// デバイス
	ID3D11Device1* m_device;
	// コンテキスト
	ID3D11DeviceContext1* m_context;
	// コモンステート
	DirectX::CommonStates* m_commonStates;

	// 定数バッファ
	PBRLitConstantBuffer m_pbrConstantBuffer;

	// モデル
	std::unique_ptr<DirectX::Model> m_model;
	
	// 頂点シェーダー
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	// ピクセルシェーダー
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;

	// 入力レイアウト
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

	// 定数バッファ
	std::unique_ptr<ConstantBuffer<PBRLitConstantBuffer>> m_PBRLitConstantBuffer;
	// 環境ライト
	std::unique_ptr<ConstantBuffer<AmbientLightParameters>> m_ambientLightParameters;


	// ベーステクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_baseTexture;
	// ノーマルマップ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMap;
	// キューブマップ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeMap;
};