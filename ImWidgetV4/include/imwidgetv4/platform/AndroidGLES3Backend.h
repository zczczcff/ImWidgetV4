#pragma once

#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/platform/ImGuiInputSource.h>
#include <cstdint>
#include <string>
#include <unordered_map>

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#endif

namespace ImWidgetV4 {

class ImApplication;

#if defined(__ANDROID__)

class ImAndroidGLES3Backend : public ImApplicationBackend {
public:
    explicit ImAndroidGLES3Backend(
        android_app* app,
        std::string windowTitle = "ImWidgetV4 Android",
        const std::string& glslVersion = "#version 300 es");
    ~ImAndroidGLES3Backend() override;

    bool Initialize() override;
    void Shutdown() override;
    void Run() override;
    bool ShouldClose() const override;
    void SetWindowTitle(const std::string& title) override;
    void SetWindowSize(int width, int height) override;
    void GetWindowSize(int& width, int& height) const override;
    void BeginFrame() override;
    void EndFrame() override;
    void SetApplication(ImApplication* app) override;
    ImApplication* GetApplication() const override;
    void RequestClose() override;
    std::string GetBackendName() const override;
    ImTextureID CreateTextureFromRGBA(
        const std::uint8_t* rgbaPixels,
        int width,
        int height) override;
    void ReleaseTexture(ImTextureID textureId) override;

private:
    bool EnsureDisplayInitialized();
    void ShutdownDisplay();
    bool EnsureImGuiBackendsInitialized();
    void ShutdownImGuiBackends();
    bool CanRender() const;
    void PumpEvents(int timeoutMillis);
    void AdvanceApplicationFrame();
    void UpdateSurfaceSize();

    void HandleAppCommand(int32_t command);
    int32_t HandleInputEvent(AInputEvent* inputEvent);

    static void HandleAppCommandThunk(android_app* app, int32_t command);
    static int32_t HandleInputEventThunk(android_app* app, AInputEvent* inputEvent);

    android_app* App_ = nullptr;
    ANativeWindow* NativeWindow_ = nullptr;
    std::string WindowTitle_;
    std::string GlslVersion_;
    int WindowWidth_ = 0;
    int WindowHeight_ = 0;
    bool bShouldClose_ = false;
    bool bInitialized_ = false;
    bool bAppResumed_ = false;
    bool bHasFocus_ = false;
    bool bImGuiContextOwned_ = false;
    bool bImGuiPlatformInitialized_ = false;
    bool bImGuiRendererInitialized_ = false;

    EGLDisplay Display_ = EGL_NO_DISPLAY;
    EGLSurface Surface_ = EGL_NO_SURFACE;
    EGLContext Context_ = EGL_NO_CONTEXT;
    EGLConfig Config_ = nullptr;

    ImApplication* Application_ = nullptr;
    FImGuiInputSource InputSource_;
    std::unordered_map<ImTextureID, GLuint> RuntimeTextures_;
};

#endif

} // namespace ImWidgetV4
