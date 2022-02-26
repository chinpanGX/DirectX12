/*------------------------------------------------------------
	
	[ImGuiManager.cpp]
	Author : �o���đ�
	
-------------------------------------------------------------*/
#include "ModelColorSet.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

void ImGuiManager::Init(HWND hwnd, Microsoft::WRL::ComPtr<ID3D12Device> device, DXGI_FORMAT formatRTV, UINT bufferCount, ID3D12DescriptorHeap * heap, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device.Get(), bufferCount, formatRTV, heap, cpuDescriptorHandle, gpuDescriptorHandle);
}

void ImGuiManager::Uninit()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::BeginUpdate(ImVec2 window)
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 0.2f, 0.0f, 1.0f));
	ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_::ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(250, 110), ImGuiCond_::ImGuiCond_Once);
	
	// �^�C�g��
	float framerate = ImGui::GetIO().Framerate;

	ImGui::Begin("Infomation");
	ImGui::Text("Instancing Using ConstantBuffer");

	// �t���[�����[�g
	ImGui::Text("Fps : %.1f", framerate);

	// �E�B���h�E�T�C�Y
	ImGui::Text("Window ");
	ImGui::Text("Width : %.0f", window.x);
	ImGui::SameLine();
	ImGui::Text("Heigth : %.0f", window.y);
	
	ImGui::End();
	ImGui::PopStyleColor();
}

void ImGuiManager::Update(int & instancingCount, float & Rotation, ModelColorSet & modelColorSet)
{
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 0.2f, 0.0f, 1.0f));
	
	ImGui::SetNextWindowPos(ImVec2(40, 160), ImGuiCond_::ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(250, 430), ImGuiCond_::ImGuiCond_Once);
	
	ImGui::Begin("Model");
	ImGui::Text("Instancing Count");
	// �X���C�_�[�ő���
	ImGui::SliderInt("Count", &instancingCount, 1, m_InstancingDataMax);
	// �������
	bool addFlag = false;
	bool deleteFlag = false;
	ImGui::Checkbox("AddModel", &addFlag);
	ImGui::SameLine();
	ImGui::Checkbox("RemoveModel", &deleteFlag);
	// ����₷
	if (addFlag)
	{
		instancingCount++;
	}
	// ����炷
	if (deleteFlag)
	{
		instancingCount--;
	}

	// ��]����
	ImGui::Text("Direction of Rotation");
	// ��
	ImGui::RadioButton("Left", &m_EnableRightRotation, 1);
	ImGui::SameLine();
	// �E
	ImGui::RadioButton("Right", &m_EnableRightRotation, 0);
	ImGui::SameLine();
	// �~�߂�
	ImGui::RadioButton("Stop", &m_EnableRightRotation, 2);
	// ���x
	ImGui::SliderFloat("Speed", &m_Speed, 0.01f, 1.0f);

	// ��]�����̍X�V
	switch (m_EnableRightRotation)
	{
	case 0:
		Rotation = -m_Speed;
		break;
	case 1:
		Rotation = m_Speed;
		break;
	case 2:
		Rotation = 0.0f;
		break;
	}

	// �J���[�s�b�J�[
	ImGui::Text("ModelColor");
	ImGui::Text("ColorNum : %d", modelColorSet.Size());
	ImGui::ColorPicker3("Color", modelColorSet.Color(), ImGuiColorEditFlags_::ImGuiColorEditFlags_InputRGB);
	ImGui::Checkbox("AddColor", &modelColorSet.CheckBoxFlagPush());
	ImGui::SameLine();
	ImGui::Checkbox("RemoveColor", &modelColorSet.CheckBoxFlagPopBack());

	modelColorSet.Update();

	ImGui::End();
	ImGui::PopStyleColor();
}

void ImGuiManager::Update(Camera & camera)
{
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(1.0f, 0.2f, 0.0f, 1.0f));

	ImGui::SetNextWindowPos(ImVec2(40, 600), ImGuiCond_::ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(250, 130), ImGuiCond_::ImGuiCond_Once);

	ImGui::Begin("Camera");
	ImGui::SliderFloat("x", &camera.Offset().x, -25.0f, 25.0f);
	ImGui::SliderFloat("y", &camera.Offset().y, -25.0f, 25.0f);
	ImGui::SliderFloat("z", &camera.Offset().z, 0.0f, -100.0f);
	ImGui::SliderFloat("Fov", &camera.Fov(), 0.1f, 90.0f);
	ImGui::End();

	ImGui::PopStyleColor();
}

void ImGuiManager::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void ImGuiManager::InstancingDataMax(int maxData)
{
	m_InstancingDataMax = maxData;
}
