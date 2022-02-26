/*------------------------------------------------------------
	
	[ModelColorSet.h]
	Author : �o���đ�

-------------------------------------------------------------*/
#pragma once
#include <vector>
#include <DirectXMath.h>

class ModelColorSet final
{
public:
	ModelColorSet();
	~ModelColorSet();
	void Update();
	float* Color();
	int Size();
	const DirectX::XMFLOAT4& Color(int index) const;
	// �`�F�b�N�{�b�N�X�֌W
	bool& CheckBoxFlagPush();
	bool& CheckBoxFlagPopBack();
private:
	std::vector<DirectX::XMFLOAT4> m_ColorSetList;
	float m_Color[3];
	bool m_IsPush = false;
	bool m_IsPop = false;
};

