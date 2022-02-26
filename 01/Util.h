/*--------------------------------------------------------------
	
	[Util.h]
	Author : 出合翔太

---------------------------------------------------------------*/
#pragma once
#include <stdexcept>
#include <d3d12.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#define STRINGFY(s) #s
#define TO_STRING(x) STRINGFY(x)
#define FILE_PREFIX __FILE__ "(" TO_STRING(__LINE__) "):"
#define THROW_IF_FAILED(hr, msg) Util::CheckResultCode(hr, FILE_PREFIX msg)

namespace Util
{
	class Dx12Exception final : public std::runtime_error
	{
	public:
		Dx12Exception() = delete;
		Dx12Exception(const std::string& Msg) : std::runtime_error(Msg.c_str()) {}
	};

	// Resultチェック関数
	inline void CheckResultCode(HRESULT hr, const std::string& msg)
	{
		// 失敗したら、エラーを出す
		if (FAILED(hr))
		{
			throw Dx12Exception(msg);
		}
	}

	// コンスタントバッファのサイズの切り上げ
	inline UINT RoundupConstantBufferSize(UINT size)
	{
		UINT tmp = (size + 255) & ~255;
		return tmp;
	}

	// Teapotモデルのラスタライザーの初期化
	inline CD3DX12_RASTERIZER_DESC CreateTeapotModelRasterzerDesc()
	{
		// Teapotモデルは半時計並びでデータが定義されている
		auto desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		// カリングモードの指定
		desc.FrontCounterClockwise = true;
		return desc;
	}

	// デフォルトのPipelineStateObjectの生成
	inline D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefautPsoDesc(DXGI_FORMAT targetFormat, Microsoft::WRL::ComPtr<ID3DBlob> vertexShader, Microsoft::WRL::ComPtr<ID3DBlob> pixelShader,
		CD3DX12_RASTERIZER_DESC rasterizerDesc, const D3D12_INPUT_ELEMENT_DESC* inputElementDesc, UINT inputElementDescCount, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		// シェーダーセット
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		// ブレンドステート設定
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		// ラスタライザーステート設定
		psoDesc.RasterizerState = rasterizerDesc;

		// 出力先は1ターゲット
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = targetFormat;		
		// デプスバッファのフォーマット設定
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = { inputElementDesc, inputElementDescCount };

		// ルートシグネチャのセット
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// マルチサンプル設定
		psoDesc.SampleDesc = { 1,0 };
		psoDesc.SampleMask = UINT_MAX; // これを忘れると絵が出ない&警告も出ない
		return psoDesc;
	}

	// リソースとヒープの両方を作成する
	inline Microsoft::WRL::ComPtr<ID3D12Resource1> CreateBufferOnUploadHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT bufferSize, const void* data = nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource1> ret;
		auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceBuffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		
		// リソースをヒープの作成
		HRESULT hr = device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ret));		
		THROW_IF_FAILED(hr, "CreateCommittedResource　Failed");
		
		if (data != nullptr)
		{
			void* map;
			hr = ret->Map(0, nullptr, &map);
			THROW_IF_FAILED(hr, "Map Filed");
			if (map)
			{
				memcpy(map, data, bufferSize);
				ret->Unmap(0, nullptr);
			}
		}

		return ret;
	}

	// DirectX::XMFLOAT4へコンバート
	inline DirectX::XMFLOAT4 toFloat4(const DirectX::XMFLOAT3& v, float w)
	{
		return DirectX::XMFLOAT4(v.x, v.y, v.z, w);
	}

	// wstringへコンバート
	inline std::wstring ConvertWstring(const std::string& str)
	{
		std::vector<wchar_t> buf;
		int len = static_cast<int>(str.length());
		// 文字列が空のとき
		if (str.empty())
		{
			return std::wstring();
		}
		
		buf.resize(len * 4);
		MultiByteToWideChar(932, 0, str.data(), -1, buf.data(), static_cast<int>(buf.size()));
		return std::wstring(buf.data());
	}
}
