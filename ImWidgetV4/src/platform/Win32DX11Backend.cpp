#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/core/Widget.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <locale>
#include <shobjidl.h>
#include <sstream>

// 前向声明 ImGui Win32 窗口过程处理函数
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImWidgetV4 {

struct ImWin32DX11Backend::FHostChromeLayoutCache {
    struct FTabLayout {
        int TabIndex = -1;
        FGeometry Geometry;
    };

    struct FActionButtonLayout {
        int ActionIndex = -1;
        FGeometry Geometry;
    };

    FGeometry IconGeometry;
    FGeometry TitleGeometry;
    std::vector<FTabLayout> VisibleTabs;
    std::vector<FActionButtonLayout> ActionButtons;
    float TitleFontSize = 0.0f;
    float TabFontSize = 0.0f;
    float IconInset = 0.0f;
};

namespace {

constexpr int InvalidHostChromeIndex = -1;

struct FScopedCoInitialize {
    HRESULT Result = E_FAIL;
    bool bShouldUninitialize = false;

    FScopedCoInitialize()
    {
        Result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        bShouldUninitialize = SUCCEEDED(Result);
    }

    ~FScopedCoInitialize()
    {
        if (bShouldUninitialize) {
            CoUninitialize();
        }
    }
};

std::wstring Utf8ToWideLocal(const std::string& text)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(text);
}

std::string WideToUtf8Local(const std::wstring& text)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(text);
}

std::string FormatDialogErrorMessage(HRESULT result)
{
    LPWSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageW(
        flags,
        nullptr,
        static_cast<DWORD>(result),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::ostringstream stream;
    stream << "HRESULT 0x" << std::hex << std::uppercase << static_cast<unsigned long>(result);
    if (length == 0 || buffer == nullptr) {
        return stream.str();
    }

    std::wstring wideMessage(buffer, length);
    LocalFree(buffer);

    while (!wideMessage.empty() &&
           (wideMessage.back() == L'\r' || wideMessage.back() == L'\n' || wideMessage.back() == L' ')) {
        wideMessage.pop_back();
    }

    return stream.str() + ": " + WideToUtf8Local(wideMessage);
}

bool IsDialogCancelled(HRESULT result)
{
    return result == HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

std::wstring JoinFilterPatterns(const std::vector<std::string>& patterns)
{
    std::wstring combined;
    for (std::size_t index = 0; index < patterns.size(); ++index) {
        if (index > 0) {
            combined += L";";
        }
        combined += Utf8ToWideLocal(patterns[index]);
    }
    return combined;
}

void ApplyDialogTitle(IFileDialog* dialog, const std::string& title)
{
    if (dialog == nullptr || title.empty()) {
        return;
    }

    const std::wstring wideTitle = Utf8ToWideLocal(title);
    dialog->SetTitle(wideTitle.c_str());
}

void ApplyInitialDirectory(IFileDialog* dialog, const std::filesystem::path& initialDirectory)
{
    if (dialog == nullptr || initialDirectory.empty()) {
        return;
    }

    IShellItem* folderItem = nullptr;
    const HRESULT itemResult = SHCreateItemFromParsingName(
        initialDirectory.wstring().c_str(),
        nullptr,
        IID_PPV_ARGS(&folderItem));
    if (FAILED(itemResult) || folderItem == nullptr) {
        return;
    }

    dialog->SetFolder(folderItem);
    folderItem->Release();
}

void ApplyFilters(
    IFileDialog* dialog,
    const std::vector<FFileDialogFilter>& filters,
    int defaultFilterIndex,
    std::vector<std::wstring>& outLabels,
    std::vector<std::wstring>& outSpecs,
    std::vector<COMDLG_FILTERSPEC>& outFilterSpecs)
{
    if (dialog == nullptr || filters.empty()) {
        return;
    }

    outLabels.clear();
    outSpecs.clear();
    outFilterSpecs.clear();
    outLabels.reserve(filters.size());
    outSpecs.reserve(filters.size());
    outFilterSpecs.reserve(filters.size());

    for (const FFileDialogFilter& filter : filters) {
        outLabels.push_back(Utf8ToWideLocal(filter.Label.empty() ? "Files" : filter.Label));
        outSpecs.push_back(JoinFilterPatterns(filter.Patterns));
    }

    for (std::size_t index = 0; index < filters.size(); ++index) {
        COMDLG_FILTERSPEC filterSpec = {};
        filterSpec.pszName = outLabels[index].c_str();
        filterSpec.pszSpec = outSpecs[index].empty() ? L"*.*" : outSpecs[index].c_str();
        outFilterSpecs.push_back(filterSpec);
    }

    dialog->SetFileTypes(static_cast<UINT>(outFilterSpecs.size()), outFilterSpecs.data());
    const UINT filterIndex = static_cast<UINT>(std::clamp(defaultFilterIndex, 0, static_cast<int>(filters.size()) - 1) + 1);
    dialog->SetFileTypeIndex(filterIndex);
}

FPathDialogResult ExtractDialogResultPath(IFileDialog* dialog)
{
    if (dialog == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = "Dialog instance was null.";
        return result;
    }

    IShellItem* shellItem = nullptr;
    HRESULT hr = dialog->GetResult(&shellItem);
    if (FAILED(hr) || shellItem == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    PWSTR fileSystemPath = nullptr;
    hr = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &fileSystemPath);
    shellItem->Release();
    if (FAILED(hr) || fileSystemPath == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    FPathDialogResult result;
    result.Code = EPathDialogResultCode::Accepted;
    result.Path = std::filesystem::path(fileSystemPath);
    CoTaskMemFree(fileSystemPath);
    return result;
}

float MeasureHostChromeTextWidth(const std::string& text, float fontSize)
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str()).x;
    }

    return fontSize * 0.55f * static_cast<float>(text.size());
}

std::vector<std::uint8_t> ResampleRgbaNearest(
    const std::uint8_t* pixels,
    int sourceWidth,
    int sourceHeight,
    int targetWidth,
    int targetHeight)
{
    std::vector<std::uint8_t> output(
        static_cast<std::size_t>(targetWidth) * static_cast<std::size_t>(targetHeight) * 4U,
        0U);
    if (pixels == nullptr || sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
        return output;
    }

    for (int y = 0; y < targetHeight; ++y) {
        const int sourceY = std::clamp(
            static_cast<int>((static_cast<float>(y) / static_cast<float>(targetHeight)) * static_cast<float>(sourceHeight)),
            0,
            sourceHeight - 1);
        for (int x = 0; x < targetWidth; ++x) {
            const int sourceX = std::clamp(
                static_cast<int>((static_cast<float>(x) / static_cast<float>(targetWidth)) * static_cast<float>(sourceWidth)),
                0,
                sourceWidth - 1);
            const std::size_t sourceOffset =
                (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(sourceWidth) + static_cast<std::size_t>(sourceX)) * 4U;
            const std::size_t destinationOffset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(targetWidth) + static_cast<std::size_t>(x)) * 4U;
            output[destinationOffset] = pixels[sourceOffset];
            output[destinationOffset + 1] = pixels[sourceOffset + 1];
            output[destinationOffset + 2] = pixels[sourceOffset + 2];
            output[destinationOffset + 3] = pixels[sourceOffset + 3];
        }
    }

    return output;
}

FPopupMenuStyle BuildHostChromePopupMenuStyle(int hostChromeHeight)
{
    FPopupMenuStyle style;
    style.BackgroundColor = FColor::FromBytes(26, 31, 38);
    style.BorderColor = FColor::FromBytes(63, 73, 89);
    style.RowHoveredColor = FColor::FromBytes(48, 60, 77);
    style.RowPressedColor = FColor::FromBytes(69, 101, 154);
    style.TextColor = FColor::FromBytes(238, 242, 247);
    style.DisabledTextColor = FColor::FromBytes(128, 134, 143);
    style.SeparatorColor = FColor::FromBytes(57, 66, 80);
    style.FontSize = std::max(13.0f, static_cast<float>(hostChromeHeight) * 0.46f);
    style.RowHeight = std::max(24.0f, static_cast<float>(hostChromeHeight) * 0.92f);
    style.IconSize = std::max(14.0f, static_cast<float>(hostChromeHeight) - 12.0f);
    style.HorizontalPadding = std::max(10.0f, static_cast<float>(hostChromeHeight) * 0.33f);
    style.IconTextSpacing = 8.0f;
    style.OuterPaddingX = 4.0f;
    style.OuterPaddingY = 6.0f;
    style.CornerRadius = 8.0f;
    style.BorderThickness = 1.0f;
    style.MinDesiredSize = FVector2(180.0f, 36.0f);
    return style;
}

} // namespace

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
    , HoveredTitleBarActionIndex_(InvalidHostChromeIndex)
    , PressedTitleBarActionIndex_(InvalidHostChromeIndex)
    , Application_(nullptr)
{
    HostChromeLayoutCache_ = std::make_unique<FHostChromeLayoutCache>();
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
    CloseTitleBarMenuPopup();
    DestroyWindowIcons();

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
            SyncTitleBarMenuPopupState();
            UpdateHostChromeLayoutCache();
            UpdateTitleBarMenuPopupWindowLayout();

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
        Application_->EnsureDefaultFontConfigured();
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
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

void ImWin32DX11Backend::SetCloseRequestedHandler(FCloseRequestedHandler callback)
{
    CloseRequestedHandler_ = std::move(callback);
}

void ImWin32DX11Backend::ClearCloseRequestedHandler()
{
    CloseRequestedHandler_ = nullptr;
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

bool ImWin32DX11Backend::SetWindowIconFromRGBA(
    const std::uint8_t* rgbaPixels,
    int width,
    int height)
{
    if (Hwnd_ == nullptr || rgbaPixels == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    const auto createIconHandle = [&](int targetSize) -> HICON {
        const std::vector<std::uint8_t> scaledPixels = (width == targetSize && height == targetSize)
            ? std::vector<std::uint8_t>(rgbaPixels, rgbaPixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U))
            : ResampleRgbaNearest(rgbaPixels, width, height, targetSize, targetSize);

        BITMAPV5HEADER bitmapHeader = {};
        bitmapHeader.bV5Size = sizeof(bitmapHeader);
        bitmapHeader.bV5Width = targetSize;
        bitmapHeader.bV5Height = -targetSize;
        bitmapHeader.bV5Planes = 1;
        bitmapHeader.bV5BitCount = 32;
        bitmapHeader.bV5Compression = BI_BITFIELDS;
        bitmapHeader.bV5RedMask = 0x00FF0000;
        bitmapHeader.bV5GreenMask = 0x0000FF00;
        bitmapHeader.bV5BlueMask = 0x000000FF;
        bitmapHeader.bV5AlphaMask = 0xFF000000;

        void* dibPixels = nullptr;
        HDC screenDc = GetDC(nullptr);
        HBITMAP colorBitmap = CreateDIBSection(
            screenDc,
            reinterpret_cast<BITMAPINFO*>(&bitmapHeader),
            DIB_RGB_COLORS,
            &dibPixels,
            nullptr,
            0);
        ReleaseDC(nullptr, screenDc);
        if (colorBitmap == nullptr || dibPixels == nullptr) {
            if (colorBitmap != nullptr) {
                DeleteObject(colorBitmap);
            }
            return nullptr;
        }

        std::uint8_t* destination = static_cast<std::uint8_t*>(dibPixels);
        for (int pixelIndex = 0; pixelIndex < targetSize * targetSize; ++pixelIndex) {
            const std::size_t offset = static_cast<std::size_t>(pixelIndex) * 4U;
            destination[offset] = scaledPixels[offset + 2];
            destination[offset + 1] = scaledPixels[offset + 1];
            destination[offset + 2] = scaledPixels[offset];
            destination[offset + 3] = scaledPixels[offset + 3];
        }

        HBITMAP maskBitmap = CreateBitmap(targetSize, targetSize, 1, 1, nullptr);
        if (maskBitmap == nullptr) {
            DeleteObject(colorBitmap);
            return nullptr;
        }

        ICONINFO iconInfo = {};
        iconInfo.fIcon = TRUE;
        iconInfo.hbmMask = maskBitmap;
        iconInfo.hbmColor = colorBitmap;
        HICON iconHandle = CreateIconIndirect(&iconInfo);
        DeleteObject(colorBitmap);
        DeleteObject(maskBitmap);
        return iconHandle;
    };

    HICON smallIcon = createIconHandle(16);
    HICON largeIcon = createIconHandle(32);
    if (smallIcon == nullptr && largeIcon == nullptr) {
        return false;
    }

    DestroyWindowIcons();
    SmallWindowIcon_ = smallIcon;
    LargeWindowIcon_ = largeIcon;

    SendMessageW(Hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(SmallWindowIcon_));
    SendMessageW(Hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LargeWindowIcon_));
    return true;
}

void ImWin32DX11Backend::ClearWindowIcon()
{
    if (Hwnd_ != nullptr) {
        SendMessageW(Hwnd_, WM_SETICON, ICON_SMALL, 0);
        SendMessageW(Hwnd_, WM_SETICON, ICON_BIG, 0);
    }

    DestroyWindowIcons();
}

FPathDialogResult ImWin32DX11Backend::OpenFileDialog(const FOpenFileDialogOptions& options)
{
    FScopedCoInitialize scopedCoInitialize;
    if (FAILED(scopedCoInitialize.Result)) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(scopedCoInitialize.Result);
        return result;
    }

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    DWORD dialogOptions = 0;
    hr = dialog->GetOptions(&dialogOptions);
    if (SUCCEEDED(hr)) {
        dialogOptions |= FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST;
        dialog->SetOptions(dialogOptions);
    }

    ApplyDialogTitle(dialog, options.Title);
    ApplyInitialDirectory(dialog, options.InitialDirectory);

    std::vector<std::wstring> filterLabels;
    std::vector<std::wstring> filterSpecs;
    std::vector<COMDLG_FILTERSPEC> dialogFilters;
    ApplyFilters(dialog, options.Filters, options.DefaultFilterIndex, filterLabels, filterSpecs, dialogFilters);

    hr = dialog->Show(Hwnd_);
    if (IsDialogCancelled(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Cancelled;
        return result;
    }

    if (FAILED(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    FPathDialogResult result = ExtractDialogResultPath(dialog);
    dialog->Release();
    return result;
}

FPathDialogResult ImWin32DX11Backend::OpenFolderDialog(const FOpenFolderDialogOptions& options)
{
    FScopedCoInitialize scopedCoInitialize;
    if (FAILED(scopedCoInitialize.Result)) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(scopedCoInitialize.Result);
        return result;
    }

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    DWORD dialogOptions = 0;
    hr = dialog->GetOptions(&dialogOptions);
    if (SUCCEEDED(hr)) {
        dialogOptions |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS;
        dialog->SetOptions(dialogOptions);
    }

    ApplyDialogTitle(dialog, options.Title);
    ApplyInitialDirectory(dialog, options.InitialDirectory);

    hr = dialog->Show(Hwnd_);
    if (IsDialogCancelled(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Cancelled;
        return result;
    }

    if (FAILED(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    FPathDialogResult result = ExtractDialogResultPath(dialog);
    dialog->Release();
    return result;
}

FPathDialogResult ImWin32DX11Backend::SaveFileDialog(const FSaveFileDialogOptions& options)
{
    FScopedCoInitialize scopedCoInitialize;
    if (FAILED(scopedCoInitialize.Result)) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(scopedCoInitialize.Result);
        return result;
    }

    IFileSaveDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr) {
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    DWORD dialogOptions = 0;
    hr = dialog->GetOptions(&dialogOptions);
    if (SUCCEEDED(hr)) {
        dialogOptions |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
        if (options.bPromptOverwrite) {
            dialogOptions |= FOS_OVERWRITEPROMPT;
        } else {
            dialogOptions &= ~FOS_OVERWRITEPROMPT;
        }
        dialog->SetOptions(dialogOptions);
    }

    ApplyDialogTitle(dialog, options.Title);
    ApplyInitialDirectory(dialog, options.InitialDirectory);

    if (!options.DefaultFileName.empty()) {
        const std::wstring wideFileName = Utf8ToWideLocal(options.DefaultFileName);
        dialog->SetFileName(wideFileName.c_str());
    }

    if (!options.DefaultExtension.empty()) {
        std::string extension = options.DefaultExtension;
        if (!extension.empty() && extension.front() == '.') {
            extension.erase(extension.begin());
        }
        const std::wstring wideExtension = Utf8ToWideLocal(extension);
        dialog->SetDefaultExtension(wideExtension.c_str());
    }

    std::vector<std::wstring> filterLabels;
    std::vector<std::wstring> filterSpecs;
    std::vector<COMDLG_FILTERSPEC> dialogFilters;
    ApplyFilters(dialog, options.Filters, options.DefaultFilterIndex, filterLabels, filterSpecs, dialogFilters);

    hr = dialog->Show(Hwnd_);
    if (IsDialogCancelled(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Cancelled;
        return result;
    }

    if (FAILED(hr)) {
        dialog->Release();
        FPathDialogResult result;
        result.Code = EPathDialogResultCode::Error;
        result.ErrorMessage = FormatDialogErrorMessage(hr);
        return result;
    }

    FPathDialogResult result = ExtractDialogResultPath(dialog);
    dialog->Release();
    return result;
}

void ImWin32DX11Backend::SetUseCustomHostChrome(bool enabled)
{
    if (bUseCustomHostChrome_ == enabled) {
        return;
    }

    bUseCustomHostChrome_ = enabled;
    HoveredHostChromeButton_ = EHostChromeButton::None;
    PressedHostChromeButton_ = EHostChromeButton::None;
    HoveredTitleBarTabIndex_ = InvalidHostChromeIndex;
    PressedTitleBarTabIndex_ = InvalidHostChromeIndex;
    HoveredTitleBarActionIndex_ = InvalidHostChromeIndex;
    PressedTitleBarActionIndex_ = InvalidHostChromeIndex;
    if (!bUseCustomHostChrome_) {
        CloseTitleBarMenuPopup();
    }

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

int ImWin32DX11Backend::HitTestTitleBarActionButton(const POINT& clientPoint) const
{
    if (!bUseCustomHostChrome_) {
        return InvalidHostChromeIndex;
    }

    const auto* layoutCache = HostChromeLayoutCache_.get();
    if (layoutCache == nullptr) {
        return InvalidHostChromeIndex;
    }

    const FVector2 point(static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y));
    for (const auto& actionLayout : layoutCache->ActionButtons) {
        if (actionLayout.Geometry.Contains(point)) {
            return actionLayout.ActionIndex;
        }
    }

    return InvalidHostChromeIndex;
}

void ImWin32DX11Backend::SyncTitleBarMenuPopupState()
{
    if (!bUseCustomHostChrome_) {
        CloseTitleBarMenuPopup();
        return;
    }

    if (TitleBarMenuPopupWindow_ && !TitleBarMenuPopupWindow_->IsOpen()) {
        TitleBarMenuPopupWindow_.reset();
        TitleBarMenuPopupWidget_.reset();
        ActiveTitleBarTabIndex_ = InvalidHostChromeIndex;
    }

    if (Application_ == nullptr) {
        CloseTitleBarMenuPopup();
        return;
    }

    const std::vector<FApplicationTitleBarTab>& tabs = Application_->GetTitleBarTabMenus();
    if (ActiveTitleBarTabIndex_ < 0 || ActiveTitleBarTabIndex_ >= static_cast<int>(tabs.size())) {
        CloseTitleBarMenuPopup();
    }
}

void ImWin32DX11Backend::UpdateHostChromeLayoutCache()
{
    if (HostChromeLayoutCache_ == nullptr) {
        HostChromeLayoutCache_ = std::make_unique<FHostChromeLayoutCache>();
    }

    HostChromeLayoutCache_->VisibleTabs.clear();
    HostChromeLayoutCache_->ActionButtons.clear();
    HostChromeLayoutCache_->IconGeometry = FGeometry();
    HostChromeLayoutCache_->TitleGeometry = FGeometry();

    if (!bUseCustomHostChrome_ || Hwnd_ == nullptr) {
        return;
    }

    RECT clientRect = {};
    GetClientRect(Hwnd_, &clientRect);
    const float chromeHeight = static_cast<float>(GetHostChromeHeight());
    const float dpiScale = GetHostChromeDpiScale();
    const float leftPadding = 12.0f * dpiScale;
    const float interItemSpacing = 10.0f * dpiScale;
    const float tabHorizontalPadding = 12.0f * dpiScale;
    const float actionButtonSpacing = 6.0f * dpiScale;
    const float actionButtonSize = std::max(18.0f, chromeHeight - 10.0f * dpiScale);
    const float titleFontSize = std::max(13.0f, chromeHeight * 0.47f);
    const float tabFontSize = std::max(12.0f, chromeHeight * 0.43f);
    const float iconInset = std::max(4.0f, chromeHeight * 0.18f);

    HostChromeLayoutCache_->TitleFontSize = titleFontSize;
    HostChromeLayoutCache_->TabFontSize = tabFontSize;
    HostChromeLayoutCache_->IconInset = iconInset;

    const RECT minimizeRect = GetHostChromeButtonRect(EHostChromeButton::Minimize);
    const float contentMaxX = static_cast<float>(minimizeRect.left) - chromeHeight;
    float cursorX = leftPadding;

    if (Application_ != nullptr) {
        const std::vector<FApplicationTitleBarTab>& tabs = Application_->GetTitleBarTabMenus();
        const std::vector<FApplicationTitleBarActionButton>& actionButtons = Application_->GetTitleBarActionButtons();
        const float tabSpacing = 4.0f * dpiScale;
        const float groupSpacing = 8.0f * dpiScale;
        const auto measureTabWidth = [&](const FApplicationTitleBarTab& tab) {
            if (tab.LabelKind == EApplicationTitleBarTabLabelKind::Icon && tab.Icon.IsValid()) {
                const float iconSize = std::max(12.0f, chromeHeight - iconInset * 2.0f);
                return iconSize + tabHorizontalPadding * 2.0f;
            }

            return MeasureHostChromeTextWidth(tab.Text, tabFontSize) + tabHorizontalPadding * 2.0f;
        };

        std::size_t trailingIconStart = tabs.size();
        while (trailingIconStart > 0) {
            const FApplicationTitleBarTab& tab = tabs[trailingIconStart - 1];
            if (tab.LabelKind != EApplicationTitleBarTabLabelKind::Icon || !tab.Icon.IsValid()) {
                break;
            }
            --trailingIconStart;
        }

        const float actionGroupWidth = actionButtons.empty()
            ? 0.0f
            : static_cast<float>(actionButtons.size()) * actionButtonSize +
                static_cast<float>(std::max<std::size_t>(0, actionButtons.size() - 1)) * actionButtonSpacing;

        float trailingIconGroupWidth = 0.0f;
        for (std::size_t tabIndex = trailingIconStart; tabIndex < tabs.size(); ++tabIndex) {
            trailingIconGroupWidth += measureTabWidth(tabs[tabIndex]);
            if (tabIndex + 1 < tabs.size()) {
                trailingIconGroupWidth += tabSpacing;
            }
        }

        float reservedSuffixWidth = 0.0f;
        if (actionGroupWidth > 0.0f) {
            reservedSuffixWidth += actionGroupWidth;
        }
        if (trailingIconGroupWidth > 0.0f) {
            if (reservedSuffixWidth > 0.0f) {
                reservedSuffixWidth += groupSpacing;
            }
            reservedSuffixWidth += trailingIconGroupWidth;
        }

        const float leadingContentMaxX = std::max(cursorX, contentMaxX - reservedSuffixWidth);

        const FImageBrush& applicationIcon = Application_->GetApplicationIcon();
        if (applicationIcon.IsValid()) {
            const float iconSize = std::max(12.0f, chromeHeight - iconInset * 2.0f);
            HostChromeLayoutCache_->IconGeometry = FGeometry(
                FVector2(cursorX, (chromeHeight - iconSize) * 0.5f),
                FVector2(iconSize, iconSize));
            cursorX += iconSize + interItemSpacing;
        }

        const std::string& applicationTitle = Application_->GetApplicationTitle();
        if (!applicationTitle.empty() && cursorX < leadingContentMaxX) {
            const float titleWidth = MeasureHostChromeTextWidth(applicationTitle, titleFontSize);
            const float clippedWidth = std::max(0.0f, std::min(titleWidth, leadingContentMaxX - cursorX));
            if (clippedWidth > 0.0f) {
                HostChromeLayoutCache_->TitleGeometry = FGeometry(
                    FVector2(cursorX, 0.0f),
                    FVector2(clippedWidth, chromeHeight));
                cursorX += clippedWidth + interItemSpacing;
            }
        }

        HostChromeLayoutCache_->VisibleTabs.reserve(tabs.size());
        for (std::size_t tabIndex = 0; tabIndex < trailingIconStart; ++tabIndex) {
            const float tabWidth = measureTabWidth(tabs[tabIndex]);
            if (cursorX + tabWidth > leadingContentMaxX) {
                break;
            }

            FHostChromeLayoutCache::FTabLayout layout;
            layout.TabIndex = static_cast<int>(tabIndex);
            layout.Geometry = FGeometry(
                FVector2(cursorX, 0.0f),
                FVector2(tabWidth, chromeHeight));
            HostChromeLayoutCache_->VisibleTabs.push_back(layout);
            cursorX += tabWidth + tabSpacing;
        }

        const bool bHasLeadingGroup =
            HostChromeLayoutCache_->IconGeometry.Size.X > 0.0f ||
            HostChromeLayoutCache_->TitleGeometry.Size.X > 0.0f ||
            !HostChromeLayoutCache_->VisibleTabs.empty();

        if (!actionButtons.empty()) {
            if (bHasLeadingGroup) {
                cursorX += groupSpacing;
            }

            HostChromeLayoutCache_->ActionButtons.reserve(actionButtons.size());
            for (std::size_t actionIndex = 0; actionIndex < actionButtons.size(); ++actionIndex) {
                FHostChromeLayoutCache::FActionButtonLayout layout;
                layout.ActionIndex = static_cast<int>(actionIndex);
                layout.Geometry = FGeometry(
                    FVector2(cursorX, (chromeHeight - actionButtonSize) * 0.5f),
                    FVector2(actionButtonSize, actionButtonSize));
                HostChromeLayoutCache_->ActionButtons.push_back(layout);
                cursorX += actionButtonSize;
                if (actionIndex + 1 < actionButtons.size()) {
                    cursorX += actionButtonSpacing;
                }
            }
        }

        if (trailingIconStart < tabs.size()) {
            const bool bHasActionButtons = !HostChromeLayoutCache_->ActionButtons.empty();
            if (bHasLeadingGroup || bHasActionButtons) {
                cursorX += groupSpacing;
            }

            for (std::size_t tabIndex = trailingIconStart; tabIndex < tabs.size(); ++tabIndex) {
                const float tabWidth = measureTabWidth(tabs[tabIndex]);
                if (cursorX + tabWidth > contentMaxX) {
                    break;
                }

                FHostChromeLayoutCache::FTabLayout layout;
                layout.TabIndex = static_cast<int>(tabIndex);
                layout.Geometry = FGeometry(
                    FVector2(cursorX, 0.0f),
                    FVector2(tabWidth, chromeHeight));
                HostChromeLayoutCache_->VisibleTabs.push_back(layout);
                cursorX += tabWidth + tabSpacing;
            }
        }
    }
}

int ImWin32DX11Backend::HitTestTitleBarTab(const POINT& clientPoint)
{
    if (!bUseCustomHostChrome_) {
        return InvalidHostChromeIndex;
    }

    UpdateHostChromeLayoutCache();
    if (HostChromeLayoutCache_ == nullptr) {
        return InvalidHostChromeIndex;
    }

    const FVector2 point(static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y));
    for (const FHostChromeLayoutCache::FTabLayout& tab : HostChromeLayoutCache_->VisibleTabs) {
        if (tab.Geometry.Contains(point)) {
            return tab.TabIndex;
        }
    }

    return InvalidHostChromeIndex;
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

    if (HitTestHostChromeButton(clientPoint) != EHostChromeButton::None) {
        return false;
    }

    if (HitTestTitleBarActionButton(clientPoint) != InvalidHostChromeIndex) {
        return false;
    }

    const_cast<ImWin32DX11Backend*>(this)->UpdateHostChromeLayoutCache();
    if (HostChromeLayoutCache_ != nullptr) {
        const FVector2 point(static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y));
        for (const FHostChromeLayoutCache::FTabLayout& tab : HostChromeLayoutCache_->VisibleTabs) {
            if (tab.Geometry.Contains(point)) {
                return false;
            }
        }
    }

    return true;
}

bool ImWin32DX11Backend::HandleHostChromeMouseDown(UINT msg, const POINT& clientPoint)
{
    if (msg != WM_LBUTTONDOWN || !bUseCustomHostChrome_) {
        return false;
    }

    const bool bPopupHit = TitleBarMenuPopupWindow_ &&
        TitleBarMenuPopupWindow_->IsOpen() &&
        TitleBarMenuPopupWindow_->GetWindowGeometry().Contains(FVector2(static_cast<float>(clientPoint.x), static_cast<float>(clientPoint.y)));
    const int tabIndex = HitTestTitleBarTab(clientPoint);
    const int actionIndex = HitTestTitleBarActionButton(clientPoint);
    if (!bPopupHit && tabIndex == InvalidHostChromeIndex) {
        CloseTitleBarMenuPopup();
    }

    const EHostChromeButton button = HitTestHostChromeButton(clientPoint);
    if (button != EHostChromeButton::None) {
        PressedHostChromeButton_ = button;
        SetCapture(Hwnd_);
        InvalidateRect(Hwnd_, nullptr, FALSE);
        return true;
    }

    if (tabIndex != InvalidHostChromeIndex) {
        PressedTitleBarTabIndex_ = tabIndex;
        SetCapture(Hwnd_);
        InvalidateRect(Hwnd_, nullptr, FALSE);
        return true;
    }

    if (actionIndex != InvalidHostChromeIndex) {
        PressedTitleBarActionIndex_ = actionIndex;
        SetCapture(Hwnd_);
        InvalidateRect(Hwnd_, nullptr, FALSE);
        return true;
    }

    return false;
}

bool ImWin32DX11Backend::HandleHostChromeMouseUp(UINT msg, const POINT& clientPoint)
{
    if (msg != WM_LBUTTONUP || !bUseCustomHostChrome_) {
        return false;
    }

    bool bHandled = false;

    if (PressedHostChromeButton_ != EHostChromeButton::None) {
        const EHostChromeButton releasedButton = PressedHostChromeButton_;
        PressedHostChromeButton_ = EHostChromeButton::None;
        const bool bInvokeAction = HitTestHostChromeButton(clientPoint) == releasedButton;
        if (bInvokeAction) {
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
        }
        bHandled = true;
    }

    if (PressedTitleBarTabIndex_ != InvalidHostChromeIndex) {
        const int pressedTabIndex = PressedTitleBarTabIndex_;
        PressedTitleBarTabIndex_ = InvalidHostChromeIndex;
        const int releasedTabIndex = HitTestTitleBarTab(clientPoint);
        if (pressedTabIndex == releasedTabIndex) {
            if (ActiveTitleBarTabIndex_ == pressedTabIndex && TitleBarMenuPopupWindow_ && TitleBarMenuPopupWindow_->IsOpen()) {
                CloseTitleBarMenuPopup();
            } else {
                OpenTitleBarMenuPopup(pressedTabIndex);
            }
        }
        bHandled = true;
    }

    if (PressedTitleBarActionIndex_ != InvalidHostChromeIndex) {
        const int releasedActionIndex = PressedTitleBarActionIndex_;
        PressedTitleBarActionIndex_ = InvalidHostChromeIndex;
        const int actionIndex = HitTestTitleBarActionButton(clientPoint);
        if (actionIndex == releasedActionIndex && Application_ != nullptr) {
            const auto& actions = Application_->GetTitleBarActionButtons();
            if (releasedActionIndex >= 0 && releasedActionIndex < static_cast<int>(actions.size())) {
                const auto& action = actions[static_cast<std::size_t>(releasedActionIndex)];
                if (!action.IsEnabled || action.IsEnabled()) {
                    if (action.OnInvoked) {
                        action.OnInvoked();
                    }
                }
            }
        }
        bHandled = true;
    }

    if (bHandled && GetCapture() == Hwnd_) {
        ReleaseCapture();
    }

    if (bHandled) {
        InvalidateRect(Hwnd_, nullptr, FALSE);
    }

    return bHandled;
}

void ImWin32DX11Backend::UpdateTitleBarMenuPopupWindowLayout()
{
    SyncTitleBarMenuPopupState();
    if (Application_ == nullptr || TitleBarMenuPopupWindow_ == nullptr || !TitleBarMenuPopupWindow_->IsOpen()) {
        return;
    }

    UpdateHostChromeLayoutCache();
    if (HostChromeLayoutCache_ == nullptr) {
        return;
    }

    const auto it = std::find_if(
        HostChromeLayoutCache_->VisibleTabs.begin(),
        HostChromeLayoutCache_->VisibleTabs.end(),
        [&](const FHostChromeLayoutCache::FTabLayout& tab) {
            return tab.TabIndex == ActiveTitleBarTabIndex_;
        });
    if (it == HostChromeLayoutCache_->VisibleTabs.end()) {
        CloseTitleBarMenuPopup();
        return;
    }

    const FVector2 popupSize = TitleBarMenuPopupWidget_
        ? TitleBarMenuPopupWidget_->GetMinSize()
        : FVector2(180.0f, 36.0f);
    TitleBarMenuPopupWindow_->SetPosition(FVector2(it->Geometry.Position.X, it->Geometry.Position.Y + it->Geometry.Size.Y));
    TitleBarMenuPopupWindow_->SetSize(popupSize);
}

void ImWin32DX11Backend::OpenTitleBarMenuPopup(int tabIndex)
{
    if (Application_ == nullptr || !bUseCustomHostChrome_) {
        return;
    }

    const std::vector<FApplicationTitleBarTab>& tabs = Application_->GetTitleBarTabMenus();
    if (tabIndex < 0 || tabIndex >= static_cast<int>(tabs.size())) {
        return;
    }

    ActiveTitleBarTabIndex_ = tabIndex;
    if (!TitleBarMenuPopupWidget_) {
        TitleBarMenuPopupWidget_ = std::make_shared<ImPopupMenu>();
        TitleBarMenuPopupWidget_->OnItemInvoked.AddLambda([this](ImPopupMenu&, int) {
            CloseTitleBarMenuPopup();
            if (Hwnd_ != nullptr) {
                ::InvalidateRect(Hwnd_, nullptr, FALSE);
            }
        });
    }

    TitleBarMenuPopupWidget_->SetStyle(BuildHostChromePopupMenuStyle(GetHostChromeHeight()));
    TitleBarMenuPopupWidget_->SetItems(tabs[static_cast<std::size_t>(tabIndex)].Items);

    if (!TitleBarMenuPopupWindow_) {
        FPopupOptions popupOptions;
        popupOptions.Title = "HostChromeMenu";
        popupOptions.Position = FVector2(0.0f, static_cast<float>(GetHostChromeHeight()));
        popupOptions.Size = TitleBarMenuPopupWidget_->GetMinSize();
        popupOptions.RootWidget = TitleBarMenuPopupWidget_;
        popupOptions.ParentWindow = Application_->GetWindowManager().GetMainWindow();
        popupOptions.Style.BackgroundColor = FColor::FromBytes(26, 31, 38);
        popupOptions.Style.InactiveBackgroundColor = popupOptions.Style.BackgroundColor;
        popupOptions.Style.BorderColor = FColor::FromBytes(63, 73, 89);
        popupOptions.Style.ActiveBorderColor = popupOptions.Style.BorderColor;
        popupOptions.Style.CornerRadius = 8.0f;
        popupOptions.Style.BorderThickness = 1.0f;
        popupOptions.Style.bDrawShadow = true;
        popupOptions.Style.ShadowColor = FColor(0.0f, 0.0f, 0.0f, 0.18f);
        popupOptions.Style.ShadowOffset = FVector2(0.0f, 10.0f);
        TitleBarMenuPopupWindow_ = Application_->GetWindowManager().CreatePopup(popupOptions);
    } else if (!TitleBarMenuPopupWindow_->IsOpen()) {
        TitleBarMenuPopupWindow_->Open();
    }

    UpdateTitleBarMenuPopupWindowLayout();
}

void ImWin32DX11Backend::CloseTitleBarMenuPopup()
{
    if (TitleBarMenuPopupWindow_ && TitleBarMenuPopupWindow_->IsOpen()) {
        TitleBarMenuPopupWindow_->Close();
    }

    TitleBarMenuPopupWindow_.reset();
    TitleBarMenuPopupWidget_.reset();
    ActiveTitleBarTabIndex_ = InvalidHostChromeIndex;
}

void ImWin32DX11Backend::DrawCustomHostChrome()
{
    if (!bUseCustomHostChrome_ || Hwnd_ == nullptr) {
        return;
    }

    SyncTitleBarMenuPopupState();
    UpdateHostChromeLayoutCache();

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
    const ImU32 tabHoverColor = IM_COL32(255, 255, 255, 18);
    const ImU32 tabActiveColor = IM_COL32(73, 116, 181, 210);

    drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(width, chromeHeight), barColor);
    drawList->AddLine(ImVec2(0.0f, chromeHeight - 1.0f), ImVec2(width, chromeHeight - 1.0f), borderColor, 1.0f);

    if (Application_ != nullptr && HostChromeLayoutCache_ != nullptr) {
        const FImageBrush& applicationIcon = Application_->GetApplicationIcon();
        if (applicationIcon.IsValid() && HostChromeLayoutCache_->IconGeometry.Size.X > 0.0f) {
            const FGeometry& geometry = HostChromeLayoutCache_->IconGeometry;
            drawList->AddImage(
                applicationIcon.TextureId,
                geometry.GetMin().ToImVec2(),
                geometry.GetMax().ToImVec2(),
                applicationIcon.Uv0.ToImVec2(),
                applicationIcon.Uv1.ToImVec2(),
                applicationIcon.TintColor.ToImU32());
        }

        const std::string& titleText = Application_->GetApplicationTitle();
        if (!titleText.empty() && HostChromeLayoutCache_->TitleGeometry.Size.X > 0.0f) {
            const FGeometry& geometry = HostChromeLayoutCache_->TitleGeometry;
            drawList->PushClipRect(
                geometry.GetMin().ToImVec2(),
                geometry.GetMax().ToImVec2(),
                true);
            drawList->AddText(
                nullptr,
                HostChromeLayoutCache_->TitleFontSize,
                ImVec2(
                    geometry.Position.X,
                    (chromeHeight - HostChromeLayoutCache_->TitleFontSize) * 0.5f),
                textColor,
                titleText.c_str());
            drawList->PopClipRect();
        }

        for (const FHostChromeLayoutCache::FTabLayout& tabLayout : HostChromeLayoutCache_->VisibleTabs) {
            const FApplicationTitleBarTab& tab =
                Application_->GetTitleBarTabMenus()[static_cast<std::size_t>(tabLayout.TabIndex)];
            const bool bHovered = HoveredTitleBarTabIndex_ == tabLayout.TabIndex;
            const bool bPressed = PressedTitleBarTabIndex_ == tabLayout.TabIndex;
            const bool bActive = ActiveTitleBarTabIndex_ == tabLayout.TabIndex && TitleBarMenuPopupWindow_ && TitleBarMenuPopupWindow_->IsOpen();

            const ImU32 fillColor = bActive
                ? tabActiveColor
                : (bPressed ? IM_COL32(255, 255, 255, 26) : (bHovered ? tabHoverColor : IM_COL32(0, 0, 0, 0)));
            if ((fillColor >> IM_COL32_A_SHIFT) != 0) {
                drawList->AddRectFilled(
                    tabLayout.Geometry.GetMin().ToImVec2(),
                    tabLayout.Geometry.GetMax().ToImVec2(),
                    fillColor,
                    6.0f);
            }

            if (tab.LabelKind == EApplicationTitleBarTabLabelKind::Icon && tab.Icon.IsValid()) {
                const float iconSize = std::max(12.0f, chromeHeight - HostChromeLayoutCache_->IconInset * 2.0f);
                const FVector2 iconMin(
                    tabLayout.Geometry.Position.X + (tabLayout.Geometry.Size.X - iconSize) * 0.5f,
                    (chromeHeight - iconSize) * 0.5f);
                drawList->AddImage(
                    tab.Icon.TextureId,
                    iconMin.ToImVec2(),
                    FVector2(iconMin.X + iconSize, iconMin.Y + iconSize).ToImVec2(),
                    tab.Icon.Uv0.ToImVec2(),
                    tab.Icon.Uv1.ToImVec2(),
                    tab.Icon.TintColor.ToImU32());
            } else {
                drawList->AddText(
                    nullptr,
                    HostChromeLayoutCache_->TabFontSize,
                    ImVec2(
                        tabLayout.Geometry.Position.X + std::max(10.0f, chromeHeight * 0.33f),
                        (chromeHeight - HostChromeLayoutCache_->TabFontSize) * 0.5f),
                    textColor,
                    tab.Text.c_str());
            }
        }
    }

    const ImU32 buttonBaseColor = IM_COL32(0, 0, 0, 0);
    const ImU32 buttonHoverColor = IM_COL32(255, 255, 255, 20);
    const ImU32 buttonPressedColor = IM_COL32(255, 255, 255, 34);
    const ImU32 closeHoverColor = IM_COL32(212, 58, 76, 220);
    const ImU32 closePressedColor = IM_COL32(188, 46, 66, 240);
    const ImU32 actionHoverColor = IM_COL32(255, 255, 255, 22);
    const ImU32 actionPressedColor = IM_COL32(255, 255, 255, 36);
    const ImU32 actionHighlightColor = IM_COL32(73, 116, 181, 120);

    if (Application_ != nullptr && HostChromeLayoutCache_ != nullptr) {
        const auto& actions = Application_->GetTitleBarActionButtons();
        for (const auto& actionLayout : HostChromeLayoutCache_->ActionButtons) {
            if (actionLayout.ActionIndex < 0 || actionLayout.ActionIndex >= static_cast<int>(actions.size())) {
                continue;
            }

            const auto& action = actions[static_cast<std::size_t>(actionLayout.ActionIndex)];
            const bool bEnabled = !action.IsEnabled || action.IsEnabled();
            const bool bHighlighted = action.IsHighlighted && action.IsHighlighted();
            const bool bHovered = HoveredTitleBarActionIndex_ == actionLayout.ActionIndex;
            const bool bPressed = PressedTitleBarActionIndex_ == actionLayout.ActionIndex;

            ImU32 fillColor = buttonBaseColor;
            if (bHighlighted) {
                fillColor = actionHighlightColor;
            }
            if (!bEnabled) {
                fillColor = IM_COL32(0, 0, 0, 0);
            } else if (bPressed) {
                fillColor = actionPressedColor;
            } else if (bHovered) {
                fillColor = actionHoverColor;
            }

            if ((fillColor >> IM_COL32_A_SHIFT) != 0) {
                drawList->AddRectFilled(
                    actionLayout.Geometry.GetMin().ToImVec2(),
                    actionLayout.Geometry.GetMax().ToImVec2(),
                    fillColor,
                    5.0f);
            }

            if (action.Icon.IsValid()) {
                drawList->AddImage(
                    action.Icon.TextureId,
                    actionLayout.Geometry.GetMin().ToImVec2(),
                    actionLayout.Geometry.GetMax().ToImVec2(),
                    action.Icon.Uv0.ToImVec2(),
                    action.Icon.Uv1.ToImVec2(),
                    (bEnabled ? action.Icon.TintColor : FColor::FromBytes(140, 146, 156)).ToImU32());
            }
        }
    }

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

void ImWin32DX11Backend::DestroyWindowIcons()
{
    if (SmallWindowIcon_ != nullptr) {
        DestroyIcon(SmallWindowIcon_);
        SmallWindowIcon_ = nullptr;
    }

    if (LargeWindowIcon_ != nullptr) {
        DestroyIcon(LargeWindowIcon_);
        LargeWindowIcon_ = nullptr;
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
            if (HitTestTitleBarActionButton(clientPoint) != InvalidHostChromeIndex) {
                return HTCLIENT;
            }
            if (HitTestTitleBarTab(clientPoint) != InvalidHostChromeIndex) {
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
            const int hoveredTabIndex = HitTestTitleBarTab(clientPoint);
            const int hoveredActionIndex = HitTestTitleBarActionButton(clientPoint);
            if (HoveredHostChromeButton_ != hoveredButton ||
                HoveredTitleBarTabIndex_ != hoveredTabIndex ||
                HoveredTitleBarActionIndex_ != hoveredActionIndex) {
                HoveredHostChromeButton_ = hoveredButton;
                HoveredTitleBarTabIndex_ = hoveredTabIndex;
                HoveredTitleBarActionIndex_ = hoveredActionIndex;
                InvalidateRect(hWnd, nullptr, FALSE);
            } else if (PressedHostChromeButton_ != EHostChromeButton::None ||
                       PressedTitleBarTabIndex_ != InvalidHostChromeIndex ||
                       PressedTitleBarActionIndex_ != InvalidHostChromeIndex ||
                       clientPoint.y < GetHostChromeHeight()) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_MOUSELEAVE:
        case WM_CAPTURECHANGED:
            if (HoveredHostChromeButton_ != EHostChromeButton::None ||
                PressedHostChromeButton_ != EHostChromeButton::None ||
                HoveredTitleBarTabIndex_ != InvalidHostChromeIndex ||
                PressedTitleBarTabIndex_ != InvalidHostChromeIndex ||
                HoveredTitleBarActionIndex_ != InvalidHostChromeIndex ||
                PressedTitleBarActionIndex_ != InvalidHostChromeIndex) {
                HoveredHostChromeButton_ = EHostChromeButton::None;
                HoveredTitleBarTabIndex_ = InvalidHostChromeIndex;
                HoveredTitleBarActionIndex_ = InvalidHostChromeIndex;
                if (msg == WM_CAPTURECHANGED) {
                    PressedHostChromeButton_ = EHostChromeButton::None;
                    PressedTitleBarTabIndex_ = InvalidHostChromeIndex;
                    PressedTitleBarActionIndex_ = InvalidHostChromeIndex;
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
        if (CloseRequestedHandler_ && !CloseRequestedHandler_()) {
            return 0;
        }
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
