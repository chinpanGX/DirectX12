/*------------------------------------------------------------

	[ModelColorSet.cpp]
	Author : 出合翔太

-------------------------------------------------------------*/
#include "ModelColorSet.h"

ModelColorSet::ModelColorSet()
{
	m_ColorSetList.push_back(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
}

ModelColorSet::~ModelColorSet()
{
	m_ColorSetList.clear();
}

void ModelColorSet::Update()
{
	// プッシュフラグをチェックする
	if (m_IsPush)
	{
		DirectX::XMFLOAT4 tmp;
		tmp.x = m_Color[0];
		tmp.y = m_Color[1];
		tmp.z = m_Color[2];
		tmp.w = 1.0f;
		m_ColorSetList.push_back(tmp);
		m_IsPush = false;
	}
	if (m_IsPop)
	{
		// 最初の一つだけなら消さない
		if (m_ColorSetList.size() != 1)
		{
			m_ColorSetList.pop_back();
		}
		m_IsPop = false;
	}
}

float * ModelColorSet::Color()
{
	return m_Color;
}

int ModelColorSet::Size()
{
	return static_cast<int>(m_ColorSetList.size());
}

const DirectX::XMFLOAT4 & ModelColorSet::Color(int index) const
{
	return m_ColorSetList[index % m_ColorSetList.size()];
}

bool& ModelColorSet::CheckBoxFlagPush()
{
	return m_IsPush;
}

bool& ModelColorSet::CheckBoxFlagPopBack()
{
	return m_IsPop;
}
