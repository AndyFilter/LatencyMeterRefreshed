#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_impl_win32.h"
#include "External/ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>

#include "gui.h"

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static int windowX = 1280, windowY = 720;

const float defaultFontSize = 13.0f;

static WNDCLASSEX wc;
static HWND hwnd;

static int (*mainGuiFunc)();

// Setup code, takes a function to run when doing GUI
void GUI::Setup(int (*OnGuiFunc)() = NULL)
{
    if (OnGuiFunc != NULL)
        mainGuiFunc = OnGuiFunc;
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("LatencyMeterRefreshed"), NULL };
    ::RegisterClassEx(&wc);
    hwnd = ::CreateWindow(wc.lpszClassName, _T("Latency Meter Refreshed"), (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX), 100, 100, windowX, windowY, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;

    io.FontDefault =  io.Fonts->AddFontFromFileTTF("../Fonts\\CourierPrime-Regular.ttf", defaultFontSize, &config);
    io.Fonts->AddFontFromFileTTF("../Fonts\\CourierPrime-Bold.ttf", defaultFontSize, &config);

    config.GlyphOffset.y = -1;
    io.Fonts->AddFontFromFileTTF("../Fonts\\SourceSansPro-SemiBold.ttf", defaultFontSize, &config);
    io.Fonts->AddFontFromFileTTF("../Fonts\\SourceSansPro-Bold.ttf", defaultFontSize, &config);
    config.GlyphOffset.y = 0;

    //io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Gothicb.ttf", 14.f);
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Framd.ttf", defaultFontSize, &config);

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Lucon.ttf", defaultFontSize, &config);

    io.Fonts->Build();

    //for (int i = 0; i < io.Fonts->Fonts.Size; i++)
    //{
    //    io.Fonts->Fonts[i]->Scale = 2.f;
    //}

    //io.DeltaTime = 200;

    //ImGuiStyle& style = ImGui::GetStyle();
    //style.Colors[ImGuiCol_Text] = ImVec4(0.31f, 0.25f, 0.24f, 1.00f);
    //style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    //style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.74f, 0.74f, 0.94f, 1.00f);
    //style.Colors[ImGuiCol_ChildBg] = ImVec4(0.68f, 0.68f, 0.68f, 0.00f);
    //style.Colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
    //style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    //style.Colors[ImGuiCol_FrameBg] = ImVec4(0.62f, 0.70f, 0.72f, 0.56f);
    //style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.95f, 0.33f, 0.14f, 0.47f);
    //style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.97f, 0.31f, 0.13f, 0.81f);
    //style.Colors[ImGuiCol_TitleBg] = ImVec4(0.42f, 0.75f, 1.00f, 0.53f);
    //style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.65f, 0.80f, 0.20f);
    //style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.40f, 0.62f, 0.80f, 0.15f);
    //style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.39f, 0.64f, 0.80f, 0.30f);
    //style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.67f, 0.80f, 0.59f);
    //style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.48f, 0.53f, 0.67f);
    //style.Colors[ImGuiCol_CheckMark] = ImVec4(0.48f, 0.47f, 0.47f, 0.71f);
    //style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.31f, 0.47f, 0.99f, 1.00f);
    //style.Colors[ImGuiCol_Button] = ImVec4(1.00f, 0.79f, 0.18f, 0.78f);
    //style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.42f, 0.82f, 1.00f, 0.81f);
    //style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.72f, 1.00f, 1.00f, 0.86f);
    //style.Colors[ImGuiCol_Header] = ImVec4(0.65f, 0.78f, 0.84f, 0.80f);
    //style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.88f, 0.94f, 0.80f);
    //style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.68f, 0.74f, 0.80f);//ImVec4(0.46f, 0.84f, 0.90f, 1.00f);
    //style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.60f, 0.60f, 0.80f, 0.30f);
    //style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    //style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    ////style.Colors[ImGuiCol_Button] = ImVec4(0.41f, 0.75f, 0.98f, 0.50f);
    ////style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(1.00f, 0.47f, 0.41f, 0.60f);
    ////style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(1.00f, 0.16f, 0.00f, 1.00f);
    //style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.99f, 0.54f, 0.43f);
    ////style.Colors[ImGuiCol_tooltip] = ImVec4(0.82f, 0.92f, 1.00f, 0.90f);
    //style.Alpha = 1.0f;
    ////style.WindowFillAlphaDefault = 1.0f;
    //style.FrameRounding = 4;
    //style.GrabRounding = 4;
    //style.IndentSpacing = 12.0f;

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

    colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.36f, 0.36f, 1);
    colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 1);
    colors[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1);
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


    // Our state
    //bool show_demo_window = true;
    //bool show_another_window = false;
    bool showMainWindow = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    return;
}
long lastFrame = 0;
int GUI::DrawGui() noexcept
{
    //if ((clock() - lastFrame) < 11)
    //    return 0;
    static bool showMainWindow = true;
    static ImVec4 clear_color = ImVec4(0.55f, 0.45f, 0.60f, 1.00f);
    static bool done = false;

    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            done = true;
    }
    if (done)
        return 1;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();


    ImGui::SetNextWindowSize({ (float)windowX - ImGui::GetStyle().WindowPadding.x * 2, (float)windowY});
    ImGui::SetNextWindowPos({ 0,0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::Begin("Window", &showMainWindow, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    ImGui::PopStyleVar();

    // Call the GUI function in main file
    if (int maninGui = mainGuiFunc())
        return maninGui;

    ImGui::End();

    ImGui::ShowDemoWindow();

    // Rendering
    ImGui::Render();
    const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    lastFrame = clock();
    g_pSwapChain->Present(0, 0);
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
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
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
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
