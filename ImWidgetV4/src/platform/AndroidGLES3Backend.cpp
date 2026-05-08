#include <imwidgetv4/platform/AndroidGLES3Backend.h>

#if defined(__ANDROID__)

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <android/log.h>
#include <backends/imgui_impl_android.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <utility>

namespace ImWidgetV4 {

namespace {

constexpr const char* GAndroidBackendLogTag = "ImWidgetV4";

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

} // namespace

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

    if (Application_ != nullptr) {
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

    for (const auto& entry : RuntimeTextures_) {
        const GLuint texture = entry.second;
        if (texture != 0U) {
            glDeleteTextures(1, &texture);
        }
    }
    RuntimeTextures_.clear();

    ShutdownDisplay();

    if (bImGuiContextOwned_ && ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }

    bImGuiContextOwned_ = false;
    bInitialized_ = false;
    NativeWindow_ = nullptr;

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

void ImAndroidGLES3Backend::ShutdownDisplay()
{
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
        break;

    case APP_CMD_RESUME:
        bAppResumed_ = true;
        break;

    case APP_CMD_PAUSE:
        bAppResumed_ = false;
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

    return ImGui_ImplAndroid_HandleInputEvent(inputEvent) ? 1 : 0;
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

} // namespace ImWidgetV4

#endif
