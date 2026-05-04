#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <codecvt>
#include <locale>

// 前向声明 ImGui Win32 窗口过程处理函数
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImWidgetV4 {

// ========== 构造函数和析构函数 ==========

ImWin32DX11Backend::ImWin32DX11Backend(
    const std::wstring& windowTitle,
    int width,
    int height)
    : HInstance_(GetModuleHandle(nullptr))
    , Hwnd_(nullptr)
    , WindowTitle_(windowTitle)
    , WindowWidth_(width)
    , WindowHeight_(height)
    , bShouldClose_(false)
    , D3DDevice_(nullptr)
    , D3DDeviceContext_(nullptr)
    , SwapChain_(nullptr)
    , MainRenderTargetView_(nullptr)
    , ResizeWidth_(0)
    , ResizeHeight_(0)
    , bSwapChainOccluded_(false)
    , Application_(nullptr)
{
}

ImWin32DX11Backend::~ImWin32DX11Backend() {
    Shutdown();
}

// ========== ImApplicationBackend 接口实现 ==========

bool ImWin32DX11Backend::Initialize() {
    // 1. 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = HInstance_;
    wc.lpszClassName = L"ImWidgetV4WindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    // 2. 创建窗口
    Hwnd_ = CreateWindowW(
        wc.lpszClassName,
        WindowTitle_.c_str(),
        WS_OVERLAPPEDWINDOW,
        100, 100,
        WindowWidth_, WindowHeight_,
        nullptr, nullptr,
        wc.hInstance,
        this  // 传递 this 指针
    );

    if (!Hwnd_) {
        return false;
    }

    // 3. 初始化 DirectX 11
    if (!CreateDeviceD3D()) {
        return false;
    }

    // 4. 显示窗口
    ShowWindow(Hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(Hwnd_);

    // 5. 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // 6. 设置 ImGui 样式
    ImGui::StyleColorsDark();

    // 7. 初始化 ImGui 后端
    ImGui_ImplWin32_Init(Hwnd_);
    ImGui_ImplDX11_Init(D3DDevice_, D3DDeviceContext_);

    return true;
}

void ImWin32DX11Backend::Shutdown() {
    // 1. 清理 ImGui
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // 2. 清理 DirectX 11
    CleanupDeviceD3D();

    // 3. 销毁窗口
    if (Hwnd_) {
        DestroyWindow(Hwnd_);
        Hwnd_ = nullptr;
    }

    // 4. 注销窗口类
    UnregisterClassW(L"ImWidgetV4WindowClass", HInstance_);
}

void ImWin32DX11Backend::Run() {
    MSG msg = {};

    while (!bShouldClose_) {
        // 1. 处理 Windows 消息
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                bShouldClose_ = true;
            }
        }

        if (bShouldClose_) {
            break;
        }

        // 2. 处理窗口遮挡
        if (bSwapChainOccluded_ &&
            SwapChain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        bSwapChainOccluded_ = false;

        // 3. 处理窗口大小调整
        HandleResize();

        // 4. 开始新帧
        BeginFrame();

        // 5. 调用 Application 的 AdvanceFrame
        FFrameInfo frameInfo;
        bool bHasFrameInfo = false;
        if (Application_) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGuiIO& io = ImGui::GetIO();
            FImGuiInputSnapshot inputSnapshot;
            PopulateImGuiInputSnapshotFromIo(io, inputSnapshot);
            InputSource_.SetSnapshot(inputSnapshot);
            DrawContext drawContext(ImGui::GetBackgroundDrawList());

            FFrameContext frameContext;
            frameContext.FrameInfo.ViewportPosition =
                FVector2(viewport->Pos.x, viewport->Pos.y);
            frameContext.FrameInfo.ViewportSize =
                FVector2(viewport->Size.x, viewport->Size.y);
            frameContext.FrameInfo.DeltaTime = io.DeltaTime;
            frameContext.FrameInfo.CurrentTime = ImGui::GetTime();
            frameContext.DrawContext_ = &drawContext;
            frameContext.InputSource = &InputSource_;

            frameInfo = frameContext.FrameInfo;
            bHasFrameInfo = true;
            Application_->AdvanceFrame(frameContext);
        }

        // 6. 结束帧
        EndFrame();

        if (bHasFrameInfo && PostFrameCallback_) {
            PostFrameCallback_(frameInfo);
        }
    }
}

bool ImWin32DX11Backend::ShouldClose() const {
    return bShouldClose_;
}

void ImWin32DX11Backend::SetWindowTitle(const std::string& title) {
    // 将 UTF-8 转换为宽字符
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    WindowTitle_ = converter.from_bytes(title);

    if (Hwnd_) {
        SetWindowTextW(Hwnd_, WindowTitle_.c_str());
    }
}

void ImWin32DX11Backend::SetWindowSize(int width, int height) {
    WindowWidth_ = width;
    WindowHeight_ = height;

    if (Hwnd_) {
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(Hwnd_, nullptr, 0, 0,
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }
}

void ImWin32DX11Backend::GetWindowSize(int& width, int& height) const {
    width = WindowWidth_;
    height = WindowHeight_;
}

void ImWin32DX11Backend::BeginFrame() {
    // 1. 开始 ImGui 新帧
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImWin32DX11Backend::EndFrame() {
    // 1. 渲染 ImGui
    ImGui::Render();

    // 2. 清空渲染目标
    const float clearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};
    D3DDeviceContext_->OMSetRenderTargets(1, &MainRenderTargetView_, nullptr);
    D3DDeviceContext_->ClearRenderTargetView(MainRenderTargetView_, clearColor);

    // 3. 渲染 ImGui 绘制数据
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // 4. 呈现
    HRESULT hr = SwapChain_->Present(1, 0);
    bSwapChainOccluded_ = (hr == DXGI_STATUS_OCCLUDED);
}

void ImWin32DX11Backend::SetApplication(ImApplication* app) {
    Application_ = app;
    if (Application_ != nullptr) {
        Application_->EnsureDefaultFontConfigured();
    }
}

ImApplication* ImWin32DX11Backend::GetApplication() const {
    return Application_;
}

void ImWin32DX11Backend::SetPostFrameCallback(FPostFrameCallback callback) {
    PostFrameCallback_ = std::move(callback);
}

void ImWin32DX11Backend::ClearPostFrameCallback() {
    PostFrameCallback_ = nullptr;
}

void ImWin32DX11Backend::RequestClose() {
    bShouldClose_ = true;
}

std::string ImWin32DX11Backend::GetBackendName() const {
    return "Win32 + DirectX 11";
}

// ========== DirectX 11 设备管理 ==========

bool ImWin32DX11Backend::CreateDeviceD3D() {
    // 1. 配置交换链
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = Hwnd_;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // 2. 创建设备和交换链
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &SwapChain_,
        &D3DDevice_,
        &featureLevel,
        &D3DDeviceContext_
    );

    // 3. 尝试 WARP 软件渲染器
    if (hr == DXGI_ERROR_UNSUPPORTED) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &SwapChain_,
            &D3DDevice_,
            &featureLevel,
            &D3DDeviceContext_
        );
    }

    if (FAILED(hr)) {
        return false;
    }

    // 4. 创建渲染目标
    CreateRenderTarget();
    return true;
}

void ImWin32DX11Backend::CleanupDeviceD3D() {
    CleanupRenderTarget();

    if (SwapChain_) {
        SwapChain_->Release();
        SwapChain_ = nullptr;
    }

    if (D3DDeviceContext_) {
        D3DDeviceContext_->Release();
        D3DDeviceContext_ = nullptr;
    }

    if (D3DDevice_) {
        D3DDevice_->Release();
        D3DDevice_ = nullptr;
    }
}

void ImWin32DX11Backend::CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    SwapChain_->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    if (pBackBuffer) {
        D3DDevice_->CreateRenderTargetView(pBackBuffer, nullptr, &MainRenderTargetView_);
        pBackBuffer->Release();
    }
}

void ImWin32DX11Backend::CleanupRenderTarget() {
    if (MainRenderTargetView_) {
        MainRenderTargetView_->Release();
        MainRenderTargetView_ = nullptr;
    }
}

void ImWin32DX11Backend::HandleResize() {
    if (ResizeWidth_ != 0 && ResizeHeight_ != 0) {
        CleanupRenderTarget();
        SwapChain_->ResizeBuffers(0, ResizeWidth_, ResizeHeight_, DXGI_FORMAT_UNKNOWN, 0);
        ResizeWidth_ = 0;
        ResizeHeight_ = 0;
        CreateRenderTarget();
    }
}

// ========== 窗口过程 ==========

LRESULT CALLBACK ImWin32DX11Backend::WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 获取 this 指针
    ImWin32DX11Backend* backend = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        backend = reinterpret_cast<ImWin32DX11Backend*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(backend));
    } else {
        backend = reinterpret_cast<ImWin32DX11Backend*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (backend) {
        return backend->WndProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT ImWin32DX11Backend::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 1. 转发给 ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    // 2. 处理窗口消息
    switch (msg) {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            return 0;
        }
        ResizeWidth_ = LOWORD(lParam);
        ResizeHeight_ = HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) {
            return 0;  // 禁用 ALT 菜单
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace ImWidgetV4
