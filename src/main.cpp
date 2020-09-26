// =============================================================================
//                                  INCLUDES
// =============================================================================
#include <Windows.h>
#include <wingdi.h>
#include <random>
#include <stdio.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <application.h>
#include <wchar.h>
#include <iostream>
#include <thread>
#include "resource.h"
#include <fmt/core.h>
#include <fmt/chrono.h>

AbstractApplication::~AbstractApplication() = default;

static HDC     g_HDCDeviceContext = nullptr;
static HGLRC   g_GLRenderContext = nullptr;
static HGLRC   g_GLImageLoadingContext = nullptr;
static HWND    g_hwnd;
static int     g_display_w = 800; 
static int     g_display_h = 600;
static ImVec4  clear_color;

void CreateGlContext();

void SetCurrentContext();

LRESULT WINAPI WndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);
    
bool Init(
    HINSTANCE hInstance);
    
void Cleanup(
    HINSTANCE hInstance);
    
extern AbstractApplication* CreateApplication();

static AbstractApplication *app = nullptr;

 int main(
     int argc,
     char *argv[])
 {  
    std::filesystem::path imageFile = std::filesystem::path();
    
    for (int i = 1; i < argc; i++)
    {
        std::cout << "" << argv[i] << std::endl;
        auto file = std::filesystem::path(argv[i]);
        if (std::filesystem::exists(file))
        {
            imageFile = file;
            
            break;
        }
    }
    
    if (argc <= 1)
    {
        BITMAPFILEHEADER bfHeader;
        BITMAPINFOHEADER biHeader;
        BITMAPINFO bInfo;
        HGDIOBJ hTempBitmap;
        HBITMAP hBitmap;
        BITMAP bAllDesktops;
        HDC hDC, hMemDC;
        LONG lWidth, lHeight;
        BYTE *bBits = NULL;
        HANDLE hHeap = GetProcessHeap();
        DWORD cbBits, dwWritten = 0;
        HANDLE hFile;
        INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);

        ZeroMemory(&bfHeader, sizeof(BITMAPFILEHEADER));
        ZeroMemory(&biHeader, sizeof(BITMAPINFOHEADER));
        ZeroMemory(&bInfo, sizeof(BITMAPINFO));
        ZeroMemory(&bAllDesktops, sizeof(BITMAP));

        hDC = GetDC(NULL);
        hTempBitmap = GetCurrentObject(hDC, OBJ_BITMAP);
        GetObjectW(hTempBitmap, sizeof(BITMAP), &bAllDesktops);

        lWidth = bAllDesktops.bmWidth;
        lHeight = bAllDesktops.bmHeight;

        DeleteObject(hTempBitmap);

        bfHeader.bfType = (WORD)('B' | ('M' << 8));
        bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        biHeader.biSize = sizeof(BITMAPINFOHEADER);
        biHeader.biBitCount = 24;
        biHeader.biCompression = BI_RGB;
        biHeader.biPlanes = 1;
        biHeader.biWidth = lWidth;
        biHeader.biHeight = lHeight;

        bInfo.bmiHeader = biHeader;

        cbBits = (((24 * lWidth + 31)&~31) / 8) * lHeight;

        hMemDC = CreateCompatibleDC(hDC);
        hBitmap = CreateDIBSection(hDC, &bInfo, DIB_RGB_COLORS, (VOID **)&bBits, NULL, 0);
        SelectObject(hMemDC, hBitmap);
        BitBlt(hMemDC, 0, 0, lWidth, lHeight, hDC, x, y, SRCCOPY);

        std::filesystem::create_directories("c:\\temp\\screenshots");

        imageFile = std::filesystem::path(
            fmt::format("c:\\temp\\screenshots\\screenshot_[{:%Y-%m-%d_%H.%M.%S}].bmp", std::chrono::system_clock::now()));
        
        hFile = CreateFileW(imageFile.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(hFile, &bfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
        WriteFile(hFile, &biHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
        WriteFile(hFile, bBits, cbBits, &dwWritten, NULL);

        CloseHandle(hFile);

        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        DeleteObject(hBitmap);
    }
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
    
    app = CreateApplication();
    
    if (!Init(hInstance))
    {
        return 1;
    }
    
    if (!app->Setup(imageFile))
    {
        Cleanup(
            hInstance);
            
        return 1;
    }
    
    // Main loop
    MSG msg;
    ZeroMemory(
        &msg,
        sizeof(msg));
    
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        wglMakeCurrent(
            g_HDCDeviceContext,
            g_GLRenderContext);
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        app->Render2d();
    
        // Rendering
        ImGui::Render();
        
        wglMakeCurrent(
            g_HDCDeviceContext,
            g_GLRenderContext);
        
        glViewport(
            0,
            0,
            g_display_w,
            g_display_h);
        
        glClearColor(
            clear_color.x,
            clear_color.y,
            clear_color.z,
            clear_color.w);
        
        glClear(
            GL_COLOR_BUFFER_BIT);
        
        app->Render3d();
    
        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData());
        
        SwapBuffers(
            g_HDCDeviceContext);
    }

    Cleanup(
        hInstance);

    return 0;
}

bool Init(
    HINSTANCE hInstance)
{
    WNDCLASS wc;
    
    ZeroMemory(
        &wc,
        sizeof(wc));
        
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINWND));
    wc.lpszClassName = "IMGUI";
    wc.style = CS_OWNDC;
    
    if(!RegisterClass(&wc))
    {
        return false;
    }
    
    g_hwnd = CreateWindow(
        wc.lpszClassName,
        "image-viewer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        hInstance,
        0);

    // Show the window
    ShowWindow(
        g_hwnd,
        SW_SHOWDEFAULT);
    
    UpdateWindow(
        g_hwnd);

    //Prepare OpenGlContext
    CreateGlContext();

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    //Init Win32
    ImGui_ImplWin32_Init(
        g_hwnd);
            
    //Init OpenGL Imgui Implementation
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(
        glsl_version);

    //Set Window bg color
    clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Setup style
    ImGui::StyleColorsDark();
    
    return true;
}

void Cleanup(
    HINSTANCE hInstance)
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    
    wglDeleteContext(
        g_GLRenderContext);
        
    ImGui::DestroyContext();
    
    ImGui_ImplWin32_Shutdown();

    DestroyWindow(g_hwnd);
    
    UnregisterClass(
        "IMGUI",
        hInstance);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
        case WM_SIZE:
        {
            if (wParam != SIZE_MINIMIZED)
            {
                g_display_w = (UINT)LOWORD(lParam);
                g_display_h = (UINT)HIWORD(lParam);
                
                if (app != nullptr)
                {
                    app->OnResize(g_display_w, g_display_h);
                }
            }
            return 0;
        }
        case WM_SYSCOMMAND:
        {
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            {
                return 0;
            }
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYUP:
        {
            if (wParam == VK_LEFT || wParam == VK_DOWN || wParam == VK_PRIOR)
            {
                app->OnPreviousPressed();
                return 0;
            }
            if (wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_NEXT)
            {
                app->OnNextPressed();
                return 0;
            }
            if (wParam == VK_HOME)
            {
                app->OnHomePressed();
                return 0;
            }
            if (wParam == VK_END)
            {
                app->OnEndPressed();
                return 0;
            }
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            app->OnZoom((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
            return 0;
        }
    }
    return DefWindowProc(
        hWnd,
        msg,
        wParam,
        lParam);
}

void CreateGlContext(){

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    g_HDCDeviceContext = GetDC(
        g_hwnd);
    
    int pixelFormat = ChoosePixelFormat(
        g_HDCDeviceContext,
        &pfd);
    
    SetPixelFormat(
        g_HDCDeviceContext,
        pixelFormat,
        &pfd);
    
    g_GLRenderContext = wglCreateContext(
        g_HDCDeviceContext);
    
    g_GLImageLoadingContext = wglCreateContext(
        g_HDCDeviceContext);
    
    BOOL error = wglShareLists(g_GLRenderContext, g_GLImageLoadingContext);

    if(error == FALSE)
    {
        std::cerr << "failed to share render contexts\n";
    }
    
    wglMakeCurrent(
        g_HDCDeviceContext,
        g_GLRenderContext);

    gladLoadGL();
}

void AbstractApplication::ActivateLoadingContext()
{
    wglMakeCurrent(
        g_HDCDeviceContext,
        g_GLImageLoadingContext);
}

void AbstractApplication::DeactivateLoadingContext()
{
    wglMakeCurrent(
        g_HDCDeviceContext,
        nullptr);
}