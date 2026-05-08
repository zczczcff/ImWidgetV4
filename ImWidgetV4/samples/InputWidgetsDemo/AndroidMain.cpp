#include <android/log.h>
#include <android_native_app_glue.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/AndroidGLES3Backend.h>
#include "DemoContent.h"
#include <memory>

using namespace ImWidgetV4;

namespace {

constexpr const char* GLogTag = "ImWidgetV4InputDemo";

void LogInfo(const char* message)
{
    __android_log_print(ANDROID_LOG_INFO, GLogTag, "%s", message);
}

} // namespace

void android_main(struct android_app* app)
{
    LogInfo("Starting ImWidgetV4 InputWidgetsDemo.");

    auto backend = std::make_shared<ImAndroidGLES3Backend>(app, "ImWidgetV4 Input Widgets Demo");
    auto application = std::make_shared<ImApplication>();
    backend->SetApplication(application.get());
    application->SetApplicationTitle("ImWidgetV4 Input Widgets Demo");
    application->SetRootWidget(ImWidgetV4::Samples::CreateInputWidgetsDemoRoot());

    backend->Run();
    backend->Shutdown();
}
