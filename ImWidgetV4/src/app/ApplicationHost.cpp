#include <imwidgetv4/app/ApplicationHost.h>

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>

#if defined(_WIN32)
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <Windows.h>
#endif

#if defined(__ANDROID__)
#include <imwidgetv4/platform/AndroidGLES3Backend.h>
#endif

namespace ImWidgetV4 {

void IApplicationHostDelegate::ConfigureBackend(ImApplicationBackend& backend)
{
    (void)backend;
}

bool IApplicationHostDelegate::InitializeApplication(ImApplication& application, ImApplicationBackend& backend)
{
    (void)application;
    (void)backend;
    return true;
}

void IApplicationHostDelegate::Tick(ImApplication& application, const FFrameInfo& frameInfo)
{
    (void)application;
    (void)frameInfo;
}

bool IApplicationHostDelegate::OnCloseRequested(ImApplication& application)
{
    (void)application;
    return true;
}

void IApplicationHostDelegate::OnShutdown(ImApplication& application)
{
    (void)application;
}

namespace {

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) {
        return std::wstring();
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }

    return result;
}

int RunHostedDesktopApplicationInternal(IApplicationHostDelegate& delegate)
{
    const FApplicationHostConfig config = delegate.GetHostConfig();

    auto backend = std::make_shared<ImWin32DX11Backend>(
        Utf8ToWide(config.Title),
        config.InitialWidth,
        config.InitialHeight);
    backend->SetUseCustomHostChrome(config.bUseCustomHostChrome);

    if (!backend->Initialize()) {
        const std::wstring message = L"Failed to initialize application backend.";
        const std::wstring title = config.Title.empty() ? L"ImWidgetV4" : Utf8ToWide(config.Title);
        MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
        return -1;
    }

    auto application = std::make_shared<ImApplication>();
    if (!config.IniSettingsPath.empty()) {
        application->SetIniSettingsPath(config.IniSettingsPath);
    }

    backend->SetApplication(application.get());
    if (!config.Title.empty()) {
        application->SetApplicationTitle(config.Title);
    }
    if (config.Icon.IsValid()) {
        application->SetApplicationIcon(config.Icon);
    }

    delegate.ConfigureApplication(*application);
    delegate.ConfigureBackend(*backend);
    if (!delegate.InitializeApplication(*application, *backend)) {
        delegate.OnShutdown(*application);
        backend->Shutdown();
        return -1;
    }

    backend->SetPostFrameCallback([&delegate, &application](const FFrameInfo& frameInfo) {
        delegate.Tick(*application, frameInfo);
    });
    backend->SetCloseRequestedHandler([&delegate, &application]() {
        return delegate.OnCloseRequested(*application);
    });

    backend->Run();
    delegate.OnShutdown(*application);
    backend->Shutdown();
    return 0;
}
#endif

#if defined(__ANDROID__)
void RunHostedAndroidApplicationInternal(::android_app* app, IApplicationHostDelegate& delegate)
{
    const FApplicationHostConfig config = delegate.GetHostConfig();

    auto backend = std::make_shared<ImAndroidGLES3Backend>(app, config.Title);
    auto application = std::make_shared<ImApplication>();
    if (!config.IniSettingsPath.empty()) {
        application->SetIniSettingsPath(config.IniSettingsPath);
    }

    backend->SetApplication(application.get());
    if (!config.Title.empty()) {
        application->SetApplicationTitle(config.Title);
    }
    if (config.Icon.IsValid()) {
        application->SetApplicationIcon(config.Icon);
    }

    delegate.ConfigureApplication(*application);
    delegate.ConfigureBackend(*backend);
    if (!delegate.InitializeApplication(*application, *backend)) {
        delegate.OnShutdown(*application);
        backend->Shutdown();
        return;
    }
    backend->SetPostFrameCallback([&delegate, &application](const FFrameInfo& frameInfo) {
        delegate.Tick(*application, frameInfo);
    });

    backend->Run();
    delegate.OnShutdown(*application);
    backend->Shutdown();
}
#endif

} // namespace

int RunHostedDesktopApplication()
{
#if defined(_WIN32)
    std::shared_ptr<IApplicationHostDelegate> delegate = CreateApplicationHostDelegate();
    if (!delegate) {
        return -1;
    }

    return RunHostedDesktopApplicationInternal(*delegate);
#else
    return -1;
#endif
}

#if defined(__ANDROID__)
void RunHostedAndroidApplication(::android_app* app)
{
    std::shared_ptr<IApplicationHostDelegate> delegate = CreateApplicationHostDelegate();
    if (!delegate) {
        return;
    }

    RunHostedAndroidApplicationInternal(app, *delegate);
}
#endif

} // namespace ImWidgetV4
