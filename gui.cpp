#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_impl_win32.h"
#include "External/ImGui/imgui_impl_dx11.h"
#include <tchar.h>
#include <iostream>

#include "gui.h"
#include "resource.h"
#include "Fonts.h"

using namespace GUI;

#pragma comment(lib, "d3d11.lib") 

// Data
//static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

float defaultFontSize = 13.0f;

static WNDCLASSEX wc;
static HWND hwnd;

static int (*mainGuiFunc)();

// Setup code, takes a function to run when doing GUI
HWND GUI::Setup(int (*OnGuiFunc)())
{
	if (OnGuiFunc != NULL)
		mainGuiFunc = OnGuiFunc;
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Latency Meter Refreshed"), NULL };
	wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	::RegisterClassEx(&wc);
	hwnd = ::CreateWindow(wc.lpszClassName, _T("Latency Meter Refreshed"), (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME), 100, 100, windowX, windowY, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return NULL;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.PopupRounding = 3.0f;
	style.GrabRounding = 4.0f;
	style.FrameBorderSize = 0.5f;

	ImVec4* colors = style.Colors;
	//colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1);
	//colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.36f, 0.36f, 1);
	//colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	//colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	//colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	//colors[ImGuiCol_Border] = ImVec4(0.293f, 0.293f, 0.293f, 1.000f);
	//colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1);
	//colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	//colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	//colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.24f, 0.24f, 1);
	//colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	//colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	//colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	//colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	//colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	//colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	//colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	//colors[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.56f, 0.94f, 1);
	//colors[ImGuiCol_SliderGrab] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	//colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.8f, 0.8f, 1);
	//colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1);
	//colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	//colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	//colors[ImGuiCol_Header] = ImVec4(0.556f, 0.143f, 0.212f, 1.0f);
	//colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.128f, 0.191f, 1.0f);
	//colors[ImGuiCol_HeaderActive] = ImVec4(0.445f, 0.114f, 0.170f, 1.0f);
	//colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	//colors[ImGuiCol_SeparatorHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	//colors[ImGuiCol_SeparatorActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	//colors[ImGuiCol_ResizeGrip] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	//colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	//colors[ImGuiCol_ResizeGripActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	//colors[ImGuiCol_PlotLines] = ImVec4(0.15f, 0.5f, 0.92f, 1);
	//colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.29f, 0.61f, 0.96f, 1);
	//colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.5f, 0.92f, 1);
	//colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.29f, 0.61f, 0.96f, 1);
	//colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);

	colors[ImGuiCol_Text] = ImVec4(0.88f, 0.88f, 0.88f, 1);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.36f, 0.36f, 1);
	colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 0.31f, 1);
	colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.18f, 1);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	colors[ImGuiCol_TitleBg] = ImVec4(0.24f, 0.24f, 0.24f, 1);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	colors[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.56f, 0.94f, 1);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8f, 0.8f, 0.8f, 1);
	colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.18f, 1);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1);
	colors[ImGuiCol_Header] = ImVec4(0.15f, 0.5f, 0.92f, 1);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.56f, 0.94f, 1);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.29f, 0.61f, 0.96f, 1);
	colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.3f, 0.3f, 0.3f, 1);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.48f, 0.48f, 0.48f, 1);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.6f, 0.6f, 0.6f, 1);
	colors[ImGuiCol_PlotLines] = ImVec4(0.15f, 0.5f, 0.92f, 1);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.29f, 0.61f, 0.96f, 1);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.5f, 0.92f, 1);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.29f, 0.61f, 0.96f, 1);


	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2f, 0.15f, 0.5, 0.92f);//ImGui::ColorConvertU32ToFloat4((BLUE400 & 0x00FFFFFF) | 0x33000000);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.69f, 1, 1, 1); //ImGui::ColorConvertU32ToFloat4((GRAY900 & 0x00FFFFFF) | 0x0A000000);


	// Handle switching to fullscreen on my own
	IDXGIDevice* pDXGIDevice = nullptr;
	auto hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);

	IDXGIAdapter* pDXGIAdapter = nullptr;
	hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);

	IDXGIFactory* pIDXGIFactory = nullptr;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);

	pIDXGIFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

	return hwnd;
}

void GUI::LoadFonts(float fontSizeMultiplier)
{
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig config;
	config.OversampleH = 2 * fontSizeMultiplier;
	config.OversampleV = 2 * fontSizeMultiplier;

	defaultFontSize += fontSizeMultiplier;

	//io.FontDefault = io.Fonts->AddFontFromFileTTF("../Fonts\\CourierPrime-Regular.ttf", defaultFontSize, &config);
	//io.Fonts->AddFontFromFileTTF("../Fonts\\CourierPrime-Bold.ttf", defaultFontSize, &config);
	io.Fonts->AddFontFromMemoryCompressedTTF(CourierPrimeRegular_compressed_data, 13, defaultFontSize, &config);
	io.Fonts->AddFontFromMemoryCompressedTTF(CourierPrimeBold_compressed_data, 13, defaultFontSize, &config);

	printf("Loaded Courier Prime\n");

	config.GlyphOffset.y = -1;
	//io.Fonts->AddFontFromFileTTF("../Fonts\\SourceSansPro-SemiBold.ttf", defaultFontSize, &config);
	//io.Fonts->AddFontFromFileTTF("../Fonts\\SourceSansPro-Black.ttf", defaultFontSize, &config);
	io.Fonts->AddFontFromMemoryCompressedTTF(SourceSansProSemiBold_compressed_data, 13, defaultFontSize, &config);
	io.Fonts->AddFontFromMemoryCompressedTTF(SourceSansProBlack_compressed_data, 13, defaultFontSize, &config);
	config.GlyphOffset.y = 0;

	//io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Gothicb.ttf", 14.f);
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Framd.ttf", defaultFontSize, &config);

	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Lucon.ttf", defaultFontSize, &config);

	io.Fonts->Build();

	//for (int i = 0; i < io.Fonts->Fonts.Size; i++)
	//{
	//    io.Fonts->Fonts[i]->Scale = 2.f;
	//}

}

int GUI::DrawGui() noexcept
{
	static bool showMainWindow = true;
	static ImVec4 clear_color = ImVec4(0.55f, 0.45f, 0.60f, 1.00f);
	static bool done = false;

	MSG msg;
	while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
		//if (msg.message == WM_QUIT)
		//    done = true;
	}
	if (done)
		return 1;

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	auto size = ImGui::GetIO().DisplaySize;
	//size = ImGui::GetWindowSize();
	ImGui::SetNextWindowSize({ (float)size.x, (float)size.y });
	ImGui::SetNextWindowPos({ 0,0 });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::Begin("Window", &showMainWindow, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();

	// Call the GUI function in main file
	if (int mainGui = mainGuiFunc())
		return mainGui;

	ImGui::End();

#ifdef _DEBUG 
	ImGui::ShowDemoWindow();
#endif // DEBUG

	// Rendering
	ImGui::Render();
	//const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
	//g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	auto presentRes = g_pSwapChain->Present(*VSyncFrame, 0);
	if (presentRes == DXGI_STATUS_OCCLUDED)
		Sleep(1);
	return 0;
}

void GUI::Destroy() noexcept
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0; // 60
	sd.BufferDesc.RefreshRate.Denominator = 0; // 1
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
		return false;

	IDXGIDevice* pDXGIDevice = nullptr;
	auto hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);

	IDXGIAdapter* pDXGIAdapter = nullptr;
	hr = pDXGIDevice->GetAdapter(&pDXGIAdapter);
	
	IDXGIFactory* pIDXGIFactory = nullptr;
	pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);
	
	// This will never find a Intel UHD gpu for some reason
	UINT iAdapter = 0;
	IDXGIAdapter* pAdapter;
	while (pIDXGIFactory->EnumAdapters(iAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		GUI::vAdapters.push_back(pAdapter);
		UINT iOutput = 0;
		IDXGIOutput* pOutput;

#ifdef _DEBUG
		DXGI_ADAPTER_DESC desc;
		pAdapter->GetDesc(&desc);
		wprintf(L"Current Adapter: %s\n", desc.Description);
#endif

		while (pAdapter->EnumOutputs(iOutput, &pOutput) != DXGI_ERROR_NOT_FOUND)
		{
			GUI::vOutputs.push_back(pOutput);

			UINT modesNum = 256;
			DXGI_MODE_DESC monitorModes[256];
			pOutput->GetDisplayModeList(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 0, &modesNum, monitorModes);

#ifdef _DEBUG
			for (UINT i = 0; i < modesNum; i++)
			{
				printf("Mode %i: %ix%i@%iHz\n", i, monitorModes[i].Width, monitorModes[i].Height, (monitorModes[modesNum - 1].RefreshRate.Numerator / monitorModes[modesNum - 1].RefreshRate.Denominator));
			}
#endif

			++iOutput;
		}

		++iAdapter;
	}

#ifdef _DEBUG
	DXGI_ADAPTER_DESC desc;
	pDXGIAdapter->GetDesc(&desc);

	wprintf(L"Selected Adapter: %s\n", desc.Description);
#endif

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	g_pSwapChain->SetFullscreenState(FALSE, NULL);
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (!pBackBuffer)
		ExitProcess(1);
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SETFOCUS:
		if (KeyDownFunc)
			KeyDownFunc(-1, -1, 0);
		break;
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			CleanupRenderTarget();
			//UINT x = (UINT)LOWORD(lParam), y = (UINT)HIWORD(lParam);
			g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = windowX;
		lpMMI->ptMinTrackSize.y = windowY;
		break;
	}
	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
		{
			if (onExitFunc)
				if (onExitFunc())
					break;
				else
					return 0;
		}
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		if (onExitFunc)
			if (onExitFunc())
			{
				::PostQuitMessage(0);
				return 0;
			}
		break;
	case WM_SYSKEYDOWN:
		if (KeyDownFunc)
			KeyDownFunc(wParam, lParam, true);
		break;
	case WM_SYSKEYUP:
		if (KeyDownFunc)
			KeyDownFunc(wParam, lParam, false);
		break;
	case WM_KEYDOWN:
		if (KeyDownFunc)
			KeyDownFunc(wParam, lParam, true);
		break;
	case WM_KEYUP:
		if (KeyDownFunc)
			KeyDownFunc(wParam, lParam, false);
		break;
	//case WM_INPUT:
	//{
	//	if (!RawInputEventFunc)
	//		break;
	//	UINT dwSize;
	//
	//	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	//	LPBYTE lpb = new BYTE[dwSize];
	//	if (lpb == NULL)
	//	{
	//		OutputDebugStringA("0");
	//		return 0;
	//	}
	//	OutputDebugStringA("1\n");
	//
	//	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
	//		OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
	//
	//	// request size of the raw input buffer to dwSize
	//	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
	//		sizeof(RAWINPUTHEADER));
	//
	//	// allocate buffer for input data
	//	auto buffer = (RAWINPUT*)HeapAlloc(GetProcessHeap(), 0, dwSize);
	//
	//	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &dwSize,
	//		sizeof(RAWINPUTHEADER)))
	//	{
	//		RawInputEventFunc(buffer);
	//	}
	//	HeapFree(GetProcessHeap(), 0, buffer);
	//	break;
	//}
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
