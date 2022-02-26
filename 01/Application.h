/*------------------------------------------------------------

	[Application.h]
	Author : 出合翔太

-------------------------------------------------------------*/
#pragma once
#include "AppBase.h"
#include <DirectXMath.h>
#include "Camera.h"
#include "ModelColorSet.h"
#include "ImGuiManager.h"

class Application final : public AppBase
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	Application();
	
	void Run()override;
	void Prepare()override;
	void Cleanup()override;

	struct SceneParameter
	{		
		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
	};
private:
	using Buffer = ComPtr<ID3D12Resource1>;

	void BeginRender();
	void Render();
	void EndRender();

	// 初期化
	void SetTeapotModel();
	void SetPipelineState();
	void SetConstantBuffer();

	// 各更新処理
	void UpdateModel();
	void UpdateCamera();

	Buffer CreateBufferResource(D3D12_HEAP_TYPE type, UINT bufferSize, D3D12_RESOURCE_STATES state);

	ComPtr<ID3DBlob> m_VertexShader;
	ComPtr<ID3DBlob> m_PixelShader;
	ComPtr<ID3D12RootSignature> m_RootSignature;
	ComPtr<ID3D12PipelineState> m_Pipeline;

	std::vector<Buffer> m_ConstantBuffers;
	std::vector<Buffer> m_InstanceBuffers;

	struct ModelData
	{
		UINT IndexCount;
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView;
		Buffer ResourceVertexBuffer;
		Buffer ResourceIndexBuffer;
	};

	struct InstanceData
	{
		DirectX::XMFLOAT4X4 World;
		DirectX::XMFLOAT4 Color;
	};

	std::vector<InstanceData> m_InstanceData;

	struct InstanceParameter
	{
		InstanceData Data[500];
	};

	const UINT m_InstanceDataMax = 500;

	std::unique_ptr<Camera> m_Camera;
	std::unique_ptr<ImGuiManager> m_ImGuiManager;
	
	ModelData m_Model;
	std::vector<DirectX::XMFLOAT3> m_ModelPositionList; // モデルの位置
	std::vector<float> m_ModelAngleList;			// モデルの回転
	std::unique_ptr<ModelColorSet> m_ModelColorSet; // モデルのカラー
	
	SceneParameter m_SceneParameter;

	int m_InstancingCount;
	float m_Rotation = 0.01f;
};

