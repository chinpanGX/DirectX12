/*------------------------------------------------------------

	[AppBase.cpp]
	Author : 出合翔太

-------------------------------------------------------------*/
#include "AppBase.h"
#include <exception>
#include <fstream>
#if _MSC_VER > 1922 && !defined(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPECTION_WARNING
#endif
#include <experimental/filesystem>
#include "Util.h"
#include <dxcapi.h>
#pragma comment(lib, "dxcompiler.lib")

AppBase::AppBase()
{
	m_FrameIndex = 0;
	m_WaitFence = CreateEvent(NULL, FALSE, FALSE, NULL);
}

AppBase::~AppBase()
{
	CloseHandle(m_WaitFence);
}

void AppBase::Init(HWND hwnd, DXGI_FORMAT format, bool isFullScreen)
{
	m_hwnd = hwnd;
	HRESULT hr;
	UINT dxgiFlags = 0;
	// デバッグレイヤーを有効にする
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugLayer;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer))))
	{
		debugLayer->EnableDebugLayer();
		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ComPtr<IDXGIFactory5> factory;
	hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&factory));
	THROW_IF_FAILED(hr, "CreateDXGIFactory2 失敗");

	// ハードウェアアダプタの検索
	ComPtr<IDXGIAdapter1> useAdapter;
	UINT adapterIndex = 0;
	ComPtr<IDXGIAdapter1> adapter;
	while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter))
	{
		DXGI_ADAPTER_DESC1 desc1{};
		adapter->GetDesc1(&desc1);
		adapterIndex++;
		if (desc1.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
		{
			continue;
		}

		hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	adapter.As(&useAdapter); // 使用アダプター

	// デバイスの作成
	hr = D3D12CreateDevice(useAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));
	THROW_IF_FAILED(hr, "D3D12CreateDevice 失敗");

	// コマンドキューの生成
	D3D12_COMMAND_QUEUE_DESC queueDesc
	{
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		0,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		0
	};
	hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));
	THROW_IF_FAILED(hr, "CreateCommandQueue 失敗");

	// 各ディスクリプタヒープの準備
	PrepareDescriptorHeaps();

	// HWNDからクライアント領域サイズを判定
	RECT rect;
	GetClientRect(hwnd, &rect);
	m_Width = rect.right - rect.left;
	m_Height = rect.bottom - rect.top;

	// HDRの設定
	bool useHDR = format == DXGI_FORMAT_R16G16B16A16_FLOAT || format == DXGI_FORMAT_R10G10B10A2_UNORM;
	if (useHDR)
	{
		bool isDisplayHDR = false;
		UINT index = 0;
		ComPtr<IDXGIOutput> current;
		while (useAdapter->EnumOutputs(index, &current) != DXGI_ERROR_NOT_FOUND)
		{
			ComPtr<IDXGIOutput6> output;
			current.As(&output);

			DXGI_OUTPUT_DESC1 desc;
			output->GetDesc1(&desc);
			isDisplayHDR |= desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			index++;
		}

		if (!isDisplayHDR)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			useHDR = false;
		}
	}

	// グラフィックスドライバーでサポートされている機能に関する情報を取得
	BOOL allowTearing = FALSE;
	hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
	m_IsAllowTearing = SUCCEEDED(hr) && allowTearing;

	// スワップチェインの生成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
	swapchainDesc.BufferCount = m_FrameBufferCount;
	swapchainDesc.Width = m_Width;
	swapchainDesc.Height = m_Height;
	swapchainDesc.Format = format;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapchainDesc.SampleDesc.Count = 1;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
	fullscreenDesc.Windowed = isFullScreen ? FALSE : TRUE;
	fullscreenDesc.RefreshRate.Denominator = 1000;
	fullscreenDesc.RefreshRate.Numerator = 60317;
	fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;

	ComPtr<IDXGISwapChain1> swapchain;
	hr = factory->CreateSwapChainForHwnd(m_CommandQueue.Get(), hwnd, &swapchainDesc, &fullscreenDesc, nullptr, &swapchain);
	THROW_IF_FAILED(hr, "CreateSwapChainForHwnd 失敗");
	m_Swapchain = std::make_shared<Swapchain>(swapchain, m_HeapRenderTargetView);

	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	m_SurfaceFormat = m_Swapchain->GetFormat();

	// デプスバッファ関連の準備
	CreateDefaultDepthBuffer(m_Width, m_Height);

	// コマンドアロケーターの準備
	CreateCommandAllocators();

	// コマンドリストの生成
	hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
	THROW_IF_FAILED(hr, "CreateCommandList 失敗");
	m_CommandList->Close();

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(m_Width), float(m_Height));
	m_ScissorRect = CD3DX12_RECT(0, 0, LONG(m_Width), LONG(m_Height));
	
	Prepare();
}

void AppBase::Uninit()
{
	WaitForIdelGpu();
	Cleanup();
}

void AppBase::Run()
{
	m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();

	m_CommandAllocators[m_FrameIndex]->Reset();
	m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);

	// スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barrierToRenderTarget = m_Swapchain->GetBarrierToRenderTarget();
	m_CommandList->ResourceBarrier(1, &barrierToRenderTarget);

	DescriptorHandle renderTargetView = m_Swapchain->GetCurrentRenderTargetView();
	DescriptorHandle depthStencilView = m_DefaultDepthStencilView;

	// 画面クリア
	const float clearColor[] = { 0.5f, 0.75f,1.0f,0.0f };
	m_CommandList->ClearRenderTargetView(renderTargetView, clearColor, 0, nullptr);

	// デプスバッファのクリア
	m_CommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画先のセット
	m_CommandList->OMSetRenderTargets(1, &static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(renderTargetView), false, &static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(depthStencilView));

	ID3D12DescriptorHeap* heaps[] = { m_Heap->GetHeap().Get() };
	m_CommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// レンダーターゲットからスワップチェイン表示可能へ
	auto barrierToPresent = m_Swapchain->GetBarrierToPresent();
	m_CommandList->ResourceBarrier(1, &barrierToPresent);
	m_CommandList->Close();

	ID3D12CommandList* lists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(1, lists);

	m_Swapchain->Present(1, 0);
	// 次のコマンドを積むまで待機
	m_Swapchain->WaitPreviousFrame(m_CommandQueue, m_FrameIndex, m_GpuWaitTimeout);
}

void AppBase::OnSizeChanged(UINT width, UINT height, bool isMinimized)
{
	m_Width = width;
	m_Height = height;
	if (!m_Swapchain || isMinimized)
	{
		return;
	}

	// 処理の完了を待ってからサイズ変更
	WaitForIdelGpu();
	m_Swapchain->ResizeBuffers(width, height);

	// デプスバッファを作り直す
	m_DepthBuffers.Reset();
	m_HeapDepthStencilView->Free(m_DefaultDepthStencilView);
	CreateDefaultDepthBuffer(m_Width, m_Height);

	m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();

	m_Viewport.Width = static_cast<float>(m_Width);
	m_Viewport.Height = static_cast<float>(m_Height);
	m_ScissorRect.right = m_Width;
	m_ScissorRect.bottom = m_Height;
}

void AppBase::SetTitle(const std::string & title)
{
	SetWindowTextA(m_hwnd, title.c_str());
}

void AppBase::ToggleFullScreen()
{
	// フルスクリーン → ウィンドウ
	if(m_Swapchain->GetIsFullScreen())
	{
		m_Swapchain->SetFullScreen(false);
		SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
		ShowWindow(m_hwnd, SW_NORMAL);
	}
	// ウィンドウ → フルスクリーン
	else
	{
		DXGI_MODE_DESC desc;
		desc.Format = m_SurfaceFormat;
		desc.Width = m_Width;
		desc.Height = m_Height;
		desc.RefreshRate.Denominator = 1;
		desc.RefreshRate.Numerator = 60;
		desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		m_Swapchain->ResizeTarget(&desc);
		m_Swapchain->SetFullScreen(true);
	}
	OnSizeChanged(m_Width, m_Height, false);
}

Microsoft::WRL::ComPtr<ID3D12Resource1> AppBase::CreateResource(const CD3DX12_RESOURCE_DESC & desc, D3D12_RESOURCE_STATES resourceState, const D3D12_CLEAR_VALUE * clearValue, D3D12_HEAP_TYPE heapType)
{
	ComPtr<ID3D12Resource1> ret;
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(heapType);
	HRESULT hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, resourceState, clearValue, IID_PPV_ARGS(&ret));
	THROW_IF_FAILED(hr, "CreateCommittedResource 失敗");
	return ret;
}

std::vector<Microsoft::WRL::ComPtr<ID3D12Resource1>> AppBase::CreateConstantBuffers(const CD3DX12_RESOURCE_DESC & desc, int count)
{
	std::vector<ComPtr<ID3D12Resource1>> buffers;
	for (int i = 0; i < count; i++)
	{
		buffers.emplace_back(CreateResource(desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, D3D12_HEAP_TYPE_UPLOAD));
	}
	return buffers;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> AppBase::CreateCommandList()
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	HRESULT hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_OneShotCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	THROW_IF_FAILED(hr, "CreateCommandList 失敗");
	commandList->SetName(L"OneShotCommand");
	return commandList;
}

void AppBase::FinishCommandList(ComPtr<ID3D12GraphicsCommandList>& command)
{
	ID3D12CommandList* commandList[] = { command.Get() };
	command->Close();
	m_CommandQueue->ExecuteCommandLists(1, commandList);
	ComPtr<ID3D12Fence1> fence;
	HRESULT hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	THROW_IF_FAILED(hr, "CreateFence 失敗");
	const UINT64 expectValue = 1;
	m_CommandQueue->Signal(fence.Get(), expectValue);
	do
	{
		
	} while (fence->GetCompletedValue() != expectValue);
	m_OneShotCommandAllocator->Reset();
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> AppBase::CreateBundleCommandList()
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	HRESULT hr = m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_OneShotCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	THROW_IF_FAILED(hr, "CreateCommandList 失敗");	
	return commandList;
}

void AppBase::WriteToUploadHeapMemory(ID3D12Resource1 * resource, uint32_t size, const void * data)
{
	void* map;
	HRESULT hr = resource->Map(0, nullptr, &map);
	// 成功したら、メモリをコピー
	if (SUCCEEDED(hr))
	{
		memcpy(map, data, size);
		resource->Unmap(0, nullptr);
	}
	THROW_IF_FAILED(hr, "map/unmap 失敗");
}

#pragma region _private_Fenction_
// ディスクリプタヒープの準備
void AppBase::PrepareDescriptorHeaps()
{
	const int maxDescriptorCount = 2048; // ShaderResourceView(SRV)、ConstantBufferView(CBV)、UnorderedAccessView(UAV)
	const int maxDescriptorCountRenderTargetView = 100; // RenderTargetView
	const int maxDescriptorCountDepthStencilView = 100; // DepthStencilView

	// RenderTargetViewのディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDescRTV
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		maxDescriptorCountRenderTargetView,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	};
	m_HeapRenderTargetView = std::make_shared<DescriptorManager>(m_Device, heapDescRTV);

	// DepthStencilViewのディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDescDSV
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		maxDescriptorCountDepthStencilView,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	};
	m_HeapDepthStencilView = std::make_shared<DescriptorManager>(m_Device, heapDescDSV);

	// ShaderResourceViewのディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc
	{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		maxDescriptorCount,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	};
	m_Heap = std::make_shared<DescriptorManager>(m_Device, heapDesc);
}

void AppBase::CreateDefaultDepthBuffer(int width, int height)
{
	// デプスバッファの生成
	auto depthBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.Format = depthBufferDesc.Format;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.DepthStencil.Stencil = 0;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthBufferDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,&depthClearValue, IID_PPV_ARGS(&m_DepthBuffers));
	THROW_IF_FAILED(hr, "CreateCommittedResource 失敗");

	// デプスステンシルビュー生成
	m_DefaultDepthStencilView = m_HeapDepthStencilView->Alloc();
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		DXGI_FORMAT_D32_FLOAT,
		D3D12_DSV_DIMENSION_TEXTURE2D,
		D3D12_DSV_FLAG_NONE,
		{
			0
		}
	};
	m_Device->CreateDepthStencilView(m_DepthBuffers.Get(), &dsvDesc, m_DefaultDepthStencilView);
}

// コマンドアロケーターの作成
void AppBase::CreateCommandAllocators()
{
	HRESULT hr;
	m_CommandAllocators.resize(m_FrameBufferCount);
	for (UINT i = 0; i < m_FrameBufferCount; i++)
	{
		hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i]));
		if (FAILED(hr))
		{
			throw std::runtime_error("CreateCommandAllocator 失敗");
		}
	}

	hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_OneShotCommandAllocator));
	THROW_IF_FAILED(hr, "CreateCommandAllocator(OneShot) 失敗");
	
	hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_BundleCommandAloocator));
	THROW_IF_FAILED(hr, "CreateCommandAllocator(Bundle) 失敗");
}

void AppBase::WaitForIdelGpu()
{
	// 発行済みコマンドの終了を待つ
	ComPtr<ID3D12Fence1> fence;
	const UINT64 expectValue = 1;
	HRESULT hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	THROW_IF_FAILED(hr, "CreateFence 失敗");

	m_CommandQueue->Signal(fence.Get(), expectValue);
	if (fence->GetCompletedValue() != expectValue)
	{
		fence->SetEventOnCompletion(expectValue, m_WaitFence);
		WaitForSingleObject(m_WaitFence, INFINITE);
	}
}
#pragma endregion privateメンバ関数

// シェーダーファイルのコンパイル
HRESULT CompileShaderFromFile(const std::wstring & fileName, const std::wstring & profile, Microsoft::WRL::ComPtr<ID3DBlob>& shaderBlob, Microsoft::WRL::ComPtr<ID3DBlob>& errorBlob)
{
	using namespace std::experimental::filesystem;
	using namespace Microsoft::WRL;
	path filePath(fileName);
	std::ifstream infile(filePath, std::ifstream::binary);
	std::vector<char> srcdata;
	// シェーダーファイルが見つからなかったとき
	if (!infile)
	{
		throw std::runtime_error("Shader Not Found");
	}
	srcdata.resize(static_cast<uint32_t>(infile.seekg(0, infile.end).tellg()));
	infile.seekg(0, infile.beg).read(srcdata.data(), srcdata.size());

	// DXCによるコンパイル処理
	ComPtr<IDxcLibrary> library;
	ComPtr<IDxcCompiler> compiler;
	ComPtr<IDxcBlobEncoding> source;
	ComPtr<IDxcOperationResult> dxcResult;

	DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	library->CreateBlobWithEncodingFromPinned(srcdata.data(), static_cast<UINT>(srcdata.size()), CP_ACP, &source);
	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

	LPCWSTR compilerFlags[] = 
	{
  #if _DEBUG
	  L"/Zi", L"/O0",
  #else
	  L"/O2" // リリースビルドでは最適化
  #endif
	};

	compiler->Compile(source.Get(), filePath.wstring().c_str(), L"main" , profile.c_str(), compilerFlags, _countof(compilerFlags), nullptr, 0, nullptr, &dxcResult);
	HRESULT hr;
	dxcResult->GetStatus(&hr);
	if (SUCCEEDED(hr))
	{
		dxcResult->GetResult(reinterpret_cast<IDxcBlob**>(shaderBlob.GetAddressOf()));
	}
	else
	{
		dxcResult->GetErrorBuffer(reinterpret_cast<IDxcBlobEncoding**>(errorBlob.GetAddressOf()));
	}
	return hr;
}
