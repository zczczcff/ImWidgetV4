#include <imwidgetv4/platform/AndroidGLES3Backend.h>

#if defined(__ANDROID__)

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <backends/imgui_impl_android.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <jni.h>
#include <utility>

namespace ImWidgetV4 {

namespace {

constexpr const char* GAndroidBackendLogTag = "ImWidgetV4";
constexpr const char* GDefaultImGuiIniFileName = "imwidgetv4_imgui.ini";

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

void LogAndroidError(const char* message)
{
    __android_log_print(ANDROID_LOG_ERROR, GAndroidBackendLogTag, "%s", message);
}

ImTextureID MakeTextureId(GLuint texture)
{
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture));
}

ImGuiKey ResolveImGuiKeyFromAndroidKeyCode(int32_t keyCode)
{
    switch (keyCode) {
    case AKEYCODE_DEL:
        return ImGuiKey_Backspace;
    case AKEYCODE_FORWARD_DEL:
        return ImGuiKey_Delete;
    case AKEYCODE_ENTER:
    case AKEYCODE_NUMPAD_ENTER:
        return ImGuiKey_Enter;
    case AKEYCODE_TAB:
        return ImGuiKey_Tab;
    case AKEYCODE_SPACE:
        return ImGuiKey_Space;
    case AKEYCODE_ESCAPE:
        return ImGuiKey_Escape;
    case AKEYCODE_DPAD_LEFT:
        return ImGuiKey_LeftArrow;
    case AKEYCODE_DPAD_RIGHT:
        return ImGuiKey_RightArrow;
    case AKEYCODE_DPAD_UP:
        return ImGuiKey_UpArrow;
    case AKEYCODE_DPAD_DOWN:
        return ImGuiKey_DownArrow;
    case AKEYCODE_MOVE_HOME:
        return ImGuiKey_Home;
    case AKEYCODE_MOVE_END:
        return ImGuiKey_End;
    case AKEYCODE_PAGE_UP:
        return ImGuiKey_PageUp;
    case AKEYCODE_PAGE_DOWN:
        return ImGuiKey_PageDown;
    case AKEYCODE_A:
        return ImGuiKey_A;
    case AKEYCODE_B:
        return ImGuiKey_B;
    case AKEYCODE_C:
        return ImGuiKey_C;
    case AKEYCODE_D:
        return ImGuiKey_D;
    case AKEYCODE_E:
        return ImGuiKey_E;
    case AKEYCODE_F:
        return ImGuiKey_F;
    case AKEYCODE_G:
        return ImGuiKey_G;
    case AKEYCODE_H:
        return ImGuiKey_H;
    case AKEYCODE_I:
        return ImGuiKey_I;
    case AKEYCODE_J:
        return ImGuiKey_J;
    case AKEYCODE_K:
        return ImGuiKey_K;
    case AKEYCODE_L:
        return ImGuiKey_L;
    case AKEYCODE_M:
        return ImGuiKey_M;
    case AKEYCODE_N:
        return ImGuiKey_N;
    case AKEYCODE_O:
        return ImGuiKey_O;
    case AKEYCODE_P:
        return ImGuiKey_P;
    case AKEYCODE_Q:
        return ImGuiKey_Q;
    case AKEYCODE_R:
        return ImGuiKey_R;
    case AKEYCODE_S:
        return ImGuiKey_S;
    case AKEYCODE_T:
        return ImGuiKey_T;
    case AKEYCODE_U:
        return ImGuiKey_U;
    case AKEYCODE_V:
        return ImGuiKey_V;
    case AKEYCODE_W:
        return ImGuiKey_W;
    case AKEYCODE_X:
        return ImGuiKey_X;
    case AKEYCODE_Y:
        return ImGuiKey_Y;
    case AKEYCODE_Z:
        return ImGuiKey_Z;
    case AKEYCODE_0:
    case AKEYCODE_NUMPAD_0:
        return ImGuiKey_0;
    case AKEYCODE_1:
    case AKEYCODE_NUMPAD_1:
        return ImGuiKey_1;
    case AKEYCODE_2:
    case AKEYCODE_NUMPAD_2:
        return ImGuiKey_2;
    case AKEYCODE_3:
    case AKEYCODE_NUMPAD_3:
        return ImGuiKey_3;
    case AKEYCODE_4:
    case AKEYCODE_NUMPAD_4:
        return ImGuiKey_4;
    case AKEYCODE_5:
    case AKEYCODE_NUMPAD_5:
        return ImGuiKey_5;
    case AKEYCODE_6:
    case AKEYCODE_NUMPAD_6:
        return ImGuiKey_6;
    case AKEYCODE_7:
    case AKEYCODE_NUMPAD_7:
        return ImGuiKey_7;
    case AKEYCODE_8:
    case AKEYCODE_NUMPAD_8:
        return ImGuiKey_8;
    case AKEYCODE_9:
    case AKEYCODE_NUMPAD_9:
        return ImGuiKey_9;
    case AKEYCODE_F1:
        return ImGuiKey_F1;
    case AKEYCODE_F2:
        return ImGuiKey_F2;
    case AKEYCODE_F3:
        return ImGuiKey_F3;
    case AKEYCODE_F4:
        return ImGuiKey_F4;
    case AKEYCODE_F5:
        return ImGuiKey_F5;
    case AKEYCODE_F6:
        return ImGuiKey_F6;
    case AKEYCODE_F7:
        return ImGuiKey_F7;
    case AKEYCODE_F8:
        return ImGuiKey_F8;
    case AKEYCODE_F9:
        return ImGuiKey_F9;
    case AKEYCODE_F10:
        return ImGuiKey_F10;
    case AKEYCODE_F11:
        return ImGuiKey_F11;
    case AKEYCODE_F12:
        return ImGuiKey_F12;
    default:
        return ImGuiKey_None;
    }
}

ImGuiKey ResolveImGuiModifierKeyFromAndroidKeyCode(int32_t keyCode)
{
    switch (keyCode) {
    case AKEYCODE_CTRL_LEFT:
    case AKEYCODE_CTRL_RIGHT:
        return ImGuiMod_Ctrl;
    case AKEYCODE_SHIFT_LEFT:
    case AKEYCODE_SHIFT_RIGHT:
        return ImGuiMod_Shift;
    case AKEYCODE_ALT_LEFT:
    case AKEYCODE_ALT_RIGHT:
        return ImGuiMod_Alt;
    case AKEYCODE_META_LEFT:
    case AKEYCODE_META_RIGHT:
        return ImGuiMod_Super;
    default:
        return ImGuiKey_None;
    }
}

bool TryLoadAssetBytes(AAssetManager* assetManager, const char* assetPath, std::vector<std::uint8_t>& outBytes)
{
    outBytes.clear();
    if (assetManager == nullptr || assetPath == nullptr || assetPath[0] == '\0') {
        return false;
    }

    AAsset* asset = AAssetManager_open(assetManager, assetPath, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        return false;
    }

    const off_t assetLength = AAsset_getLength(asset);
    if (assetLength <= 0) {
        AAsset_close(asset);
        return false;
    }

    outBytes.resize(static_cast<std::size_t>(assetLength));
    const int readLength = AAsset_read(asset, outBytes.data(), assetLength);
    AAsset_close(asset);
    if (readLength != assetLength) {
        outBytes.clear();
        return false;
    }

    return true;
}

bool ClearPendingJavaException(JNIEnv* env)
{
    if (env == nullptr || !env->ExceptionCheck()) {
        return false;
    }

    env->ExceptionClear();
    return true;
}

} // namespace

struct ImAndroidGLES3Backend::FScopedJniEnv {
    JNIEnv* Env = nullptr;
    JavaVM* Vm = nullptr;
    bool bAttached = false;

    explicit FScopedJniEnv(ANativeActivity* activity)
    {
        if (activity == nullptr || activity->vm == nullptr) {
            return;
        }

        Vm = activity->vm;
        if (Vm->GetEnv(reinterpret_cast<void**>(&Env), JNI_VERSION_1_6) == JNI_OK) {
            return;
        }

#if defined(__ANDROID__)
        if (Vm->AttachCurrentThread(&Env, nullptr) == JNI_OK) {
            bAttached = true;
        }
#endif
    }

    ~FScopedJniEnv()
    {
        if (bAttached && Vm != nullptr) {
            Vm->DetachCurrentThread();
        }
    }

    bool IsValid() const
    {
        return Env != nullptr;
    }
};

ImAndroidGLES3Backend::ImAndroidGLES3Backend(
    android_app* app,
    std::string windowTitle,
    const std::string& glslVersion)
    : App_(app)
    , WindowTitle_(std::move(windowTitle))
    , GlslVersion_(glslVersion)
{
}

ImAndroidGLES3Backend::~ImAndroidGLES3Backend()
{
    Shutdown();
}

bool ImAndroidGLES3Backend::Initialize()
{
    if (bInitialized_) {
        return true;
    }

    if (App_ == nullptr) {
        LogAndroidError("Android backend initialization failed: android_app was null.");
        return false;
    }

    App_->userData = this;
    App_->onAppCmd = HandleAppCommandThunk;
    App_->onInputEvent = HandleInputEventThunk;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    bImGuiContextOwned_ = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ConfigureImGuiPlatformIo();

    if (Application_ != nullptr) {
        ConfigureApplicationForAndroid();
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
        Application_->EnsureDefaultFontConfigured();
    }

    ImGui::StyleColorsDark();

    bInitialized_ = true;
    return true;
}

void ImAndroidGLES3Backend::Shutdown()
{
    if (!bInitialized_ && !bImGuiContextOwned_ && Display_ == EGL_NO_DISPLAY) {
        return;
    }

    ShutdownImGuiBackends();
    ReleaseRuntimeTextures();
    ShutdownDisplay();

    if (bImGuiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }

    bImGuiContextOwned_ = false;
    bInitialized_ = false;
    NativeWindow_ = nullptr;
    ClipboardTextCache_.clear();
    bSoftKeyboardVisible_ = false;
    SetNativeBackendHandle(0);

    if (App_ != nullptr && App_->userData == this) {
        App_->userData = nullptr;
    }
}

void ImAndroidGLES3Backend::Run()
{
    if (!Initialize()) {
        return;
    }

    while (!bShouldClose_) {
        const int timeoutMillis = CanRender() ? 0 : -1;
        PumpEvents(timeoutMillis);
        if (bShouldClose_) {
            break;
        }

        if (App_ == nullptr || App_->window == nullptr || !bAppResumed_) {
            continue;
        }

        if (!EnsureDisplayInitialized()) {
            continue;
        }

        if (!EnsureImGuiBackendsInitialized() || !CanRender()) {
            continue;
        }

        BeginFrame();
        AdvanceApplicationFrame();
        EndFrame();
    }
}

bool ImAndroidGLES3Backend::ShouldClose() const
{
    return bShouldClose_;
}

void ImAndroidGLES3Backend::SetWindowTitle(const std::string& title)
{
    WindowTitle_ = title;
    const std::uint8_t* rgbaPixels = TaskDescriptionIconPixels_.empty() ? nullptr : TaskDescriptionIconPixels_.data();
    UpdateTaskDescription(rgbaPixels, TaskDescriptionIconWidth_, TaskDescriptionIconHeight_);
}

void ImAndroidGLES3Backend::SetWindowSize(int width, int height)
{
    WindowWidth_ = std::max(0, width);
    WindowHeight_ = std::max(0, height);
}

void ImAndroidGLES3Backend::GetWindowSize(int& width, int& height) const
{
    width = WindowWidth_;
    height = WindowHeight_;
}

void ImAndroidGLES3Backend::BeginFrame()
{
    // Android may resize the EGL window surface after device rotation without
    // recreating the backend object. Refresh the cached surface size each frame
    // so layout and viewport use the current orientation dimensions.
    UpdateSurfaceSize();
    FlushPendingJavaInputToImGui();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
}

void ImAndroidGLES3Backend::EndFrame()
{
    ImGui::Render();

    glViewport(0, 0, WindowWidth_, WindowHeight_);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(Display_, Surface_);
}

void ImAndroidGLES3Backend::SetApplication(ImApplication* app)
{
    Application_ = app;
    if (Application_ != nullptr) {
        ConfigureApplicationForAndroid();
        Application_->SetBackend(this);
        Application_->EnsureDefaultFontConfigured();
        Application_->SetIniSettingsPath(Application_->GetIniSettingsPath());
    }
}

ImApplication* ImAndroidGLES3Backend::GetApplication() const
{
    return Application_;
}

void ImAndroidGLES3Backend::RequestClose()
{
    bShouldClose_ = true;
    if (App_ != nullptr) {
        App_->destroyRequested = 1;
    }
}

std::string ImAndroidGLES3Backend::GetBackendName() const
{
    return "Android + OpenGL ES 3";
}

void ImAndroidGLES3Backend::SetTouchScrollWheelEnabled(bool bEnabled)
{
    if (bTouchScrollWheelEnabled_ == bEnabled) {
        return;
    }

    bTouchScrollWheelEnabled_ = bEnabled;
    if (!bTouchScrollWheelEnabled_) {
        ResetTouchScrollWheelGesture();
    }
}

bool ImAndroidGLES3Backend::IsTouchScrollWheelEnabled() const
{
    return bTouchScrollWheelEnabled_;
}

void ImAndroidGLES3Backend::SetTouchScrollWheelInverted(bool bInverted)
{
    bTouchScrollWheelInverted_ = bInverted;
}

bool ImAndroidGLES3Backend::IsTouchScrollWheelInverted() const
{
    return bTouchScrollWheelInverted_;
}

ImTextureID ImAndroidGLES3Backend::CreateTextureFromRGBA(
    const std::uint8_t* rgbaPixels,
    int width,
    int height)
{
    if (rgbaPixels == nullptr || width <= 0 || height <= 0 || !CanRender()) {
        return nullptr;
    }

    GLuint texture = 0U;
    glGenTextures(1, &texture);
    if (texture == 0U) {
        return nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgbaPixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    const ImTextureID textureId = MakeTextureId(texture);
    RuntimeTextures_[textureId] = texture;
    return textureId;
}

void ImAndroidGLES3Backend::ReleaseTexture(ImTextureID textureId)
{
    const auto found = RuntimeTextures_.find(textureId);
    if (found == RuntimeTextures_.end()) {
        return;
    }

    const GLuint texture = found->second;
    if (texture != 0U) {
        glDeleteTextures(1, &texture);
    }

    RuntimeTextures_.erase(found);
}

bool ImAndroidGLES3Backend::EnsureDisplayInitialized()
{
    if (Display_ != EGL_NO_DISPLAY &&
        Surface_ != EGL_NO_SURFACE &&
        Context_ != EGL_NO_CONTEXT) {
        return true;
    }

    if (App_ == nullptr || App_->window == nullptr) {
        return false;
    }

    NativeWindow_ = App_->window;

    Display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (Display_ == EGL_NO_DISPLAY) {
        LogAndroidError("eglGetDisplay failed.");
        ShutdownDisplay();
        return false;
    }

    if (eglInitialize(Display_, nullptr, nullptr) != EGL_TRUE) {
        LogAndroidError("eglInitialize failed.");
        ShutdownDisplay();
        return false;
    }

    const std::array<EGLint, 13> attributes = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };

    EGLint configCount = 0;
    if (eglChooseConfig(Display_, attributes.data(), &Config_, 1, &configCount) != EGL_TRUE || configCount <= 0) {
        LogAndroidError("eglChooseConfig failed.");
        ShutdownDisplay();
        return false;
    }

    EGLint nativeVisualId = 0;
    eglGetConfigAttrib(Display_, Config_, EGL_NATIVE_VISUAL_ID, &nativeVisualId);
    ANativeWindow_setBuffersGeometry(NativeWindow_, 0, 0, nativeVisualId);

    const EGLint contextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    Context_ = eglCreateContext(Display_, Config_, EGL_NO_CONTEXT, contextAttributes);
    if (Context_ == EGL_NO_CONTEXT) {
        LogAndroidError("eglCreateContext failed.");
        ShutdownDisplay();
        return false;
    }

    Surface_ = eglCreateWindowSurface(Display_, Config_, NativeWindow_, nullptr);
    if (Surface_ == EGL_NO_SURFACE) {
        LogAndroidError("eglCreateWindowSurface failed.");
        ShutdownDisplay();
        return false;
    }

    if (eglMakeCurrent(Display_, Surface_, Surface_, Context_) != EGL_TRUE) {
        LogAndroidError("eglMakeCurrent failed.");
        ShutdownDisplay();
        return false;
    }

    UpdateSurfaceSize();
    return true;
}

void ImAndroidGLES3Backend::FlushPendingJavaInputToImGui()
{
    std::vector<unsigned int> textInput;
    std::vector<std::pair<int32_t, bool>> specialKeys;
    {
        std::lock_guard<std::mutex> lock(PendingJavaInputMutex_);
        textInput.swap(PendingJavaTextInput_);
        specialKeys.swap(PendingJavaSpecialKeys_);
    }

    if (textInput.empty() && specialKeys.empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    for (const auto& [keyCode, bDown] : specialKeys) {
        const ImGuiKey modifierKey = ResolveImGuiModifierKeyFromAndroidKeyCode(keyCode);
        if (modifierKey != ImGuiKey_None) {
            io.AddKeyEvent(modifierKey, bDown);
            continue;
        }

        const ImGuiKey key = ResolveImGuiKeyFromAndroidKeyCode(keyCode);
        if (key != ImGuiKey_None) {
            io.AddKeyEvent(key, bDown);
        }
    }

    for (unsigned int codepoint : textInput) {
        io.AddInputCharacter(codepoint);
    }
}

void ImAndroidGLES3Backend::ShutdownDisplay()
{
    ReleaseRuntimeTextures();

    if (Display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(Display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    if (Surface_ != EGL_NO_SURFACE && Display_ != EGL_NO_DISPLAY) {
        eglDestroySurface(Display_, Surface_);
    }
    if (Context_ != EGL_NO_CONTEXT && Display_ != EGL_NO_DISPLAY) {
        eglDestroyContext(Display_, Context_);
    }
    if (Display_ != EGL_NO_DISPLAY) {
        eglTerminate(Display_);
    }

    Display_ = EGL_NO_DISPLAY;
    Surface_ = EGL_NO_SURFACE;
    Context_ = EGL_NO_CONTEXT;
    Config_ = nullptr;
    NativeWindow_ = nullptr;
    WindowWidth_ = 0;
    WindowHeight_ = 0;
}

void ImAndroidGLES3Backend::ReleaseRuntimeTextures()
{
    if (!RuntimeTextures_.empty() &&
        Display_ != EGL_NO_DISPLAY &&
        Surface_ != EGL_NO_SURFACE &&
        Context_ != EGL_NO_CONTEXT) {
        for (const auto& entry : RuntimeTextures_) {
            const GLuint texture = entry.second;
            if (texture != 0U) {
                glDeleteTextures(1, &texture);
            }
        }
    }

    RuntimeTextures_.clear();

    if (Application_ != nullptr) {
        Application_->NotifyBackendTextureResourcesLost();
    }
}

bool ImAndroidGLES3Backend::EnsureImGuiBackendsInitialized()
{
    if (!bImGuiPlatformInitialized_) {
        if (!ImGui_ImplAndroid_Init(NativeWindow_)) {
            LogAndroidError("ImGui_ImplAndroid_Init failed.");
            return false;
        }
        bImGuiPlatformInitialized_ = true;
    }

    if (!bImGuiRendererInitialized_) {
        if (!ImGui_ImplOpenGL3_Init(GlslVersion_.c_str())) {
            LogAndroidError("ImGui_ImplOpenGL3_Init failed.");
            return false;
        }
        bImGuiRendererInitialized_ = true;
    }

    return true;
}

void ImAndroidGLES3Backend::ShutdownImGuiBackends()
{
    if (bImGuiRendererInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        bImGuiRendererInitialized_ = false;
    }

    if (bImGuiPlatformInitialized_) {
        ImGui_ImplAndroid_Shutdown();
        bImGuiPlatformInitialized_ = false;
    }
}

bool ImAndroidGLES3Backend::CanRender() const
{
    return !bShouldClose_ &&
        App_ != nullptr &&
        App_->window != nullptr &&
        bAppResumed_ &&
        Display_ != EGL_NO_DISPLAY &&
        Surface_ != EGL_NO_SURFACE &&
        Context_ != EGL_NO_CONTEXT;
}

void ImAndroidGLES3Backend::PumpEvents(int timeoutMillis)
{
    int events = 0;
    android_poll_source* source = nullptr;
    while (ALooper_pollOnce(timeoutMillis, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source != nullptr) {
            source->process(App_, source);
        }

        if (App_ != nullptr && App_->destroyRequested != 0) {
            bShouldClose_ = true;
            return;
        }

        timeoutMillis = 0;
    }
}

void ImAndroidGLES3Backend::AdvanceApplicationFrame()
{
    if (Application_ == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    FImGuiInputSnapshot inputSnapshot;
    PopulateImGuiInputSnapshotFromIo(io, inputSnapshot);
    InputSource_.SetSnapshot(inputSnapshot);

    DrawContext drawContext(ImGui::GetBackgroundDrawList());

    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportPosition = FVector2(0.0f, 0.0f);
    frameContext.FrameInfo.ViewportSize = FVector2(
        static_cast<float>(WindowWidth_),
        static_cast<float>(WindowHeight_));
    frameContext.FrameInfo.DeltaTime = io.DeltaTime;
    frameContext.FrameInfo.CurrentTime = ImGui::GetTime();
    frameContext.DrawContext_ = &drawContext;
    frameContext.InputSource = &InputSource_;

    Application_->AdvanceFrame(frameContext);
    SyncSoftKeyboardVisibility();
}

void ImAndroidGLES3Backend::UpdateSurfaceSize()
{
    if (Display_ == EGL_NO_DISPLAY || Surface_ == EGL_NO_SURFACE) {
        WindowWidth_ = 0;
        WindowHeight_ = 0;
        return;
    }

    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(Display_, Surface_, EGL_WIDTH, &width);
    eglQuerySurface(Display_, Surface_, EGL_HEIGHT, &height);

    WindowWidth_ = std::max(0, static_cast<int>(width));
    WindowHeight_ = std::max(0, static_cast<int>(height));
}

void ImAndroidGLES3Backend::ConfigureApplicationForAndroid()
{
    if (Application_ == nullptr || App_ == nullptr || App_->activity == nullptr) {
        return;
    }

    if (Application_->GetIniSettingsPath().empty()) {
        const char* internalDataPath = App_->activity->internalDataPath;
        if (internalDataPath != nullptr && internalDataPath[0] != '\0') {
            Application_->SetIniSettingsPath(std::filesystem::path(internalDataPath) / GDefaultImGuiIniFileName);
        }
    }

    SetNativeBackendHandle(reinterpret_cast<std::intptr_t>(this));

    std::vector<std::uint8_t> fontData;
    AAssetManager* assetManager = App_->activity->assetManager;
    if (TryLoadAssetBytes(assetManager, "fonts/default.ttf", fontData) ||
        TryLoadAssetBytes(assetManager, "fonts/default.otf", fontData) ||
        TryLoadAssetBytes(assetManager, "fonts/NotoSansSC-Regular.otf", fontData)) {
        Application_->SetPreferredDefaultFontData(std::move(fontData));
        return;
    }

    Application_->SetPreferredDefaultFontCandidates({
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/NotoSansSC-Regular.otf",
        "/system/fonts/SourceHanSansSC-Regular.otf",
        "/system/fonts/DroidSansFallback.ttf",
        "/system/fonts/Roboto-Regular.ttf"
    });
}

void ImAndroidGLES3Backend::ConfigureImGuiPlatformIo()
{
    ImGuiIO& io = ImGui::GetIO();
    io.SetClipboardTextFn = SetClipboardTextThunk;
    io.GetClipboardTextFn = GetClipboardTextThunk;
    io.ClipboardUserData = this;
}

void ImAndroidGLES3Backend::SyncSoftKeyboardVisibility()
{
    const bool bShouldShowKeyboard = ShouldShowSoftKeyboard();
    if (bShouldShowKeyboard == bSoftKeyboardVisible_) {
        return;
    }

    if (bShouldShowKeyboard) {
        ShowSoftKeyboard();
    } else {
        HideSoftKeyboard();
    }

    bSoftKeyboardVisible_ = bShouldShowKeyboard;
}

bool ImAndroidGLES3Backend::ShouldShowSoftKeyboard() const
{
    if (Application_ == nullptr) {
        return false;
    }

    const std::shared_ptr<ImWidget>& focusedWidget = Application_->GetKeyboardFocus();
    if (!focusedWidget) {
        return false;
    }

    return dynamic_cast<ImEditableText*>(focusedWidget.get()) != nullptr;
}

void ImAndroidGLES3Backend::ShowSoftKeyboard()
{
    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return;
    }

    jmethodID methodId = jni.Env->GetMethodID(activityClass, "showNativeKeyboard", "()V");
    if (methodId != nullptr) {
        jni.Env->CallVoidMethod(App_->activity->clazz, methodId);
        ClearPendingJavaException(jni.Env);
    }

    jni.Env->DeleteLocalRef(activityClass);
}

void ImAndroidGLES3Backend::HideSoftKeyboard()
{
    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return;
    }

    jmethodID methodId = jni.Env->GetMethodID(activityClass, "hideNativeKeyboard", "()V");
    if (methodId != nullptr) {
        jni.Env->CallVoidMethod(App_->activity->clazz, methodId);
        ClearPendingJavaException(jni.Env);
    }

    jni.Env->DeleteLocalRef(activityClass);
}

bool ImAndroidGLES3Backend::UpdateTaskDescription(const std::uint8_t* rgbaPixels, int width, int height)
{
    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return false;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return false;
    }

    jmethodID methodId = jni.Env->GetMethodID(
        activityClass,
        "setNativeTaskDescription",
        "(Ljava/lang/String;[BII)V");
    if (methodId == nullptr) {
        ClearPendingJavaException(jni.Env);
        jni.Env->DeleteLocalRef(activityClass);
        return false;
    }

    const std::string label = WindowTitle_.empty() ? "ImWidgetV4 Android" : WindowTitle_;
    jstring labelString = jni.Env->NewStringUTF(label.c_str());
    jbyteArray pixelArray = nullptr;
    if (rgbaPixels != nullptr && width > 0 && height > 0) {
        const jsize byteCount = static_cast<jsize>(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);
        pixelArray = jni.Env->NewByteArray(byteCount);
        if (pixelArray != nullptr) {
            jni.Env->SetByteArrayRegion(
                pixelArray,
                0,
                byteCount,
                reinterpret_cast<const jbyte*>(rgbaPixels));
        }
    }

    jni.Env->CallVoidMethod(
        App_->activity->clazz,
        methodId,
        labelString,
        pixelArray,
        static_cast<jint>(width),
        static_cast<jint>(height));
    const bool bSuccess = !ClearPendingJavaException(jni.Env);

    if (pixelArray != nullptr) {
        jni.Env->DeleteLocalRef(pixelArray);
    }
    if (labelString != nullptr) {
        jni.Env->DeleteLocalRef(labelString);
    }
    jni.Env->DeleteLocalRef(activityClass);
    return bSuccess;
}

bool ImAndroidGLES3Backend::SetClipboardText(const std::string& text)
{
    ClipboardTextCache_ = text;

    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return false;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return false;
    }

    jmethodID methodId = jni.Env->GetMethodID(activityClass, "setNativeClipboardText", "(Ljava/lang/String;)V");
    if (methodId == nullptr) {
        ClearPendingJavaException(jni.Env);
        jni.Env->DeleteLocalRef(activityClass);
        return false;
    }

    jstring textString = jni.Env->NewStringUTF(text.c_str());
    jni.Env->CallVoidMethod(App_->activity->clazz, methodId, textString);
    const bool bSuccess = !ClearPendingJavaException(jni.Env);

    if (textString != nullptr) {
        jni.Env->DeleteLocalRef(textString);
    }
    jni.Env->DeleteLocalRef(activityClass);
    return bSuccess;
}

std::string ImAndroidGLES3Backend::GetClipboardText()
{
    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return ClipboardTextCache_;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return ClipboardTextCache_;
    }

    jmethodID methodId = jni.Env->GetMethodID(activityClass, "getNativeClipboardText", "()Ljava/lang/String;");
    if (methodId == nullptr) {
        ClearPendingJavaException(jni.Env);
        jni.Env->DeleteLocalRef(activityClass);
        return ClipboardTextCache_;
    }

    jstring textString = static_cast<jstring>(jni.Env->CallObjectMethod(App_->activity->clazz, methodId));
    if (ClearPendingJavaException(jni.Env) || textString == nullptr) {
        if (textString != nullptr) {
            jni.Env->DeleteLocalRef(textString);
        }
        jni.Env->DeleteLocalRef(activityClass);
        return ClipboardTextCache_;
    }

    const char* utfChars = jni.Env->GetStringUTFChars(textString, nullptr);
    if (utfChars != nullptr) {
        ClipboardTextCache_ = utfChars;
        jni.Env->ReleaseStringUTFChars(textString, utfChars);
    }

    jni.Env->DeleteLocalRef(textString);
    jni.Env->DeleteLocalRef(activityClass);
    return ClipboardTextCache_;
}

void ImAndroidGLES3Backend::SetNativeBackendHandle(std::intptr_t backendHandle)
{
    FScopedJniEnv jni(App_ != nullptr ? App_->activity : nullptr);
    if (!jni.IsValid() || App_ == nullptr || App_->activity == nullptr || App_->activity->clazz == nullptr) {
        return;
    }

    jclass activityClass = jni.Env->GetObjectClass(App_->activity->clazz);
    if (activityClass == nullptr) {
        ClearPendingJavaException(jni.Env);
        return;
    }

    jmethodID methodId = jni.Env->GetMethodID(activityClass, "setNativeBackendHandle", "(J)V");
    if (methodId != nullptr) {
        jni.Env->CallVoidMethod(
            App_->activity->clazz,
            methodId,
            static_cast<jlong>(backendHandle));
        ClearPendingJavaException(jni.Env);
    } else {
        ClearPendingJavaException(jni.Env);
    }

    jni.Env->DeleteLocalRef(activityClass);
}

void ImAndroidGLES3Backend::EnqueueJavaTextInput(unsigned int codepoint)
{
    if (codepoint == 0U) {
        return;
    }

    std::lock_guard<std::mutex> lock(PendingJavaInputMutex_);
    PendingJavaTextInput_.push_back(codepoint);
}

void ImAndroidGLES3Backend::EnqueueJavaSpecialKey(int32_t keyCode, bool bDown)
{
    std::lock_guard<std::mutex> lock(PendingJavaInputMutex_);
    PendingJavaSpecialKeys_.push_back({keyCode, bDown});
}

bool ImAndroidGLES3Backend::TryHandleTouchScrollWheelGesture(AInputEvent* inputEvent)
{
    if (!bTouchScrollWheelEnabled_ ||
        inputEvent == nullptr ||
        AInputEvent_getType(inputEvent) != AINPUT_EVENT_TYPE_MOTION) {
        return false;
    }

    const int32_t action = AMotionEvent_getAction(inputEvent);
    const int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    const int32_t pointerCount = AMotionEvent_getPointerCount(inputEvent);

    switch (actionMasked) {
    case AMOTION_EVENT_ACTION_DOWN: {
        if (pointerCount <= 0) {
            ResetTouchScrollWheelGesture();
            return false;
        }

        bTouchScrollWheelPointerDown_ = true;
        bTouchScrollWheelGestureActive_ = false;
        TouchScrollWheelPointerId_ = AMotionEvent_getPointerId(inputEvent, 0);
        TouchScrollWheelDownPosition_ = FVector2(
            AMotionEvent_getX(inputEvent, 0),
            AMotionEvent_getY(inputEvent, 0));
        TouchScrollWheelLastPosition_ = TouchScrollWheelDownPosition_;
        return false;
    }

    case AMOTION_EVENT_ACTION_POINTER_DOWN:
        ResetTouchScrollWheelGesture();
        return false;

    case AMOTION_EVENT_ACTION_MOVE: {
        if (!bTouchScrollWheelPointerDown_ || TouchScrollWheelPointerId_ < 0 || pointerCount != 1) {
            return false;
        }

        int32_t pointerIndex = -1;
        for (int32_t index = 0; index < pointerCount; ++index) {
            if (AMotionEvent_getPointerId(inputEvent, index) == TouchScrollWheelPointerId_) {
                pointerIndex = index;
                break;
            }
        }

        if (pointerIndex < 0) {
            ResetTouchScrollWheelGesture();
            return false;
        }

        const FVector2 currentPosition(
            AMotionEvent_getX(inputEvent, pointerIndex),
            AMotionEvent_getY(inputEvent, pointerIndex));
        const FVector2 totalDelta = currentPosition - TouchScrollWheelDownPosition_;
        const FVector2 frameDelta = currentPosition - TouchScrollWheelLastPosition_;

        if (!bTouchScrollWheelGestureActive_) {
            if (Application_ != nullptr && Application_->GetMouseCapture() != nullptr) {
                TouchScrollWheelLastPosition_ = currentPosition;
                return false;
            }

            const bool bVerticalDragDominant =
                std::fabs(totalDelta.Y) >= TouchScrollWheelActivationThreshold_ &&
                std::fabs(totalDelta.Y) >= std::fabs(totalDelta.X) * 1.25f;
            if (!bVerticalDragDominant) {
                TouchScrollWheelLastPosition_ = currentPosition;
                return false;
            }

            ImGuiIO& io = ImGui::GetIO();
            io.AddMousePosEvent(currentPosition.X, currentPosition.Y);
            io.AddMouseButtonEvent(0, false);
            bTouchScrollWheelGestureActive_ = true;
            TouchScrollWheelLastPosition_ = currentPosition;
            return true;
        }

        const float wheelDeltaY =
            (bTouchScrollWheelInverted_ ? -1.0f : 1.0f) *
            (frameDelta.Y / TouchScrollWheelPixelsPerWheelStep_);
        TouchScrollWheelLastPosition_ = currentPosition;

        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(currentPosition.X, currentPosition.Y);
        if (wheelDeltaY != 0.0f) {
            io.AddMouseWheelEvent(0.0f, wheelDeltaY);
        }
        return true;
    }

    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL: {
        const bool bConsumed = bTouchScrollWheelGestureActive_;
        ResetTouchScrollWheelGesture();
        return bConsumed;
    }

    case AMOTION_EVENT_ACTION_POINTER_UP: {
        const int32_t pointerIndex =
            (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const bool bTrackedPointerReleased =
            pointerIndex >= 0 &&
            pointerIndex < pointerCount &&
            AMotionEvent_getPointerId(inputEvent, pointerIndex) == TouchScrollWheelPointerId_;
        const bool bConsumed = bTouchScrollWheelGestureActive_ && bTrackedPointerReleased;
        if (bTrackedPointerReleased) {
            ResetTouchScrollWheelGesture();
        }
        return bConsumed;
    }

    default:
        return false;
    }
}

void ImAndroidGLES3Backend::ResetTouchScrollWheelGesture()
{
    bTouchScrollWheelGestureActive_ = false;
    bTouchScrollWheelPointerDown_ = false;
    TouchScrollWheelPointerId_ = -1;
    TouchScrollWheelDownPosition_ = FVector2(0.0f, 0.0f);
    TouchScrollWheelLastPosition_ = FVector2(0.0f, 0.0f);
}

void ImAndroidGLES3Backend::HandleAppCommand(int32_t command)
{
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        NativeWindow_ = App_ != nullptr ? App_->window : nullptr;
        EnsureDisplayInitialized();
        EnsureImGuiBackendsInitialized();
        break;

    case APP_CMD_TERM_WINDOW:
        ShutdownImGuiBackends();
        ShutdownDisplay();
        break;

    case APP_CMD_GAINED_FOCUS:
        bHasFocus_ = true;
        break;

    case APP_CMD_LOST_FOCUS:
        bHasFocus_ = false;
        HideSoftKeyboard();
        bSoftKeyboardVisible_ = false;
        ResetTouchScrollWheelGesture();
        break;

    case APP_CMD_RESUME:
        bAppResumed_ = true;
        break;

    case APP_CMD_PAUSE:
        bAppResumed_ = false;
        HideSoftKeyboard();
        bSoftKeyboardVisible_ = false;
        ResetTouchScrollWheelGesture();
        break;

    case APP_CMD_DESTROY:
        bShouldClose_ = true;
        break;

    default:
        break;
    }
}

int32_t ImAndroidGLES3Backend::HandleInputEvent(AInputEvent* inputEvent)
{
    if (inputEvent == nullptr) {
        return 0;
    }

    if (TryHandleTouchScrollWheelGesture(inputEvent)) {
        return 1;
    }

    if (AInputEvent_getType(inputEvent) == AINPUT_EVENT_TYPE_KEY) {
        const int32_t keyCode = AKeyEvent_getKeyCode(inputEvent);
        const int32_t action = AKeyEvent_getAction(inputEvent);
        if (keyCode == AKEYCODE_BACK) {
            ImGuiIO& io = ImGui::GetIO();
            if (action == AKEY_EVENT_ACTION_DOWN) {
                io.AddKeyEvent(ImGuiKey_Escape, true);
                return 1;
            }

            if (action == AKEY_EVENT_ACTION_UP) {
                io.AddKeyEvent(ImGuiKey_Escape, false);
                if (Application_ != nullptr && Application_->GetKeyboardFocus() != nullptr) {
                    Application_->ClearKeyboardFocus();
                    HideSoftKeyboard();
                    bSoftKeyboardVisible_ = false;
                } else if (bSoftKeyboardVisible_) {
                    HideSoftKeyboard();
                    bSoftKeyboardVisible_ = false;
                } else {
                    RequestClose();
                }
                return 1;
            }
        }
    }

    return ImGui_ImplAndroid_HandleInputEvent(inputEvent) ? 1 : 0;
}

bool ImAndroidGLES3Backend::SetWindowIconFromRGBA(
    const std::uint8_t* rgbaPixels,
    int width,
    int height)
{
    if (rgbaPixels == nullptr || width <= 0 || height <= 0) {
        return false;
    }

    TaskDescriptionIconPixels_.assign(
        rgbaPixels,
        rgbaPixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U));
    TaskDescriptionIconWidth_ = width;
    TaskDescriptionIconHeight_ = height;
    return UpdateTaskDescription(TaskDescriptionIconPixels_.data(), width, height);
}

void ImAndroidGLES3Backend::ClearWindowIcon()
{
    TaskDescriptionIconPixels_.clear();
    TaskDescriptionIconWidth_ = 0;
    TaskDescriptionIconHeight_ = 0;
    UpdateTaskDescription(nullptr, 0, 0);
}

void ImAndroidGLES3Backend::HandleAppCommandThunk(android_app* app, int32_t command)
{
    if (app == nullptr || app->userData == nullptr) {
        return;
    }

    static_cast<ImAndroidGLES3Backend*>(app->userData)->HandleAppCommand(command);
}

int32_t ImAndroidGLES3Backend::HandleInputEventThunk(android_app* app, AInputEvent* inputEvent)
{
    if (app == nullptr || app->userData == nullptr) {
        return 0;
    }

    return static_cast<ImAndroidGLES3Backend*>(app->userData)->HandleInputEvent(inputEvent);
}

const char* ImAndroidGLES3Backend::GetClipboardTextThunk(void* userData)
{
    if (userData == nullptr) {
        return "";
    }

    ImAndroidGLES3Backend* backend = static_cast<ImAndroidGLES3Backend*>(userData);
    backend->ClipboardTextCache_ = backend->GetClipboardText();
    return backend->ClipboardTextCache_.c_str();
}

void ImAndroidGLES3Backend::SetClipboardTextThunk(void* userData, const char* text)
{
    if (userData == nullptr) {
        return;
    }

    ImAndroidGLES3Backend* backend = static_cast<ImAndroidGLES3Backend*>(userData);
    backend->SetClipboardText(text != nullptr ? text : "");
}

void ImAndroidGLES3Backend::NativeOnTextInput(JNIEnv*, jclass, jlong backendHandle, jint codepoint)
{
    if (backendHandle == 0) {
        return;
    }

    ImAndroidGLES3Backend* backend =
        reinterpret_cast<ImAndroidGLES3Backend*>(static_cast<intptr_t>(backendHandle));
    backend->EnqueueJavaTextInput(static_cast<unsigned int>(codepoint));
}

void ImAndroidGLES3Backend::NativeOnSpecialKey(JNIEnv*, jclass, jlong backendHandle, jint keyCode, jboolean bDown)
{
    if (backendHandle == 0) {
        return;
    }

    ImAndroidGLES3Backend* backend =
        reinterpret_cast<ImAndroidGLES3Backend*>(static_cast<intptr_t>(backendHandle));
    backend->EnqueueJavaSpecialKey(static_cast<int32_t>(keyCode), bDown == JNI_TRUE);
}

} // namespace ImWidgetV4

extern "C" JNIEXPORT void JNICALL
Java_com_imwidgetv4_android_ImWidgetNativeActivity_nativeOnTextInput(
    JNIEnv* env,
    jclass clazz,
    jlong backendHandle,
    jint codepoint)
{
    ImWidgetV4::ImAndroidGLES3Backend::NativeOnTextInput(env, clazz, backendHandle, codepoint);
}

extern "C" JNIEXPORT void JNICALL
Java_com_imwidgetv4_android_ImWidgetNativeActivity_nativeOnSpecialKey(
    JNIEnv* env,
    jclass clazz,
    jlong backendHandle,
    jint keyCode,
    jboolean bDown)
{
    ImWidgetV4::ImAndroidGLES3Backend::NativeOnSpecialKey(env, clazz, backendHandle, keyCode, bDown);
}

#endif
