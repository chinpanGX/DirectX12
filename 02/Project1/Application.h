/*------------------------------------------------------------
	
	[Application.h]
	Author : èoçá

--------------------------------------------------------------*/
#pragma once
#include <Windows.h>

class Application
{
public:
	void Startup();
	void Mainloop();
private:
	void InitWindow();
	HINSTANCE m_Instance;
	HWND m_Wnd;
};

