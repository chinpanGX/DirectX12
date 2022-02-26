/*------------------------------------------------------------
	
	[Application.cpp]
	Author : 出合翔太

-------------------------------------------------------------*/
#include "Application.h"
#include "TeapotModel.h"
#include <array>
#include <random>

Application::Application() : m_InstancingCount(100)
{

}

void Application::Run()
{
	m_ImGuiManager->BeginUpdate(ImVec2(m_Viewport.Width, m_Viewport.Height));
	m_ImGuiManager->Update(m_InstancingCount, m_Rotation, *m_ModelColorSet);
	m_ImGuiManager->Update(*m_Camera);
	
	UpdateCamera();
	UpdateModel();

	BeginRender();

	Render();

	// ImGuiの描画
	m_ImGuiManager->Render(m_CommandList);

	EndRender();	
}

void Application::Prepare()
{
	// タイトル
	SetTitle("インスタンシング描画");
	
	// バッファの転送をするためのコマンドリストを準備する
	m_CommandAllocators[m_FrameIndex]->Reset();
	m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);

	m_ModelColorSet = std::make_unique<ModelColorSet>();
	m_Camera = std::make_unique<Camera>();
	m_ImGuiManager = std::make_unique<ImGuiManager>();
	
	// ティーポットモデルの設定
	SetTeapotModel();
	
	// パイプラインステート設定
	SetPipelineState();

	// コンスタントバッファ設定
	SetConstantBuffer();

	// ImGuiの設定
	auto descriptor = m_Heap->Alloc();
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpu(descriptor);
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpu(descriptor);
	m_ImGuiManager->Init(m_hwnd, m_Device, m_SurfaceFormat, m_FrameBufferCount, m_Heap->GetHeap().Get(), hCpu, hGpu);
	
	m_ImGuiManager->InstancingDataMax(m_InstanceDataMax);
}

void Application::Cleanup()
{
	m_ImGuiManager->Uninit();
}

void Application::BeginRender()
{
	m_FrameIndex = m_Swapchain->GetCurrentBackBufferIndex();
	m_CommandAllocators[m_FrameIndex]->Reset();
	m_CommandList->Reset(m_CommandAllocators[m_FrameIndex].Get(), nullptr);

	// スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barrierToRenderTarget = m_Swapchain->GetBarrierToRenderTarget();
	m_CommandList->ResourceBarrier(1, &barrierToRenderTarget);

	auto renderTargetView = m_Swapchain->GetCurrentRenderTargetView();
	auto depthStencilView = m_DefaultDepthStencilView;

	// カラーバッファ(レンダーターゲットビュー)のクリア
	float m_clearColor[4] = { 0.3f, 1.0f, 0.3f, 0.0f };
	m_CommandList->ClearRenderTargetView(renderTargetView, m_clearColor, 0, nullptr);

	// デプスバッファ(デプスステンシルビュー)のクリア
	m_CommandList->ClearDepthStencilView(depthStencilView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 描画先をセット
	m_CommandList->OMSetRenderTargets(1, &static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(renderTargetView), false, &static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(depthStencilView));

	// ビューポートとシザーのセット
	auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
	auto scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height));
	m_CommandList->RSSetViewports(1, &viewport);
	m_CommandList->RSSetScissorRects(1, &scissorRect);
}

void Application::Render()
{
	ID3D12DescriptorHeap* heaps[] = { m_Heap->GetHeap().Get() };
	m_CommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	// シーン定数バッファ
	auto constantBuffer = m_ConstantBuffers[m_FrameIndex];
	void* map;
	constantBuffer->Map(0, nullptr, &map);
	memcpy(map, &m_SceneParameter, sizeof(m_SceneParameter));
	constantBuffer->Unmap(0, nullptr);

	// インスタンシングの定数バッファ
	auto instanceConstantBuffer = m_InstanceBuffers[m_FrameIndex];

	instanceConstantBuffer->Map(0, nullptr, &map);
	memcpy(map, m_InstanceData.data(), sizeof(InstanceData) * m_InstanceDataMax);
	instanceConstantBuffer->Unmap(0, nullptr);

	// モデルの描画
	m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_CommandList->IASetVertexBuffers(0, 1, &m_Model.VertexBufferView);
	m_CommandList->IASetIndexBuffer(&m_Model.IndexBufferView);

	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	m_CommandList->SetPipelineState(m_Pipeline.Get());

	m_CommandList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetGPUVirtualAddress());
	m_CommandList->SetGraphicsRootConstantBufferView(1, instanceConstantBuffer->GetGPUVirtualAddress());

	m_CommandList->DrawIndexedInstanced(m_Model.IndexCount, m_InstancingCount, 0, 0, 0);
}

void Application::EndRender()
{
	// レンダーターゲットからスワップチェイン表示可能へ
	auto barrierToPresent = m_Swapchain->GetBarrierToPresent();
	m_CommandList->ResourceBarrier(1, &barrierToPresent);
	m_CommandList->Close();
	ID3D12CommandList* lists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(1, lists);
	m_Swapchain->Present(1, 0);
	m_Swapchain->WaitPreviousFrame(m_CommandQueue, m_FrameIndex, m_GpuWaitTimeout);
}

void Application::SetTeapotModel()
{
	HRESULT hr;
	void* map;
	UINT bufferSize;

	// ティーポットモデルの頂点バッファ設定
	bufferSize = sizeof(TeapotModel::TeapotVerticesPN);
	// 読み取り用バッファ
	m_Model.ResourceVertexBuffer = CreateBufferResource(D3D12_HEAP_TYPE_DEFAULT, bufferSize, D3D12_RESOURCE_STATE_COPY_DEST);
	// アップロード用バッファ
	auto uploadVertexBuffer = CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	// アップロードリソースへのマップ
	hr = uploadVertexBuffer->Map(0, nullptr, &map);
	if (SUCCEEDED(hr))
	{
		memcpy(map, TeapotModel::TeapotVerticesPN, bufferSize);
		uploadVertexBuffer->Unmap(0, nullptr);
	}
	m_Model.VertexBufferView.BufferLocation = m_Model.ResourceVertexBuffer->GetGPUVirtualAddress();
	m_Model.VertexBufferView.SizeInBytes = bufferSize;
	m_Model.VertexBufferView.StrideInBytes = sizeof(TeapotModel::Vertex);
	// 読み取り用バッファへコピー
	m_CommandList->CopyResource(m_Model.ResourceVertexBuffer.Get(), uploadVertexBuffer.Get());

	// ティーポットモデルのインデックスバッファ設定
	bufferSize = sizeof(TeapotModel::TeapotIndices);
	// 読み取り専用バッファ
	m_Model.ResourceIndexBuffer = CreateBufferResource(D3D12_HEAP_TYPE_DEFAULT, bufferSize, D3D12_RESOURCE_STATE_COPY_DEST);
	// アップロードバッファ
	auto uploadIndexBuffer = CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
	// アップロードリソースへのマップ
	hr = uploadIndexBuffer->Map(0, nullptr, &map);
	if (SUCCEEDED(hr))
	{
		memcpy(map, TeapotModel::TeapotIndices, bufferSize);
		uploadIndexBuffer->Unmap(0, nullptr);
	}
	m_Model.IndexBufferView.BufferLocation = m_Model.ResourceIndexBuffer->GetGPUVirtualAddress();
	m_Model.IndexBufferView.SizeInBytes = bufferSize;
	m_Model.IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_Model.IndexCount = _countof(TeapotModel::TeapotIndices);
	// 読み取り専用バッファへコピー
	m_CommandList->CopyResource(m_Model.ResourceIndexBuffer.Get(), uploadIndexBuffer.Get());

	// コピー処理後、各バッファのステートを変更
	auto barrierVertexBuffer = CD3DX12_RESOURCE_BARRIER::Transition(m_Model.ResourceVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	auto barrierIndexBuffer = CD3DX12_RESOURCE_BARRIER::Transition(m_Model.ResourceIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	D3D12_RESOURCE_BARRIER barriers[] =
	{
		barrierVertexBuffer, barrierIndexBuffer, 
	};
	m_CommandList->ResourceBarrier(_countof(barriers), barriers);
	m_CommandList->Close();
	ID3D12CommandList* commandList[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(1, commandList);
	WaitForIdelGpu(); // 準備が完了するまで待機
}

void Application::SetPipelineState()
{
	HRESULT hr;
	ComPtr<ID3DBlob> errBlob;

	// 頂点シェーダーのコンパイル
	hr = CompileShaderFromFile(L"BasicVertexShader.hlsl", L"vs_6_0", m_VertexShader, errBlob);
	THROW_IF_FAILED(hr, "Compile VertexShader Error");

	// ピクセルシェーダーのコンパイル
	hr = CompileShaderFromFile(L"BasicPixelShader.hlsl", L"ps_6_0", m_PixelShader, errBlob);
	THROW_IF_FAILED(hr, "CompileShader PixelShader Error");

	// ルートシグネチャの構築
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	rootParameter[0].InitAsConstantBufferView(0);
	rootParameter[1].InitAsConstantBufferView(1);
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(_countof(rootParameter), rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	// ルートシグネチャのバージョンをシリアライズ化する
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errBlob);
	THROW_IF_FAILED(hr, "D3D12SerializeRootSignature 失敗");
	// ルートシグネチャの作成
	hr = m_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	THROW_IF_FAILED(hr, "CreateRootSignature 失敗");

	// インプットレイアウト
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(TeapotModel::Vertex, Position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(TeapotModel::Vertex, Normal),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },		
	};

	// パイプラインステートオブジェクトの作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateObjectDesc{};
	// シェーダーセット
	pipelineStateObjectDesc.VS = CD3DX12_SHADER_BYTECODE(m_VertexShader.Get());
	pipelineStateObjectDesc.PS = CD3DX12_SHADER_BYTECODE(m_PixelShader.Get());

	// ブレンドステート設定
	pipelineStateObjectDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// ラスタライザーステート
	pipelineStateObjectDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.RasterizerState.FrontCounterClockwise = true;
	pipelineStateObjectDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// 出力先は1ターゲット
	pipelineStateObjectDesc.NumRenderTargets = 1;
	pipelineStateObjectDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	// デプスバッファのフォーマット設定
	pipelineStateObjectDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateObjectDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateObjectDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };

	// ルートシグネチャのセット
	pipelineStateObjectDesc.pRootSignature = m_RootSignature.Get();
	pipelineStateObjectDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// マルチサンプル設定
	pipelineStateObjectDesc.SampleDesc = { 1, 0 };
	pipelineStateObjectDesc.SampleMask = UINT_MAX;

	hr = m_Device->CreateGraphicsPipelineState(&pipelineStateObjectDesc, IID_PPV_ARGS(&m_Pipeline));
	THROW_IF_FAILED(hr, "CreateGraphicsPipelineState 失敗");
}

void Application::SetConstantBuffer()
{
	UINT bufferSize = Util::RoundupConstantBufferSize(sizeof(SceneParameter));
	for (UINT i = 0; i < m_FrameBufferCount; i++)
	{
		auto constantBuffer = CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_ConstantBuffers.emplace_back(constantBuffer);
	}

	m_InstanceData.resize(m_InstanceDataMax);
	m_ModelAngleList.resize(m_InstanceDataMax);
	m_ModelPositionList.resize(m_InstanceDataMax);
	// 描画位置と色を決める
	std::random_device random;
	for (UINT i = 0; i < m_InstanceDataMax; i++)
	{
		// モデルの角度を乱数で出す
		float angle = static_cast<float>(random() % 360);
		// 各モデル角度を格納
		m_ModelAngleList[i] = angle;

		// 位置を出す
		float x = (i % 5) * 3.0f;
		float z = (i / 5) * -3.0f;
		// 各モデルの位置を格納
		m_ModelPositionList[i] = DirectX::XMFLOAT3(x, 0.0f, z);

		// マトリクス設定
		DirectX::XMMATRIX trans = DirectX::XMMatrixTranslation(x, 0.0f, z);
		DirectX::XMMATRIX matz = DirectX::XMMatrixRotationZ(angle);
		DirectX::XMMATRIX matx = DirectX::XMMatrixRotationX(angle);

		// データを格納
		auto& data = m_InstanceData[i];
		DirectX::XMStoreFloat4x4(&data.World, DirectX::XMMatrixTranspose(matz * matx * trans));
		data.Color = m_ModelColorSet->Color(i);
	}

	// インスタンシング用のデータを準備
	bufferSize = sizeof(InstanceData) * m_InstanceDataMax;
	bufferSize = Util::RoundupConstantBufferSize(bufferSize);
	for (UINT i = 0; i < m_FrameBufferCount; i++)
	{
		auto constantBuffer = CreateBufferResource(D3D12_HEAP_TYPE_UPLOAD, bufferSize, D3D12_RESOURCE_STATE_GENERIC_READ);
		m_InstanceBuffers.push_back(constantBuffer);

		void* map;
		constantBuffer->Map(0, nullptr, &map);
		memcpy(map, m_InstanceData.data(), bufferSize);
		constantBuffer->Unmap(0, nullptr);
	}
}

// カメラの更新
void Application::UpdateCamera()
{
	// カメラの設定
	DirectX::XMVECTOR cameraPosition = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&m_Camera->Position()), DirectX::XMLoadFloat4(&m_Camera->Offset()));
	DirectX::XMVECTOR cameraForce = DirectX::XMVectorAdd(DirectX::XMLoadFloat4(&m_Camera->Force()), DirectX::XMLoadFloat4(&m_Camera->Offset()));
	DirectX::XMVECTOR cameraUp = DirectX::XMLoadFloat4(&m_Camera->Up());
	// ビューマトリクス
	DirectX::XMMATRIX matView = DirectX::XMMatrixLookAtRH(cameraPosition, cameraForce, cameraUp);
	// プロジェクションマトリクス
	DirectX::XMMATRIX matProj = DirectX::XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(m_Camera->Fov()), m_Viewport.Width / m_Viewport.Height, 0.1f, 1000.0f);

	// 転置行列を変換して格納
	DirectX::XMStoreFloat4x4(&m_SceneParameter.View, DirectX::XMMatrixTranspose(matView));
	DirectX::XMStoreFloat4x4(&m_SceneParameter.Projection, DirectX::XMMatrixTranspose(matProj));
}

// モデルの更新
void Application::UpdateModel()
{
	UINT bufferSize = sizeof(InstanceData) * m_InstanceDataMax;
	bufferSize = Util::RoundupConstantBufferSize(bufferSize);
	for (UINT i = 0; i < m_InstanceDataMax; i++)
	{
		m_ModelAngleList[i] += m_Rotation;
		DirectX::XMFLOAT3 position = m_ModelPositionList[i];

		DirectX::XMMATRIX scl = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX rot = DirectX::XMMatrixRotationZ(m_ModelAngleList[i]);
		DirectX::XMMATRIX trans = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		DirectX::XMMATRIX world = scl * rot * trans;

		auto& data = m_InstanceData[i];
		DirectX::XMStoreFloat4x4(&data.World, DirectX::XMMatrixTranspose(world));
		data.Color = m_ModelColorSet->Color(i);
	}
}

Application::Buffer Application::CreateBufferResource(D3D12_HEAP_TYPE type, UINT bufferSize, D3D12_RESOURCE_STATES state)
{
	Buffer ret;
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(type);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	HRESULT hr = m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &buffer, state, nullptr, IID_PPV_ARGS(&ret));
	THROW_IF_FAILED(hr, "CreateCommittedResource");
	return ret;
}
