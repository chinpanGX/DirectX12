/*--------------------------------------------------------------
	
	[Swapchain.h]
	Author : 出合翔太

---------------------------------------------------------------*/
#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
#include <memory>
#include "DescriptorManager.h"

class Swapchain final
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	Swapchain() = delete;
	Swapchain(ComPtr<IDXGISwapChain1> swapchain, std::shared_ptr<DescriptorManager>& heapRTV, bool useHDR = false);
	~Swapchain();

	UINT GetCurrentBackBufferIndex() const;
	DescriptorHandle GetCurrentRenderTargetView() const;
	ComPtr<ID3D12Resource1> GetImage(UINT index);	

	// 現在のイメージに対して描画可能バリア設定の取得
	CD3DX12_RESOURCE_BARRIER GetBarrierToRenderTarget();
	// 現在のイメージに対して表示可能バリア設定の取得
	CD3DX12_RESOURCE_BARRIER GetBarrierToPresent();
	
	DXGI_FORMAT GetFormat() const;
	bool GetIsFullScreen() const;
	
	HRESULT Present(UINT syncInterval, UINT flags);	
	// 次のコマンドが積めるようになるまで待機
	void WaitPreviousFrame(ComPtr<ID3D12CommandQueue> commandQueue, int frameIndex, DWORD timeOut);
	void ResizeBuffers(UINT width, UINT height);

	void SetFullScreen(bool toFullScreen);
	void ResizeTarget(const DXGI_MODE_DESC* newTargetParameter);
private:
	// メタデータのセット
	void SetMetaData(); 

	ComPtr<IDXGISwapChain4> m_Swapchain;
	// レンダーターゲット
	std::vector<ComPtr<ID3D12Resource1>> m_Images;
	std::vector<DescriptorHandle> m_ImageRenderTargetView;

	// フェンス
	std::vector<UINT64> m_FenceValues;
	std::vector<ComPtr<ID3D12Fence1>> m_Fences;

	DXGI_SWAP_CHAIN_DESC1 m_Desc;
	HANDLE m_WaitEvent;
};

