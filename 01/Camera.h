/*------------------------------------------------------------
	
	[Camera.h]
	Author : èoçá„ƒëæ

-------------------------------------------------------------*/
#pragma once
#include <DirectXMath.h>

class Camera final
{
public:
	Camera() {};
	~Camera() {};
	const DirectX::XMFLOAT4& Position() const { return m_Position; }
	const DirectX::XMFLOAT4& Force() const { return m_Force; }
	const DirectX::XMFLOAT4& Up() const { return m_Up; }
	DirectX::XMFLOAT4& Offset() { return m_Offset; }
	float& Fov() { return m_Fov; }
private:
	DirectX::XMFLOAT4 m_Position = DirectX::XMFLOAT4(3.0f, 5.0f, 10.0f, 0.0f);
	DirectX::XMFLOAT4 m_Force = DirectX::XMFLOAT4(3.0f, 2.0f, 0.0f, 0.0f);
	const DirectX::XMFLOAT4 m_Up = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMFLOAT4 m_Offset = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	float m_Fov = 45.0f;
};

