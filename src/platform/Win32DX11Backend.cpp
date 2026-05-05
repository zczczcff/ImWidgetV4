#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <algorithm>
#include <cmath>
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
    , bWindowClassRegistered_(false)
    , bImGuiBackendInitialized_(false)
    , bImGuiContextOwned_(false)
    , bUseCustomHostChrome_(false)
    , HoveredHostChromeButton_(EHostChromeButton::None)
    , PressedHostChromeButton_(EHostChromeButton::None)
    , Application_(nullptr)
{
}

ImWin32DX11Backend::~ImWin32DX11Backend() {
    Shutdown();
}

// ========== ImApplicationBackend 接口实现 ==========

bool ImWin32DX11Backend::Initialize() {
    if (bWindowClassRegistered_ || Hwnd_ != nullptr || bImGuiBackendInitialized_) {
        return true;
    }

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
    bWindowClassRegistered_ = true;

    // 2. 创建窗口
    const DWORD windowStyle = GetResolvedWindowStyle();
    RECT windowRect = {0, 0, WindowWidth_, WindowHeight_};
    if (bUseCustomHostChrome_) {
        windowRect.bottom += GetHostChromeHeight();
    }
    AdjustWindowRect(&windowRect, windowStyle, FALSE);

    Hwnd_ = CreateWindowW(
        wc.lpszClassName,
        WindowTitle_.c_str(),
        windowStyle,
        100, 100,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
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
    bImGuiContextOwned_ = true;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (Application_ != nullptr) {
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
        Application_->EnsureDefaultFontConfigured();
    }

    // 6. 设置 ImGui 样式
    ImGui::StyleColorsDark();

    // 7. 初始化 ImGui 后端
    ImGui_ImplWin32_Init(Hwnd_);
    ImGui_ImplDX11_Init(D3DDevice_, D3DDeviceContext_);
    bImGuiBackendInitialized_ = true;

    return true;
}

void ImWin32DX11Backend::Shutdown() {
    // 1. 清理 ImGui
    if (bImGuiBackendInitialized_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        bImGuiBackendInitialized_ = false;
    }

    if (bImGuiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
        bImGuiContextOwned_ = false;
    }

    // 2. 清理 DirectX 11
    CleanupDeviceD3D();

    // 3. 销毁窗口
    if (Hwnd_ != nullptr) {
        if (::IsWindow(Hwnd_)) {
            DestroyWindow(Hwnd_);
        }
        Hwnd_ = nullptr;
    }

    // 4. 注销窗口类
    if (bWindowClassRegistered_) {
        UnregisterClassW(L"ImWidgetV4WindowClass", HInstance_);
        bWindowClassRegistered_ = false;
    }
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
            ImGuiIO& io = ImGui::GetIO();
            FImGuiInputSnapshot inputSnapshot;
            PopulateImGuiInputSnapshotFromIo(io, inputSnapshot);
            InputSource_.SetSnapshot(inputSnapshot);
            DrawContext drawContext(ImGui::GetBackgroundDrawList());
            RECT clientRect = {};
            GetClientRect(Hwnd_, &clientRect);

            const float chromeTopInset = bUseCustomHostChrome_
                ? static_cast<float>(GetHostChromeHeight())
                : 0.0f;
            const float viewportWidth = static_cast<float>(std::max(0L, clientRect.right - clientRect.left));
            const float viewportHeight = static_cast<float>(std::max(
                0L,
                (clientRect.bottom - clientRect.top) - static_cast<LONG>(chromeTopInset)));

            FFrameContext frameContext;
            frameContext.FrameInfo.ViewportPosition = FVector2(0.0f, chromeTopInset);
            frameContext.FrameInfo.ViewportSize = FVector2(viewportWidth, viewportHeight);
            frameContext.FrameInfo.DeltaTime = io.DeltaTime;
            frameContext.FrameInfo.CurrentTime = ImGui::GetTime();
            frameContext.DrawContext_ = &drawContext;
            frameContext.InputSource = &InputSource_;

            frameInfo = frameContext.FrameInfo;
            bHasFrameInfo = true;
            Application_->AdvanceFrame(frameContext);
        }

        if (bUseCustomHostChrome_) {
            DrawCustomHostChrome();
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
    WindowTitle_ = Utf8ToWide(title);

    if (Hwnd_) {
        SetWindowTextW(Hwnd_, WindowTitle_.c_str());
    }
}

void ImWin32DX11Backend::SetWindowSize(int width, int height) {
    WindowWidth_ = width;
    WindowHeight_ = height;

    if (Hwnd_) {
        RECT rect = {0, 0, width, height};
        if (bUseCustomHostChrome_) {
            rect.bottom += GetHostChromeHeight();
        }
        AdjustWindowRect(&rect, GetResolvedWindowStyle(), FALSE);
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
        Application_->SetBackend(this);
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
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
    if (Hwnd_ != nullptr && ::IsWindow(Hwnd_)) {
        ::PostMessageW(Hwnd_, WM_CLOSE, 0, 0);
    }
}

std::string ImWin32DX11Backend::GetBackendName() const {
    return "Win32 + DirectX 11";
}

// ========== DirectX 11 设备管理 ==========

ImTextureID ImWin32DX11Backend::CreateTextureFromRGBA(
    const std::uint8_t* rgbaPixels,
    int width,
    int height)
{
    if (rgbaPixels == nullptr || width <= 0 || height <= 0 || D3DDevice_ == nullptr) {
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC description = {};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subresourceData = {};
    subresourceData.pSysMem = rgbaPixels;
    subresourceData.SysMemPitch = static_cast<UINT>(width * 4);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = D3DDevice_->CreateTexture2D(&description, &subresourceData, &texture);
    if (FAILED(hr) || texture == nullptr) {
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
    viewDescription.Format = description.Format;
    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    viewDescription.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* textureView = nullptr;
    hr = D3DDevice_->CreateShaderResourceView(texture, &viewDescription, &textureView);
    texture->Release();
    if (FAILED(hr) || textureView == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<ImTextureID>(textureView);
}

void ImWin32DX11Backend::ReleaseTexture(ImTextureID textureId)
{
    if (textureId == nullptr) {
        return;
    }

    ID3D11ShaderResourceView* textureView = reinterpret_cast<ID3D11ShaderResourceView*>(textureId);
    textureView->Release();
}

void ImWin32DX11Backend::SetUseCustomHostChrome(bool enabled)
{
    if (bUseCustomHostChrome_ == enabled) {
        return;
    }

    bUseCustomHostChrome_ = enabled;
    HoveredHostChromeButton_ = EHostChromeButton::None;
    PressedHostChromeButton_ = EHostChromeButton::None;

    if (Hwnd_ != nullptr) {
        ApplyWindowStyle();
        SetWindowSize(WindowWidth_, WindowHeight_);
        InvalidateRect(Hwnd_, nullptr, FALSE);
    }
}

DWORD ImWin32DX11Backend::GetResolvedWindowStyle() const
{
    if (bUseCustomHostChrome_) {
        return WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
    }

    return WS_OVERLAPPEDWINDOW;
}

void ImWin32DX11Backend::ApplyWindowStyle()
{
    if (Hwnd_ == nullptr) {
        return;
    }

    SetWindowLongPtrW(Hwnd_, GWL_STYLE, static_cast<LONG_PTR>(GetResolvedWindowStyle()));
    SetWindowPos(
        Hwnd_,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

float ImWin32DX11Backend::GetHostChromeDpiScale() const
{
    if (Hwnd_ != nullptr) {
        const UINT dpi = GetDpiForWindow(Hwnd_);
        if (dpi != 0) {
            return static_cast<float>(dpi) / 96.0f;
        }
    }

    return 1.0f;
}

int ImWin32DX11Backend::GetHostChromeHeight() const
{
    return std::max(28, static_cast<int>(std::lround(30.0f * GetHostChromeDpiScale())));
}

int ImWin32DX11Backend::GetHostChromeResizeBorderThickness() const
{
    const int systemFrame = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    const int fallback = std::max(6, static_cast<int>(std::lround(8.0f * GetHostChromeDpiScale())));
    return std::max(systemFrame, fallback);
}

RECT ImWin32DX11Backend::GetHostChromeButtonRect(EHostChromeButton button) const
{
    RECT clientRect = {};
    if (Hwnd_ == nullptr || button == EHostChromeButton::None) {
        return clientRect;
    }

    GetClientRect(Hwnd_, &clientRect);
    const int chromeHeight = GetHostChromeHeight();
    const int buttonWidth = std::max(chromeHeight, static_cast<int>(std::lround(46.0f * GetHostChromeDpiScale())));

    int buttonIndexFromRight = 0;
    switch (button) {
    case EHostChromeButton::Close:
        buttonIndexFromRight = 0;
        break;
    case EHostChromeButton::Maximize:
        buttonIndexFromRight = 1;
        break;
    case EHostChromeButton::Minimize:
        buttonIndexFromRight = 2;
        break;
    default:
        break;
    }

    clientRect.left = clientRect.right - ((buttonIndexFromRight + 1) * buttonWidth);
    clientRect.right = clientRect.left + buttonWidth;
    clientRect.top = 0;
    clientRect.bottom = chromeHeight;
    return clientRect;
}

ImWin32DX11Backend::EHostChromeButton ImWin32DX11Backend::HitTestHostChromeButton(const POINT& clientPoint) const
{
    if (!bUseCustomHostChrome_ || Hwnd_ == nullptr) {
        return EHostChromeButton::None;
    }

    for (EHostChromeButton button : {
             EHostChromeButton::Close,
             EHostChromeButton::Maximize,
             EHostChromeButton::Minimize}) {
        const RECT buttonRect = GetHostChromeButtonRect(button);
        if (PtInRect(&buttonRect, clientPoint)) {
            return button;
        }
    }

    return EHostChromeButton::None;
}

bool ImWin32DX11Backend::IsPointInHostChromeCaption(const POINT& clientPoint) const
{
    if (!bUseCustomHostChrome_ || Hwnd_ == nullptr) {
        return false;
    }

    RECT clientRect = {};
    GetClientRect(Hwnd_, &clientRect);
    if (clientPoint.y < 0 || clientPoint.y >= GetHostChromeHeight()) {
        return false;
    }

    if (clientPoint.x < 0 || clientPoint.x >= clientRect.right) {
        return false;
    }

    return HitTestHostChromeButton(clientPoint) == EHostChromeButton::None;
}

bool ImWin32DX11Backend::HandleHostChromeMouseDown(UINT msg, const POINT& clientPoint)
{
    if (msg != WM_LBUTTONDOWN || !bUseCustomHostChrome_) {
        return false;
    }

    const EHostChromeButton button = HitTestHostChromeButton(clientPoint);
    if (button == EHostChromeButton::None) {
        return false;
    }

    PressedHostChromeButton_ = button;
    SetCapture(Hwnd_);
    InvalidateRect(Hwnd_, nullptr, FALSE);
    return true;
}

bool ImWin32DX11Backend::HandleHostChromeMouseUp(UINT msg, const POINT& clientPoint)
{
    if (msg != WM_LBUTTONUP || !bUseCustomHostChrome_ || PressedHostChromeButton_ == EHostChromeButton::None) {
        return false;
    }

    const EHostChromeButton releasedButton = PressedHostChromeButton_;
    PressedHostChromeButton_ = EHostChromeButton::None;
    if (GetCapture() == Hwnd_) {
        ReleaseCapture();
    }

    const bool bInvokeAction = HitTestHostChromeButton(clientPoint) == releasedButton;
    InvalidateRect(Hwnd_, nullptr, FALSE);

    if (!bInvokeAction) {
        return true;
    }

    switch (releasedButton) {
    case EHostChromeButton::Minimize:
        ShowWindow(Hwnd_, SW_MINIMIZE);
        break;
    case EHostChromeButton::Maximize:
        ShowWindow(Hwnd_, IsZoomed(Hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
        break;
    case EHostChromeButton::Close:
        RequestClose();
        break;
    default:
        break;
    }

    return true;
}

void ImWin32DX11Backend::DrawCustomHostChrome()
{
    if (!bUseCustomHostChrome_ || Hwnd_ == nullptr) {
        return;
    }

    RECT clientRect = {};
    GetClientRect(Hwnd_, &clientRect);
    const float width = static_cast<float>(clientRect.right - clientRect.left);
    const float chromeHeight = static_cast<float>(GetHostChromeHeight());
    if (width <= 0.0f || chromeHeight <= 0.0f) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const bool bIsActiveWindow = GetForegroundWindow() == Hwnd_;
    const float dpiScale = GetHostChromeDpiScale();
    const float iconStroke = std::max(1.6f, 1.6f * dpiScale);
    const ImU32 barColor = bIsActiveWindow ? IM_COL32(24, 29, 38, 255) : IM_COL32(33, 37, 45, 255);
    const ImU32 borderColor = bIsActiveWindow ? IM_COL32(79, 89, 107, 255) : IM_COL32(61, 68, 83, 255);
    const ImU32 textColor = bIsActiveWindow ? IM_COL32(240, 244, 250, 255) : IM_COL32(195, 202, 212, 255);

    drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(width, chromeHeight), barColor);
    drawList->AddLine(ImVec2(0.0f, chromeHeight - 1.0f), ImVec2(width, chromeHeight - 1.0f), borderColor, 1.0f);

    const std::string titleText = WideToUtf8(WindowTitle_);
    if (!titleText.empty()) {
        const RECT closeRect = GetHostChromeButtonRect(EHostChromeButton::Minimize);
        const float textLeft = 14.0f * dpiScale;
        const float textRight = std::max(textLeft + 1.0f, static_cast<float>(closeRect.left) - (12.0f * dpiScale));
        drawList->PushClipRect(ImVec2(textLeft, 0.0f), ImVec2(textRight, chromeHeight), true);
        drawList->AddText(
            ImVec2(textLeft, (chromeHeight - ImGui::GetFontSize()) * 0.5f),
            textColor,
            titleText.c_str());
        drawList->PopClipRect();
    }

    const ImU32 buttonBaseColor = IM_COL32(0, 0, 0, 0);
    const ImU32 buttonHoverColor = IM_COL32(255, 255, 255, 20);
    const ImU32 buttonPressedColor = IM_COL32(255, 255, 255, 34);
    const ImU32 closeHoverColor = IM_COL32(212, 58, 76, 220);
    const ImU32 closePressedColor = IM_COL32(188, 46, 66, 240);

    for (EHostChromeButton button : {
             EHostChromeButton::Minimize,
             EHostChromeButton::Maximize,
             EHostChromeButton::Close}) {
        const RECT buttonRect = GetHostChromeButtonRect(button);
        const ImVec2 buttonMin(static_cast<float>(buttonRect.left), static_cast<float>(buttonRect.top));
        const ImVec2 buttonMax(static_cast<float>(buttonRect.right), static_cast<float>(buttonRect.bottom));
        const bool bHovered = HoveredHostChromeButton_ == button;
        const bool bPressed = PressedHostChromeButton_ == button;

        ImU32 fillColor = buttonBaseColor;
        if (button == EHostChromeButton::Close) {
            fillColor = bPressed ? closePressedColor : (bHovered ? closeHoverColor : buttonBaseColor);
        } else if (bPressed) {
            fillColor = buttonPressedColor;
        } else if (bHovered) {
            fillColor = buttonHoverColor;
        }

        if ((fillColor >> IM_COL32_A_SHIFT) != 0) {
            drawList->AddRectFilled(buttonMin, buttonMax, fillColor);
        }

        const ImVec2 center((buttonMin.x + buttonMax.x) * 0.5f, (buttonMin.y + buttonMax.y) * 0.5f);
        const float halfExtent = 6.0f * dpiScale;
        const ImU32 glyphColor = IM_COL32(245, 247, 250, 255);

        switch (button) {
        case EHostChromeButton::Minimize:
            drawList->AddLine(
                ImVec2(center.x - halfExtent, center.y + 4.0f * dpiScale),
                ImVec2(center.x + halfExtent, center.y + 4.0f * dpiScale),
                glyphColor,
                iconStroke);
            break;

        case EHostChromeButton::Maximize:
            if (IsZoomed(Hwnd_)) {
                const float offset = 2.5f * dpiScale;
                drawList->AddRect(
                    ImVec2(center.x - halfExtent + offset, center.y - halfExtent - offset),
                    ImVec2(center.x + halfExtent + offset, center.y + halfExtent - offset),
                    glyphColor,
                    0.0f,
                    0,
                    iconStroke);
                drawList->AddRect(
                    ImVec2(center.x - halfExtent - offset, center.y - halfExtent + offset),
                    ImVec2(center.x + halfExtent - offset, center.y + halfExtent + offset),
                    glyphColor,
                    0.0f,
                    0,
                    iconStroke);
            } else {
                drawList->AddRect(
                    ImVec2(center.x - halfExtent, center.y - halfExtent),
                    ImVec2(center.x + halfExtent, center.y + halfExtent),
                    glyphColor,
                    0.0f,
                    0,
                    iconStroke);
            }
            break;

        case EHostChromeButton::Close:
            drawList->AddLine(
                ImVec2(center.x - halfExtent, center.y - halfExtent),
                ImVec2(center.x + halfExtent, center.y + halfExtent),
                glyphColor,
                iconStroke);
            drawList->AddLine(
                ImVec2(center.x + halfExtent, center.y - halfExtent),
                ImVec2(center.x - halfExtent, center.y + halfExtent),
                glyphColor,
                iconStroke);
            break;

        default:
            break;
        }
    }
}

std::wstring ImWin32DX11Backend::Utf8ToWide(const std::string& text)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(text);
}

std::string ImWin32DX11Backend::WideToUtf8(const std::wstring& text)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(text);
}

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
    if (bUseCustomHostChrome_) {
        switch (msg) {
        case WM_NCCALCSIZE:
            return 0;

        case WM_GETMINMAXINFO: {
            MINMAXINFO* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo = {};
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (GetMonitorInfoW(monitor, &monitorInfo)) {
                minMaxInfo->ptMaxPosition.x = monitorInfo.rcWork.left - monitorInfo.rcMonitor.left;
                minMaxInfo->ptMaxPosition.y = monitorInfo.rcWork.top - monitorInfo.rcMonitor.top;
                minMaxInfo->ptMaxSize.x = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
                minMaxInfo->ptMaxSize.y = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
            }
            return 0;
        }

        case WM_NCHITTEST: {
            const POINT screenPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            POINT clientPoint = screenPoint;
            ScreenToClient(hWnd, &clientPoint);

            RECT clientRect = {};
            GetClientRect(hWnd, &clientRect);
            const int resizeBorder = IsZoomed(hWnd) ? 0 : GetHostChromeResizeBorderThickness();
            const bool bOnLeft = clientPoint.x >= 0 && clientPoint.x < resizeBorder;
            const bool bOnRight = clientPoint.x < clientRect.right && clientPoint.x >= clientRect.right - resizeBorder;
            const bool bOnTop = clientPoint.y >= 0 && clientPoint.y < resizeBorder;
            const bool bOnBottom = clientPoint.y < clientRect.bottom && clientPoint.y >= clientRect.bottom - resizeBorder;

            if (bOnTop && bOnLeft) {
                return HTTOPLEFT;
            }
            if (bOnTop && bOnRight) {
                return HTTOPRIGHT;
            }
            if (bOnBottom && bOnLeft) {
                return HTBOTTOMLEFT;
            }
            if (bOnBottom && bOnRight) {
                return HTBOTTOMRIGHT;
            }
            if (bOnLeft) {
                return HTLEFT;
            }
            if (bOnRight) {
                return HTRIGHT;
            }
            if (bOnTop) {
                return HTTOP;
            }
            if (bOnBottom) {
                return HTBOTTOM;
            }
            if (HitTestHostChromeButton(clientPoint) != EHostChromeButton::None) {
                return HTCLIENT;
            }
            if (IsPointInHostChromeCaption(clientPoint)) {
                return HTCAPTION;
            }
            return HTCLIENT;
        }

        case WM_MOUSEMOVE: {
            const POINT clientPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            TRACKMOUSEEVENT trackMouseEvent = {};
            trackMouseEvent.cbSize = sizeof(trackMouseEvent);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = hWnd;
            TrackMouseEvent(&trackMouseEvent);
            const EHostChromeButton hoveredButton = HitTestHostChromeButton(clientPoint);
            if (HoveredHostChromeButton_ != hoveredButton) {
                HoveredHostChromeButton_ = hoveredButton;
                InvalidateRect(hWnd, nullptr, FALSE);
            } else if (PressedHostChromeButton_ != EHostChromeButton::None || clientPoint.y < GetHostChromeHeight()) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_MOUSELEAVE:
        case WM_CAPTURECHANGED:
            if (HoveredHostChromeButton_ != EHostChromeButton::None || PressedHostChromeButton_ != EHostChromeButton::None) {
                HoveredHostChromeButton_ = EHostChromeButton::None;
                if (msg == WM_CAPTURECHANGED) {
                    PressedHostChromeButton_ = EHostChromeButton::None;
                }
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;

        case WM_LBUTTONDOWN: {
            const POINT clientPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (HandleHostChromeMouseDown(msg, clientPoint)) {
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            const POINT clientPoint = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (HandleHostChromeMouseUp(msg, clientPoint)) {
                return 0;
            }
            break;
        }
        }
    }

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

    case WM_CLOSE:
        bShouldClose_ = true;
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        Hwnd_ = nullptr;
        bShouldClose_ = true;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace ImWidgetV4
