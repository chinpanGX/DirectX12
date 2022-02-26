/*--------------------------------------------------------------
	
	[Swapchain.h]
	Author : �o���đ�

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

	// ���݂̃C���[�W�ɑ΂��ĕ`��\�o���A�ݒ�̎擾
	CD3DX12_RESOURCE_BARRIER GetBarrierToRenderTarget();
	// ���݂̃C���[�W�ɑ΂��ĕ\���\�o���A�ݒ�̎擾
	CD3DX12_RESOURCE_BARRIER GetBarrierToPresent();
	
	DXGI_FORMAT GetFormat() const;
	bool GetIsFullScreen() const;
	
	HRESULT Present(UINT syncInterval, UINT flags);	
	// ���̃R�}���h���ς߂�悤�ɂȂ�܂őҋ@
	void WaitPreviousFrame(ComPtr<ID3D12CommandQueue> commandQueue, int frameIndex, DWORD timeOut);
	void ResizeBuffers(UINT width, UINT height);

	void SetFullScreen(bool toFullScreen);
	void ResizeTarget(const DXGI_MODE_DESC* newTargetParameter);
private:
	// ���^�f�[�^�̃Z�b�g
	void SetMetaData(); 

	ComPtr<IDXGISwapChain4> m_Swapchain;
	// �����_�[�^�[�Q�b�g
	std::vector<ComPtr<ID3D12Resource1>> m_Images;
	std::vector<DescriptorHandle> m_ImageRenderTargetView;

	// �t�F���X
	std::vector<UINT64> m_FenceValues;
	std::vector<ComPtr<ID3D12Fence1>> m_Fences;

	DXGI_SWAP_CHAIN_DESC1 m_Desc;
	HANDLE m_WaitEvent;
};

