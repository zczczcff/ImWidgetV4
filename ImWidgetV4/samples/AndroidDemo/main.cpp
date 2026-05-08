#include <android/log.h>
#include <android_native_app_glue.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/AndroidGLES3Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

constexpr const char* GLogTag = "ImWidgetV4Sample";

void LogInfo(const char* message)
{
    __android_log_print(ANDROID_LOG_INFO, GLogTag, "%s", message);
}

} // namespace

void android_main(struct android_app* app)
{
    LogInfo("Starting ImWidgetV4 Android demo.");

    auto backend = std::make_shared<ImAndroidGLES3Backend>(app, "ImWidgetV4 Android Demo");
    auto application = std::make_shared<ImApplication>();
    backend->SetApplication(application.get());
    application->SetApplicationTitle("ImWidgetV4 Android Demo");

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(16.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("ImWidgetV4 Android Backend");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(20.0f, 20.0f, 20.0f, 0.0f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Tap the button to verify retained-mode input routing.");
    status->SetWrapText(true);
    status->SetTextColor(FColor::FromBytes(220, 227, 235));
    root->AddChild(status, FMargin(20.0f, 0.0f, 20.0f, 0.0f));

    auto button = std::make_shared<ImButton>();
    button->SetText("Tap Me");
    root->AddChild(button, FMargin(20.0f, 0.0f, 20.0f, 20.0f));

    int tapCount = 0;
    button->OnClicked.AddLambda([status, &tapCount](ImButton&) {
        ++tapCount;
        status->SetText("Button tapped " + std::to_string(tapCount) + " times.");
    });

    application->SetRootWidget(root);

    backend->Run();
    backend->Shutdown();
}
