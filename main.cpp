#include <Windows.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>

using namespace std;

// DirectX SDK
#include <d3d11.h>
#include <d3dx11.h>
#include <D3DX11tex.h>
#include <d3dcompiler.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dcompiler.lib")

// Global Object Declarations
IDXGISwapChain*                     dx_Swapchain; 
DXGI_SWAP_CHAIN_DESC                dx_Swapchain_Desc;
ID3D11Device*                       dx_Device;           
ID3D11DeviceContext*                dx_Context; 
ID3D11RenderTargetView*             dx_BackBuffer;

ID3D11InputLayout*                  dx_Layout;
ID3D11VertexShader*                 dx_DefaultVS;
ID3D11PixelShader*                  dx_DefaultPS;
ID3D11Buffer*                       dx_VertexBuffer = 0;
D3D11_BUFFER_DESC                   dx_VertexBuffer_Desc;
ID3D11RasterizerState*              dx_rasterState;

ID3D11Texture2D*                    colorBufferTex;
ID3D11ShaderResourceView*           colorBufferSRV;
ID3D11Texture2D*                    depthBufferTex;
ID3D11ShaderResourceView*           depthBufferSRV;

// Global Value Declarations
HWND winHwnd = 0;  HRESULT hres = 0;
bool initialized = false;
RECT wr = { 0, 0, 512, 512 };
float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black

// Types & Structs
struct VertexData
{
    float x, y, z;
    unsigned int color;
};

// Vertex Data
const int triangleCount = 2;
VertexData quad_verts[] =
{
    // Triangle 1
    { -1.0f, -1.0f, 0, 0xFFff0000 },
    { -1.0f, 1.0f,  0, 0xFF00ff00 },
    { 1.0f,  1.0f,  0, 0xFF0000ff },

    // Triangle 2
    { -1.0f, -1.0f, 0, 0xFFffff00 },
    { 1.0f,  1.0f,  0, 0xFF00ffff },
    { 1.0f, -1.0f,  0, 0xFFff00ff }
};

// Function Prototypes
void InitD3D(HWND hWnd);
void RenderFrame();
void ResetD3D(HWND hWnd);
void ReleaseD3D();

// Configs
#define DXGI_DEFAULT_OUTPUT DXGI_FORMAT_R8G8B8A8_UNORM

// Macros
#define log(fmt,...)    printf(fmt "\n",__VA_ARGS__);
#define SHADERS_PATH    "./Shaders/"
#define TEXTURES_PATH   "./Textures/"
#define WINDOW_CLASS    L"DXWindow"
#define CONSOLE_WAIT    10

// Win32 Window
BOOL CenterWindow(HWND hwndWindow)
{
    HWND hwndParent; RECT rectWindow, rectParent;
    if ((hwndParent = GetDesktopWindow()) != NULL)
    {
        GetWindowRect(hwndWindow, &rectWindow);
        GetWindowRect(hwndParent, &rectParent);

        int nWidth = rectWindow.right - rectWindow.left;
        int nHeight = rectWindow.bottom - rectWindow.top;

        int nX = ((rectParent.right - rectParent.left) - nWidth) / 2 + rectParent.left;
        int nY = ((rectParent.bottom - rectParent.top) - nHeight) / 2 + rectParent.top;

        int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        if (nX < 0) nX = 0;
        if (nY < 0) nY = 0;
        if (nX + nWidth > nScreenWidth) nX = nScreenWidth - nWidth;
        if (nY + nHeight > nScreenHeight) nY = nScreenHeight - nHeight;

        MoveWindow(hwndWindow, nX, nY, nWidth, nHeight, FALSE);

        return TRUE;
    }
    return FALSE;
}
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (initialized) 
        {
            ResetD3D(hWnd);
            RenderFrame();
        }
        break;
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    } break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
HWND CreateAppWindow() 
{
    HWND hWnd;
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));
    RegisterClassEx(&wc);
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE); 
    hWnd = CreateWindowEx(NULL,
        WINDOW_CLASS,         
        L"DirectX 11 :: Render View",       
        WS_OVERLAPPEDWINDOW, 0, 0,
        wr.right - wr.left,
        wr.bottom - wr.top,
        NULL, NULL, wc.hInstance, NULL);
    CenterWindow(hWnd);
    ShowWindow(hWnd, SW_NORMAL);
    return hWnd;
}

// Entrypoint
int main() 
{
    // Config Console
    SetConsoleTitle(L"Basic DirectX11 Application");
    log("Initializing..."); Sleep(CONSOLE_WAIT);

    // Create App Window
    winHwnd = CreateAppWindow();

    // Initialize Direct3D
    InitD3D(winHwnd);

    // Message Loop
    log("Rendering...");
    MSG msg = { 0 };
    while(TRUE)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if(msg.message == WM_QUIT) break;
        }
        else
        {
            RenderFrame();
        }
    }

    log("Closing...");
    ReleaseD3D(); Sleep(CONSOLE_WAIT); return 0;
}

// DirectX Functions
void InitD3D(HWND hWnd)
{
    // Feature Level
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    // Setup Device, Swapchain & Viewport
    ZeroMemory(&dx_Swapchain_Desc, sizeof(DXGI_SWAP_CHAIN_DESC));
    dx_Swapchain_Desc.BufferCount = 1;
    dx_Swapchain_Desc.BufferDesc.Format = DXGI_DEFAULT_OUTPUT;
    dx_Swapchain_Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dx_Swapchain_Desc.OutputWindow = hWnd;
    dx_Swapchain_Desc.SampleDesc.Count = 1;
    dx_Swapchain_Desc.SampleDesc.Quality = 0;
    dx_Swapchain_Desc.Windowed = TRUE;
    hres = 
        D3D11CreateDeviceAndSwapChain(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        NULL,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &dx_Swapchain_Desc,
        &dx_Swapchain,
        &dx_Device,
        NULL,
        &dx_Context);
    if (hres == 0x887a0004) { MessageBoxA(winHwnd, "DirectX11, Shader Model 5.0 is Required!", "Error while Creating Device", MB_ICONERROR); exit(-12); }
    if (FAILED(hres)) { MessageBoxA(winHwnd, "Failed to Create Device, Failed with Unkown Error!", "Error while Creating Device", MB_ICONERROR); exit(-12); }

    // Create Render Buffer
    ID3D11Texture2D* pBackBuffer;
    dx_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    dx_Device->CreateRenderTargetView(pBackBuffer, NULL, &dx_BackBuffer);
    pBackBuffer->Release();
    RECT ViewportRect;
    GetClientRect(hWnd, &ViewportRect);
    const D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<FLOAT>(ViewportRect.right - ViewportRect.left),
                                    static_cast<FLOAT>(ViewportRect.bottom - ViewportRect.top), 0.0f, 1.0f };
    dx_Context->RSSetViewports(1, &viewport);

    // Create Rasterizer
    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = true;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    hres = dx_Device->CreateRasterizerState(&rasterDesc, &dx_rasterState);
    if (FAILED(hres)) { MessageBoxA(winHwnd, "Can't Create Rasterizer!", "Error while Creating RS!", MB_ICONERROR); }
    dx_Context->RSSetState(dx_rasterState);

    // Compile Shaders & Geometries
    memset(&dx_VertexBuffer_Desc, 0, sizeof(dx_VertexBuffer_Desc));
    dx_VertexBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
    dx_VertexBuffer_Desc.ByteWidth = 256;
    dx_VertexBuffer_Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    dx_Device->CreateBuffer(&dx_VertexBuffer_Desc, NULL, &dx_VertexBuffer);
    ID3D10Blob* VS, * PS, * Error;
    hres = D3DX11CompileFromFileW(SHADERS_PATH L"Default.hlsl", 0, 0, "VS_MAIN", "vs_5_0", 0, 0, 0, &VS, &Error, 0);
    if (FAILED(hres)) { MessageBoxA(winHwnd, (char*)Error->GetBufferPointer(), "VS Shader Failed to Compile!", MB_ICONERROR); }
    hres = D3DX11CompileFromFileW(SHADERS_PATH L"Default.hlsl", 0, 0, "PS_MAIN", "ps_5_0", 0, 0, 0, &PS, &Error, 0);
    if (FAILED(hres)) { MessageBoxA(winHwnd, (char*)Error->GetBufferPointer(), "PS Shader Failed to Compile!", MB_ICONERROR); }
    hres = dx_Device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &dx_DefaultVS);  if (FAILED(hres)) exit(-50);
    hres = dx_Device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &dx_DefaultPS);  if (FAILED(hres)) exit(-40);
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "DIFFUSE", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    dx_Device->CreateInputLayout(ied, 3, VS->GetBufferPointer(), VS->GetBufferSize(), &dx_Layout);
    dx_Context->IASetInputLayout(dx_Layout);

    // Load Pre-Rendered Buffers
    D3DX11_IMAGE_LOAD_INFO loadInfo = {};
    loadInfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    loadInfo.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    loadInfo.CpuAccessFlags = 0;
    loadInfo.Filter = D3DX11_FILTER_SRGB;
    loadInfo.Usage = D3D11_USAGE_DEFAULT;
    loadInfo.MipLevels = 1; loadInfo.MiscFlags = 0;
    hres = D3DX11CreateTextureFromFileW(dx_Device, TEXTURES_PATH L"ColorBuffer.png", &loadInfo, nullptr, (ID3D11Resource**)&colorBufferTex, nullptr);
    if (FAILED(hres)) { MessageBoxA(winHwnd, "Can't Load Color Buffer Texture!", "Error while Importing Texture!", MB_ICONERROR); }
    if (colorBufferTex != nullptr)
    {
        D3D11_TEXTURE2D_DESC desc;
        colorBufferTex->GetDesc(&desc);
        hres = dx_Device->CreateShaderResourceView(colorBufferTex, nullptr, &colorBufferSRV);
        if (FAILED(hres)) { MessageBoxA(winHwnd, "Can't Make Color Buffer SRV!", "Error while creating SRV!", MB_ICONERROR); }
    }
    hres = D3DX11CreateTextureFromFileW(dx_Device, TEXTURES_PATH L"DepthBuffer.png", &loadInfo, nullptr, (ID3D11Resource**)&depthBufferTex, nullptr);
    if (FAILED(hres)) { MessageBoxA(winHwnd, "Can't Load Depth Buffer Texture!", "Error while Importing Texture!", MB_ICONERROR); }
    if (depthBufferTex != nullptr)
    {
        D3D11_TEXTURE2D_DESC desc;
        depthBufferTex->GetDesc(&desc);
        hres = dx_Device->CreateShaderResourceView(depthBufferTex, nullptr, &depthBufferSRV);
        if (FAILED(hres)) { MessageBoxA(winHwnd, "Can't Make Depth Buffer SRV!", "Error while creating SRV!", MB_ICONERROR); }
    }

    initialized = true;
}
void RenderFrame()
{
    // Render Shader
    dx_Context->OMSetRenderTargets(1, &dx_BackBuffer, NULL);
    dx_Context->ClearRenderTargetView(dx_BackBuffer, clear_color);

    // Set shaders
    dx_Context->VSSetShader(dx_DefaultVS, NULL, 0);
    dx_Context->PSSetShader(dx_DefaultPS, NULL, 0);

    // Set Textures
    dx_Context->PSSetShaderResources(0, 1, &colorBufferSRV);
    dx_Context->PSSetShaderResources(1, 1, &depthBufferSRV);

    // Update Vertex Buffer
    const int vertSize = 12 + 4;
    dx_Context->UpdateSubresource(dx_VertexBuffer, 0, NULL, quad_verts, triangleCount * 3 * vertSize, 0);

    // Set Input Assembler Data
    dx_Context->IASetInputLayout(dx_Layout);
    dx_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = vertSize; UINT offset = 0;
    dx_Context->IASetVertexBuffers(0, 1, &dx_VertexBuffer, &stride, &offset);

    // Draw & Render
    dx_Context->Draw(triangleCount * 3, 0);

    // Present Render
    dx_Swapchain->Present(1, 0 );
}
void ResetD3D(HWND hWnd)
{
    RECT ViewportRect; 
    GetClientRect(hWnd, &ViewportRect);
    dx_BackBuffer->Release();
    dx_Context->OMSetRenderTargets(0, NULL, NULL);
    dx_Context->Flush();
    hres = dx_Swapchain->ResizeBuffers(
        dx_Swapchain_Desc.BufferCount,
        ViewportRect.right - ViewportRect.left,
        ViewportRect.bottom - ViewportRect.top,
        DXGI_DEFAULT_OUTPUT, dx_Swapchain_Desc.Flags);
    if (FAILED(hres)) { log("Buffer Size Update Failed,  Error : %x", hres); }

    D3D11_TEXTURE2D_DESC desc = {};
    ID3D11Texture2D* pBackBuffer;
    dx_Swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    pBackBuffer->GetDesc(&desc);
    // log("Back Buffer Width : %d | Heigh : %d", desc.Width, desc.Height);
    dx_Device->CreateRenderTargetView(pBackBuffer, NULL, &dx_BackBuffer);
    pBackBuffer->Release();

    const D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<FLOAT>(desc.Width), static_cast<FLOAT>(desc.Height), 0.0f, 1.0f };
    dx_Context->RSSetViewports(1, &viewport);
}
void ReleaseD3D()
{
    colorBufferTex->Release();
    colorBufferSRV->Release();
    depthBufferTex->Release();
    depthBufferSRV->Release();
    dx_Layout->Release();
    dx_DefaultVS->Release();
    dx_DefaultPS->Release();
    dx_VertexBuffer->Release();
    dx_rasterState->Release();
    dx_Swapchain->Release();
    dx_BackBuffer->Release();
    dx_Device->Release();
    dx_Context->Release();
}