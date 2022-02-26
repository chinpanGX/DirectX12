/*--------------------------------------------------------------
	
	[Swapchian.cpp]
	Author : �o���đ�

---------------------------------------------------------------*/
#include "Swapchain.h"
#include "Util.h"

Swapchain::Swapchain(ComPtr<IDXGISwapChain1> swapchain, std::shared_ptr<DescriptorManager>& heapRTV, bool useHDR)
{
	swapchain.As(&m_Swapchain); // IDXGISwapChain4���擾
	m_Swapchain->GetDesc1(&m_Desc);

	ComPtr<ID3D12Device> device;
	swapchain->GetDevice(IID_PPV_ARGS(&device));

	// BackBuffer���̗̈���m�ۂ���
	m_Images.resize(m_Desc.BufferCount);
	m_ImageRenderTargetView.resize(m_Desc.BufferCount);
	m_FenceValues.resize(m_Desc.BufferCount);
	m_Fences.resize(m_Desc.BufferCount);

	m_WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	HRESULT hr;
	for (UINT i = 0; i < m_Desc.BufferCount; i++)
	{
		// �t�F���X�̍쐬
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fences[i]));
		THROW_IF_FAILED(hr, "CreateFence ���s");

		m_ImageRenderTargetView[i] = heapRTV->Alloc();

		// �����_�[�^�[�Q�b�g�̐���
		hr = m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Images[i]));
		THROW_IF_FAILED(hr, "GetBuffer ���s");		
		device->CreateRenderTargetView(m_Images[i].Get(), nullptr, m_ImageRenderTargetView[i]);
	}

	// �t�H�[�}�b�g�ɉ����ăJ���[�X�y�[�X��ݒ�
	DXGI_COLOR_SPACE_TYPE colorSpace;
	switch (m_Desc.Format)
	{
		// �f�t�H���g
	default:
		colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		break;
		// �A���t�@���܂ރ`���l��������16�r�b�g���T�|�[�g����4�R���|�[�l���g��64�r�b�g���������_�`��
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
		break;
		// �A���t�@���܂ރ`���l��������16�r�b�g���T�|�[�g����4�R���|�[�l���g��64�r�b�gunsigned - normalized - integer�`��
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		colorSpace = DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020;
		break;
	}
	m_Swapchain->SetColorSpace1(colorSpace);

	// HDR�f�B�X�v���C���g��
	if (useHDR)
	{
		SetMetaData();
	}
}

Swapchain::~Swapchain()
{
	BOOL isFullScreen;
	m_Swapchain->GetFullscreenState(&isFullScreen, nullptr);
	if (isFullScreen)
	{
		m_Swapchain->SetFullscreenState(FALSE, nullptr);
	}
	CloseHandle(m_WaitEvent);
}

UINT Swapchain::GetCurrentBackBufferIndex() const
{
	return m_Swapchain->GetCurrentBackBufferIndex();
}

DescriptorHandle Swapchain::GetCurrentRenderTargetView() const
{
	return m_ImageRenderTargetView[GetCurrentBackBufferIndex()];
}

Microsoft::WRL::ComPtr<ID3D12Resource1> Swapchain::GetImage(UINT index)
{
	return m_Images[index];
}

CD3DX12_RESOURCE_BARRIER Swapchain::GetBarrierToRenderTarget()
{
	// �`��\�o���A�ݒ�̎擾
	auto renderResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Images[GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	return renderResourceBarrier;
}

CD3DX12_RESOURCE_BARRIER Swapchain::GetBarrierToPresent()
{
	//�\���\�o���A�ݒ�̎擾
	auto presentResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Images[GetCurrentBackBufferIndex()].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	return presentResourceBarrier;
}

DXGI_FORMAT Swapchain::GetFormat() const
{
	return m_Desc.Format;
}

bool Swapchain::GetIsFullScreen() const
{
	BOOL fullScreen = TRUE;
	if (FAILED(m_Swapchain->GetFullscreenState(&fullScreen, nullptr)))
	{
		fullScreen = FALSE;
	}
	return fullScreen;
}

HRESULT Swapchain::Present(UINT syncInterval, UINT flags)
{
	return m_Swapchain->Present(syncInterval, flags);
}

void Swapchain::WaitPreviousFrame(ComPtr<ID3D12CommandQueue> commandQueue, int frameIndex, DWORD timeOut)
{
	auto fence = m_Fences[frameIndex];
	// ���݂̃t�F���X��GPU�����B��ݒ肳���l���Z�b�g
	UINT64 value = m_FenceValues[frameIndex]++;
	commandQueue->Signal(fence.Get(), value);

	// ���t���[���ŏ�������R�}���h�̎��s������ҋ@����
	UINT nextIndex = GetCurrentBackBufferIndex();
	UINT64 finishValue = m_FenceValues[nextIndex];
	fence = m_Fences[nextIndex];
	value = fence->GetCompletedValue();
	if (value < finishValue)
	{
		// �������̂��߃C�x���g�őҋ@
		fence->SetEventOnCompletion(finishValue, m_WaitEvent);
		WaitForSingleObject(m_WaitEvent, timeOut);
	}
}

void Swapchain::ResizeBuffers(UINT width, UINT height)
{
	// ���T�C�Y����̂ŁA��U���
	for (auto& v : m_Images)
	{
		v.Reset();
	}
	HRESULT hr = m_Swapchain->ResizeBuffers(m_Desc.BufferCount, width, height, m_Desc.Format, m_Desc.Flags);
	THROW_IF_FAILED(hr, "ResizeBuffers ���s");

	// �C���[�W����蒼���āARTV���Đ���
	ComPtr<ID3D12Device> device;
	m_Swapchain->GetDevice(IID_PPV_ARGS(&device));

	for (UINT i = 0; i < m_Desc.BufferCount; i++)
	{
		m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_Images[i]));
		device->CreateRenderTargetView(m_Images[i].Get(), nullptr, m_ImageRenderTargetView[i]);
	}
}

void Swapchain::SetFullScreen(bool toFullScreen)
{
	if (toFullScreen)
	{
		ComPtr<IDXGIOutput> output;
		m_Swapchain->GetContainingOutput(&output);
		if (output)
		{
			DXGI_OUTPUT_DESC desc{};
			output->GetDesc(&desc);
		}
		m_Swapchain->SetFullscreenState(TRUE, nullptr);
	}
	else
	{
		m_Swapchain->SetFullscreenState(FALSE, nullptr);
	}
}

void Swapchain::ResizeTarget(const DXGI_MODE_DESC * newTargetParameter)
{
	m_Swapchain->ResizeTarget(newTargetParameter);
}

void Swapchain::SetMetaData()
{
	// RGB�J���[�X�y�[�X�̒�`
	struct DisplayChromacitiecs
	{
		float Red_x;
		float Red_y;
		float Green_x;
		float Green_y;
		float Blue_x;
		float Blue_y;
		float White_x;
		float White_y;
	} DisplayChromacityList[] = {
		{ 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // sRG(Rec709)
		{ 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // UHDTV(Rec2020)
	};

	int useIndex = 0;
	if (m_Desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		useIndex = 1;
	}

	const DisplayChromacitiecs& chroma = DisplayChromacityList[useIndex];

	DXGI_HDR_METADATA_HDR10 metaDataHDR10{};
	// ���K������l
	float valueNarmalize = 50000.0f;
	metaDataHDR10.RedPrimary[0] = static_cast<UINT16>(chroma.Red_x * valueNarmalize);
	metaDataHDR10.RedPrimary[1] = static_cast<UINT16>(chroma.Red_y * valueNarmalize);
	metaDataHDR10.GreenPrimary[0] = static_cast<UINT16>(chroma.Green_x * valueNarmalize);
	metaDataHDR10.GreenPrimary[1] = static_cast<UINT16>(chroma.Green_y * valueNarmalize);
	metaDataHDR10.BluePrimary[0] = static_cast<UINT16>(chroma.Blue_x * valueNarmalize);
	metaDataHDR10.BluePrimary[1] = static_cast<UINT16>(chroma.Blue_y * valueNarmalize);
	metaDataHDR10.WhitePoint[0] = static_cast<UINT16>(chroma.White_x * valueNarmalize);
	metaDataHDR10.WhitePoint[1] = static_cast<UINT16>(chroma.White_y * valueNarmalize);

	valueNarmalize = 10000.0f;
	metaDataHDR10.MaxMasteringLuminance = static_cast<UINT16>(1000.0f * valueNarmalize);
	metaDataHDR10.MinMasteringLuminance = static_cast<UINT16>(0.001f * valueNarmalize);
	metaDataHDR10.MaxContentLightLevel = static_cast<UINT16>(2000.0f);
	metaDataHDR10.MaxFrameAverageLightLevel = static_cast<UINT16>(500.0f);
	m_Swapchain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(metaDataHDR10), &metaDataHDR10);
}
