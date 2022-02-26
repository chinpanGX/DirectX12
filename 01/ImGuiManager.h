/*------------------------------------------------------------
	
	[ImGuiManager.h]
	Author : 出合翔太

-------------------------------------------------------------*/
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "d3dx12.h"
#include "imgui/imgui.h"

class ImGuiManager
{
public:
	void Init(HWND hwnd, Microsoft::WRL::ComPtr<ID3D12Device> device, DXGI_FORMAT formatRTV, UINT bufferCount, ID3D12DescriptorHeap* heap, 
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle);
	void Uninit();
	void BeginUpdate(ImVec2 window);
	void Update(int& instancingCount, float& Rotation, class ModelColorSet& modelColorSet);
	void Update(class Camera& camera);
	void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void InstancingDataMax(int maxData);
private:
	int m_InstancingDataMax;
	int m_InstancingCount;
	int m_EnableRightRotation = 1; // モデルの回転方向のフラグ
	float m_Speed = 0.01f;
	float m_Color[3];
};

