/*---------------------------------------------------------------

	[Main.cpp]
	Author : 出合翔太

----------------------------------------------------------------*/
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdexcept>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "Application.h"

// ウィンドウサイズの設定
const int WINDOW_WIDTH = 1600;
const int WINDOW_HEIGHT = 900;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// コールバック関数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	// ポインタ型を強制的に変換する
	AppBase* appbase = reinterpret_cast<AppBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp))
	{
		return TRUE;
	}

	switch (msg)
	{
	case WM_PAINT:
		if (appbase)
		{
			appbase->Run();
		}
		return 0;
	
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_SIZE:
		if (appbase)
		{
			RECT rect{};
			GetClientRect(hwnd, &rect);
			bool isMinimized = wp == SIZE_MINIMIZED;
			appbase->OnSizeChanged(rect.right - rect.left, rect.bottom - rect.top, isMinimized);
		}
		return 0;

	case WM_SYSKEYDOWN:
		if ((wp == VK_RETURN) && (lp & (1 << 29)))
		{
			if (appbase)
			{
				appbase->ToggleFullScreen();
			}
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

// WinMain関数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	Application application{};

	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = "DirectX12";
	RegisterClassEx(&windowClass);

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;// &~WS_SIZEBOX;
	RECT rect = { 0,0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&rect, dwStyle, FALSE);

	auto hwnd = CreateWindow(windowClass.lpszClassName, "DirectX12", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, &application);

	try
	{
		application.Init(hwnd, DXGI_FORMAT_R8G8B8A8_UNORM, false);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&application));
		ShowWindow(hwnd, nCmdShow);

		MSG msg{};
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		application.Uninit();
		return static_cast<int>(msg.wParam);
	}
	// 例外処理が発生したときの処理
	catch(std::runtime_error error)
	{
		OutputDebugStringA(error.what());
		OutputDebugStringA("\n");
	}
	return 0;
}