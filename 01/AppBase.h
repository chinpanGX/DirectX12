/*------------------------------------------------------------
	
	[AppBase.h]
	Author : 出合翔太

-------------------------------------------------------------*/
#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#include <wrl.h>
#include <memory>
#include "DescriptorManager.h"
#include "Swapchain.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class AppBase
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	AppBase();
	virtual ~AppBase();
		
	void Init(HWND hwnd, DXGI_FORMAT format, bool isFullScreen);
	void Uninit();

	virtual void Run();
	virtual void Prepare() {}
	virtual void Cleanup() {}

	virtual void OnSizeChanged(UINT width, UINT height, bool isMinimized);
	virtual void OnMouseButtonDown(UINT msg) {}
	virtual void OnMouseButtonUp(UINT msg) {}
	virtual void OnMouseMove(UINT msg, int dx, int dy) {}

	void SetTitle(const std::string& title);
	void ToggleFullScreen();

	// リソース生成
	ComPtr<ID3D12Resource1> CreateResource(const CD3DX12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE* clearValue, D3D12_HEAP_TYPE heapType);
	std::vector<ComPtr<ID3D12Resource1>> CreateConstantBuffers(const CD3DX12_RESOURCE_DESC& desc, int count = m_FrameBufferCount);

	// コマンドバッファ関連
	ComPtr<ID3D12GraphicsCommandList> CreateCommandList();
	void FinishCommandList(ComPtr<ID3D12GraphicsCommandList>& command);
	ComPtr<ID3D12GraphicsCommandList> CreateBundleCommandList();

	void WriteToUploadHeapMemory(ID3D12Resource1* resource, uint32_t size, const void* data);

	ComPtr<ID3D12Device> GetDevice() { return m_Device; }
	std::shared_ptr<Swapchain> GetSwapchain() { return m_Swapchain; }
	std::shared_ptr<DescriptorManager> GetDescriptorManager() { return m_Heap; }
protected:
	void PrepareDescriptorHeaps();
	void CreateDefaultDepthBuffer(int width, int height);
	void CreateCommandAllocators();
	void WaitForIdelGpu();

	ComPtr<ID3D12Device> m_Device;
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	std::shared_ptr<Swapchain> m_Swapchain;
	std::vector<ComPtr<ID3D12Resource1>> m_RenderTargetList;
	ComPtr<ID3D12Resource1> m_DepthBuffers;

	CD3DX12_VIEWPORT m_Viewport;
	CD3DX12_RECT m_ScissorRect;
	DXGI_FORMAT m_SurfaceFormat;

	std::vector<ComPtr<ID3D12CommandAllocator>> m_CommandAllocators;
	ComPtr<ID3D12CommandAllocator> m_OneShotCommandAllocator;
	ComPtr<ID3D12CommandAllocator> m_BundleCommandAloocator;

	std::shared_ptr<DescriptorManager> m_HeapRenderTargetView;
	std::shared_ptr<DescriptorManager>m_HeapDepthStencilView;
	std::shared_ptr<DescriptorManager>m_Heap;

	DescriptorHandle m_DefaultDepthStencilView;
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;

	HANDLE m_WaitFence;
	UINT m_FrameIndex;
	UINT m_Width;
	UINT m_Height;
	bool m_IsAllowTearing;
	HWND m_hwnd;

	const UINT m_GpuWaitTimeout = (10 * 1000);
	static const UINT m_FrameBufferCount = 2;
};

HRESULT CompileShaderFromFile(const std::wstring& fileName, const std::wstring& profile, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob);
