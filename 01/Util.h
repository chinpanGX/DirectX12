/*--------------------------------------------------------------
	
	[Util.h]
	Author : �o���đ�

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

	// Result�`�F�b�N�֐�
	inline void CheckResultCode(HRESULT hr, const std::string& msg)
	{
		// ���s������A�G���[���o��
		if (FAILED(hr))
		{
			throw Dx12Exception(msg);
		}
	}

	// �R���X�^���g�o�b�t�@�̃T�C�Y�̐؂�グ
	inline UINT RoundupConstantBufferSize(UINT size)
	{
		UINT tmp = (size + 255) & ~255;
		return tmp;
	}

	// Teapot���f���̃��X�^���C�U�[�̏�����
	inline CD3DX12_RASTERIZER_DESC CreateTeapotModelRasterzerDesc()
	{
		// Teapot���f���͔����v���тŃf�[�^����`����Ă���
		auto desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		// �J�����O���[�h�̎w��
		desc.FrontCounterClockwise = true;
		return desc;
	}

	// �f�t�H���g��PipelineStateObject�̐���
	inline D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefautPsoDesc(DXGI_FORMAT targetFormat, Microsoft::WRL::ComPtr<ID3DBlob> vertexShader, Microsoft::WRL::ComPtr<ID3DBlob> pixelShader,
		CD3DX12_RASTERIZER_DESC rasterizerDesc, const D3D12_INPUT_ELEMENT_DESC* inputElementDesc, UINT inputElementDescCount, Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		// �V�F�[�_�[�Z�b�g
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		// �u�����h�X�e�[�g�ݒ�
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		// ���X�^���C�U�[�X�e�[�g�ݒ�
		psoDesc.RasterizerState = rasterizerDesc;

		// �o�͐��1�^�[�Q�b�g
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = targetFormat;		
		// �f�v�X�o�b�t�@�̃t�H�[�}�b�g�ݒ�
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.InputLayout = { inputElementDesc, inputElementDescCount };

		// ���[�g�V�O�l�`���̃Z�b�g
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// �}���`�T���v���ݒ�
		psoDesc.SampleDesc = { 1,0 };
		psoDesc.SampleMask = UINT_MAX; // �����Y���ƊG���o�Ȃ�&�x�����o�Ȃ�
		return psoDesc;
	}

	// ���\�[�X�ƃq�[�v�̗������쐬����
	inline Microsoft::WRL::ComPtr<ID3D12Resource1> CreateBufferOnUploadHeap(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT bufferSize, const void* data = nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource1> ret;
		auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceBuffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		
		// ���\�[�X���q�[�v�̍쐬
		HRESULT hr = device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &resourceBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&ret));		
		THROW_IF_FAILED(hr, "CreateCommittedResource�@Failed");
		
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

	// DirectX::XMFLOAT4�փR���o�[�g
	inline DirectX::XMFLOAT4 toFloat4(const DirectX::XMFLOAT3& v, float w)
	{
		return DirectX::XMFLOAT4(v.x, v.y, v.z, w);
	}

	// wstring�փR���o�[�g
	inline std::wstring ConvertWstring(const std::string& str)
	{
		std::vector<wchar_t> buf;
		int len = static_cast<int>(str.length());
		// �����񂪋�̂Ƃ�
		if (str.empty())
		{
			return std::wstring();
		}
		
		buf.resize(len * 4);
		MultiByteToWideChar(932, 0, str.data(), -1, buf.data(), static_cast<int>(buf.size()));
		return std::wstring(buf.data());
	}
}
