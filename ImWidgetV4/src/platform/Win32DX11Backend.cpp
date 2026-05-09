#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/core/Widget.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <algorithm>
#include <cmath>
#include <codecvt>
#include <locale>
#include <shobjidl.h>
#include <sstream>

// 闂佸憡鎸哥粔鎾箖濠婂嫮鐝堕柣妤€鐗婇～?ImGui Win32 缂備焦鍔栭〃鍛般亹濞戞瑦浜ら柛銉㈡杹閺屻倕顭跨捄鍝勵伀闁诡喖锕畷娆撴嚍閵夛附顔?
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ImWidgetV4 {

namespace {

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

} // namespace

// ========== 闂佸搫顑呯€氫即鍩€椤掑倸孝闁搞倝浜跺顐﹀级閹稿骸顏梺鍝勵儐閸旀牠鎮楅鐐茬闁兼亽鍎插▓?==========

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
    , Application_(nullptr)
{
}

ImWin32DX11Backend::~ImWin32DX11Backend() {
    Shutdown();
}

// ========== ImApplicationBackend 闂佽浜介崕杈亹濞戞埃鍋撻崷顓炰户妤?==========

bool ImWin32DX11Backend::Initialize() {
    if (bWindowClassRegistered_ || Hwnd_ != nullptr || bImGuiBackendInitialized_) {
        return true;
    }

    // 1. Register the Win32 window class.
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

    const DWORD windowStyle = GetResolvedWindowStyle();
    RECT windowRect = {0, 0, WindowWidth_, WindowHeight_};
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
        this);

    if (!Hwnd_) {
        return false;
    }

    // 3. 闂佸憡甯楃换鍌烇綖閹版澘绀?DirectX 11
    if (!CreateDeviceD3D()) {
        return false;
    }

    // Show the host window after the device is ready.
    ShowWindow(Hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(Hwnd_);

    // 5. 闂佸憡甯楃换鍌烇綖閹版澘绀?ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    bImGuiContextOwned_ = true;
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (Application_ != nullptr) {
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
        Application_->EnsureDefaultFontConfigured();
    }

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(Hwnd_);
    ImGui_ImplDX11_Init(D3DDevice_, D3DDeviceContext_);
    bImGuiBackendInitialized_ = true;

    return true;
}

void ImWin32DX11Backend::Shutdown() {
    DestroyWindowIcons();

    // 1. 濠电偞鎸搁幊鎰板箖?ImGui
    if (bImGuiBackendInitialized_) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        bImGuiBackendInitialized_ = false;
    }

    if (bImGuiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
        bImGuiContextOwned_ = false;
    }

    // 2. 濠电偞鎸搁幊鎰板箖?DirectX 11
    CleanupDeviceD3D();
    // Destroy the host window.
    if (Hwnd_ != nullptr) {
        if (::IsWindow(Hwnd_)) {
            DestroyWindow(Hwnd_);
        }
        Hwnd_ = nullptr;
    }

    if (bWindowClassRegistered_) {
        UnregisterClassW(L"ImWidgetV4WindowClass", HInstance_);
        bWindowClassRegistered_ = false;
    }
}

void ImWin32DX11Backend::Run() {
    MSG msg = {};

    while (!bShouldClose_) {
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

        // 2. 婵犮垼娉涚€氼噣骞冩繝鍕幓婵°倐鍋撶憸鐗堢洴閺屽棝顢欓挊澶屾
        if (bSwapChainOccluded_ &&
            SwapChain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        bSwapChainOccluded_ = false;

        // 3. Handle pending host resize before starting the frame.
        HandleResize();

        // 4. Start a new ImGui frame.
        BeginFrame();

        // 5. Advance and paint the retained-mode application tree.
        FFrameInfo frameInfo;
        bool bHasFrameInfo = false;
        if (Application_) {
            ImGuiIO& io = ImGui::GetIO();
            FImGuiInputSnapshot inputSnapshot;
            PopulateImGuiInputSnapshotFromIo(io, inputSnapshot);
            InputSource_.SetSnapshot(inputSnapshot);
            RECT clientRect = {};
            GetClientRect(Hwnd_, &clientRect);

            const float viewportWidth = static_cast<float>(std::max(0L, clientRect.right - clientRect.left));
            const float viewportHeight = static_cast<float>(std::max(0L, clientRect.bottom - clientRect.top));
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(viewportWidth, viewportHeight), ImGuiCond_Always);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            const ImGuiWindowFlags canvasFlags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoInputs;
            const bool bCanvasOpen = ImGui::Begin("##ImWidgetV4HostCanvas", nullptr, canvasFlags);
            ImDrawList* canvasDrawList = bCanvasOpen ? ImGui::GetWindowDrawList() : nullptr;
            DrawContext drawContext(canvasDrawList);

            FFrameContext frameContext;
            frameContext.FrameInfo.ViewportPosition = FVector2(0.0f, 0.0f);
            frameContext.FrameInfo.ViewportSize = FVector2(viewportWidth, viewportHeight);
            frameContext.FrameInfo.DeltaTime = io.DeltaTime;
            frameContext.FrameInfo.CurrentTime = ImGui::GetTime();
            frameContext.DrawContext_ = &drawContext;
            frameContext.InputSource = &InputSource_;

            frameInfo = frameContext.FrameInfo;
            bHasFrameInfo = true;
            Application_->AdvanceFrame(frameContext);

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }

        // 6. Render and present the frame.
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
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImWin32DX11Backend::EndFrame() {
    // 1. 濠电偞鎸稿鍫曟偂?ImGui
    ImGui::Render();

    const float clearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};
    D3DDeviceContext_->OMSetRenderTargets(1, &MainRenderTargetView_, nullptr);
    D3DDeviceContext_->ClearRenderTargetView(MainRenderTargetView_, clearColor);

    // 3. 濠电偞鎸稿鍫曟偂?ImGui 缂傚倷鐒﹂敋闁糕晜顨婂顐︽偋閸繄銈?
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

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

// ========== DirectX 11 闁荤姳鐒﹂崕鎶剿囬鍌滀笉闁挎稑瀚崐?==========

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

    if (Hwnd_ != nullptr) {
        ApplyWindowStyle();
        SetWindowSize(WindowWidth_, WindowHeight_);
        InvalidateRect(Hwnd_, nullptr, FALSE);
    }
}

bool ImWin32DX11Backend::SupportsHostWindowDrag() const
{
    return bUseCustomHostChrome_ && Hwnd_ != nullptr;
}

bool ImWin32DX11Backend::SupportsHostWindowMinimize() const
{
    return bUseCustomHostChrome_ && Hwnd_ != nullptr;
}

bool ImWin32DX11Backend::SupportsHostWindowMaximize() const
{
    return bUseCustomHostChrome_ && Hwnd_ != nullptr;
}

bool ImWin32DX11Backend::SupportsHostWindowClose() const
{
    return bUseCustomHostChrome_ && Hwnd_ != nullptr;
}

bool ImWin32DX11Backend::IsHostWindowMaximized() const
{
    return Hwnd_ != nullptr && ::IsZoomed(Hwnd_) != FALSE;
}

bool ImWin32DX11Backend::BeginHostWindowDrag()
{
    if (!SupportsHostWindowDrag()) {
        return false;
    }

    ::ReleaseCapture();
    ::SendMessageW(Hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, 0);
    return true;
}

bool ImWin32DX11Backend::MinimizeHostWindow()
{
    if (!SupportsHostWindowMinimize()) {
        return false;
    }

    ::ShowWindow(Hwnd_, SW_MINIMIZE);
    return true;
}

bool ImWin32DX11Backend::ToggleHostWindowMaximize()
{
    if (!SupportsHostWindowMaximize()) {
        return false;
    }

    ::ShowWindow(Hwnd_, ::IsZoomed(Hwnd_) ? SW_RESTORE : SW_MAXIMIZE);
    return true;
}

bool ImWin32DX11Backend::CloseHostWindow()
{
    if (!SupportsHostWindowClose()) {
        return false;
    }

    RequestClose();
    return true;
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

    // 2. Create the D3D device and swap chain.
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

    // 4. Create the render target for the swap chain.
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

// ========== 缂備焦鍔栭〃鍛般亹濞戞瑦浜ら柛銉㈡杹閺?==========

LRESULT CALLBACK ImWin32DX11Backend::WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
            return HTCLIENT;
        }
        }
    }

    // 1. 闁哄鍎愰崜娆掋亹閸屾粎纾?ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    // 2. 婵犮垼娉涚€氼噣骞冩繝鍕幓婵°倐鍋撶憸鐗堢☉閳藉宕奸悢鍓佺
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
            return 0;  // Disable the ALT application menu.
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
