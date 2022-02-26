/*--------------------------------------------------------------
	
	[DescriptorManager.h]
	Author : �o���đ�

---------------------------------------------------------------*/
#pragma once
#include <wrl.h>
#include <list>
#include "Util.h"
#include "d3dx12.h"

class DescriptorHandle final
{
public:
	DescriptorHandle() : m_CpuHandle(), m_GpuHandle() {}
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE hCpu, D3D12_GPU_DESCRIPTOR_HANDLE hGpu) : m_CpuHandle(hCpu), m_GpuHandle(hGpu) {}
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CpuHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GpuHandle; }
private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
};

class DescriptorManager final
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	DescriptorManager() = delete;
	DescriptorManager(ComPtr<ID3D12Device> device, const D3D12_DESCRIPTOR_HEAP_DESC& desc) : m_Index(0), m_IncrementSize(0) 
	{
		// �f�B�X�N���v�^�q�[�v�̍쐬
		HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap));
		THROW_IF_FAILED(hr, "CreateDescriptorHeap ���s");

		// �f�B�X�N���v�^�q�[�v�n���h���̐擪�A�h���X�̎擾
		m_CpuDescriptorHandle = m_Heap->GetCPUDescriptorHandleForHeapStart();
		m_GpuDescriptorHandle = m_Heap->GetGPUDescriptorHandleForHeapStart();
		// �f�B�X�N���v�^1������̃T�C�Y
		m_IncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);
	}

	ComPtr<ID3D12DescriptorHeap> GetHeap() const { return m_Heap; }

	DescriptorHandle Alloc()
	{
		// �󂶂�Ȃ����
		if (!m_FreeList.empty())
		{
			// �擪�v�f�̎Q�Ƃ�Ԃ�
			auto ret = m_FreeList.front();
			// �擪�v�f���폜
			m_FreeList.pop_front(); 
			return ret;
		}

		UINT use = m_Index++;
		auto ret = DescriptorHandle(m_CpuDescriptorHandle.Offset(use, m_IncrementSize), m_GpuDescriptorHandle.Offset(use, m_IncrementSize));
		return ret;
	}

	void Free(const DescriptorHandle& handle)
	{
		m_FreeList.push_back(handle);
	}

private:
	std::list<DescriptorHandle> m_FreeList;
	ComPtr<ID3D12DescriptorHeap> m_Heap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_GpuDescriptorHandle;
	UINT m_Index;
	UINT m_IncrementSize;
};

