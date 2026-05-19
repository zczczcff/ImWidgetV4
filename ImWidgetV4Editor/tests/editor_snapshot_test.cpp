#include <gtest/gtest.h>

#include "../src/editor/EditorPaths.h"
#include "../src/editor/EditorTheme.h"
#include "../src/palette/WidgetPaletteView.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imgui.h>

#include <filesystem>
#include <memory>

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

class FImGuiScope {
public:
    FImGuiScope()
    {
        IMGUI_CHECKVERSION();
        PreviousContext_ = ImGui::GetCurrentContext();
        Context_ = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Build();
        io.DisplaySize = ImVec2(1440.0f, 900.0f);
        io.DeltaTime = 1.0f / 60.0f;
    }

    ~FImGuiScope()
    {
        ImGui::DestroyContext(Context_);
        ImGui::SetCurrentContext(PreviousContext_);
    }

private:
    ImGuiContext* PreviousContext_ = nullptr;
    ImGuiContext* Context_ = nullptr;
};

class FSnapshotTestBackend : public ImApplicationBackend {
public:
    bool Initialize() override { return true; }
    void Shutdown() override {}
    void Run() override {}
    bool ShouldClose() const override { return false; }
    void SetWindowTitle(const std::string&) override {}
    void SetWindowSize(int, int) override {}
    void GetWindowSize(int& width, int& height) const override
    {
        width = 1440;
        height = 900;
    }
    void BeginFrame() override {}
    void EndFrame() override {}
    void SetApplication(ImApplication* app) override
    {
        Application = app;
        if (Application != nullptr) {
            Application->SetBackend(this);
        }
    }
    ImApplication* GetApplication() const override { return Application; }
    void RequestClose() override {}
    std::string GetBackendName() const override { return "EditorSnapshotTestBackend"; }
    ImTextureID CreateTextureFromRGBA(const std::uint8_t*, int, int) override
    {
        return reinterpret_cast<ImTextureID>(++NextTextureIdValue);
    }
    void ReleaseTexture(ImTextureID) override {}
    std::filesystem::path GetExecutableDirectory() const override
    {
        return GetEditorExecutableDirectory();
    }

private:
    ImApplication* Application = nullptr;
    std::uintptr_t NextTextureIdValue = 1;
};

class FCurrentPathScope {
public:
    explicit FCurrentPathScope(const std::filesystem::path& path)
        : PreviousPath_(std::filesystem::current_path())
    {
        std::filesystem::current_path(path);
    }

    ~FCurrentPathScope()
    {
        std::error_code errorCode;
        std::filesystem::current_path(PreviousPath_, errorCode);
    }

private:
    std::filesystem::path PreviousPath_;
};

FFrameContext MakeFrameContext(float width, float height, double currentTime = 0.0)
{
    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportPosition = FVector2(0.0f, 0.0f);
    frameContext.FrameInfo.ViewportSize = FVector2(width, height);
    frameContext.FrameInfo.DeltaTime = 1.0f / 60.0f;
    frameContext.FrameInfo.CurrentTime = currentTime;
    return frameContext;
}

std::filesystem::path GetEditorSnapshotOutputDirectory()
{
    const std::filesystem::path outputDirectory =
        GetEditorExecutableDirectory() / "artifacts" / "snapshots" / "editor";
    std::error_code errorCode;
    std::filesystem::create_directories(outputDirectory, errorCode);
    return outputDirectory;
}

void AdvanceEditorHost(IApplicationHostDelegate& delegate, ImApplication& application, int frameCount = 2)
{
    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const FFrameInfo frameInfo {
            FVector2(0.0f, 0.0f),
            FVector2(1440.0f, 900.0f),
            1.0 / 60.0,
            static_cast<double>(frameIndex) / 60.0
        };
        application.AdvanceFrame(MakeFrameContext(1440.0f, 900.0f, frameInfo.CurrentTime));
        delegate.Tick(application, frameInfo);
    }
}

void ShutdownEditorHostForTest(IApplicationHostDelegate& delegate, ImApplication& application)
{
    delegate.OnShutdown(application);
    application.SetRootWidget(nullptr);
    application.SetBackend(nullptr);
}

void ExportEditorThemeSnapshot(
    const std::filesystem::path& outputDirectory,
    const std::string& themeName,
    const std::filesystem::path& outputFileName)
{
    FImGuiScope imguiScope;

    std::shared_ptr<IApplicationHostDelegate> delegate = CreateApplicationHostDelegate();
    ASSERT_NE(delegate, nullptr);

    FSnapshotTestBackend backend;
    ImApplication application;
    backend.SetApplication(&application);
    application.SetIniSettingsPath({});
    delegate->ConfigureApplication(application);
    ASSERT_TRUE(delegate->InitializeApplication(application, backend));

    if (!themeName.empty()) {
        ASSERT_TRUE(application.SetActiveTheme(themeName));
    }

    AdvanceEditorHost(*delegate, application, 3);

    const std::filesystem::path outputPath = outputDirectory / outputFileName;
    const bool bExported = application.ExportSnapshotToPng(
        outputPath,
        MakeFrameContext(1440.0f, 900.0f, 1.0),
        FSnapshotOptions {1440, 900, FColor::FromBytes(8, 10, 14, 255)});
    ShutdownEditorHostForTest(*delegate, application);

    ASSERT_TRUE(bExported);
    ASSERT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_GT(std::filesystem::file_size(outputPath), 0u);
}

std::shared_ptr<WidgetPaletteItemButton> FindFirstPaletteButton(const std::shared_ptr<ImWidget>& root)
{
    if (!root) {
        return nullptr;
    }

    if (auto paletteButton = std::dynamic_pointer_cast<WidgetPaletteItemButton>(root)) {
        return paletteButton;
    }

    for (const std::shared_ptr<ImWidget>& child : root->GetChildren()) {
        if (auto found = FindFirstPaletteButton(child)) {
            return found;
        }
    }

    return nullptr;
}

} // namespace

TEST(EditorSnapshotTest, ExportsRealEditorShellSnapshotsForThemes)
{
    const std::filesystem::path workspaceRoot = GetDefaultEditorWorkspaceDirectory();
    const std::filesystem::path outputDirectory = GetEditorSnapshotOutputDirectory();
    FCurrentPathScope currentPathScope(workspaceRoot);

    ExportEditorThemeSnapshot(outputDirectory, "Default", "editor_default_theme.png");
    ExportEditorThemeSnapshot(outputDirectory, "Dark", "editor_dark_theme.png");
    ExportEditorThemeSnapshot(outputDirectory, "Light", "editor_light_theme.png");
    ExportEditorThemeSnapshot(outputDirectory, "Editor Blue Green", "editor_blue_green_theme.png");
    ExportEditorThemeSnapshot(outputDirectory, "Editor Light Gray", "editor_light_gray_theme.png");
}

TEST(EditorSnapshotTest, PaletteButtonUsesEditorExplicitStyle)
{
    FImGuiScope imguiScope;
    std::shared_ptr<IApplicationHostDelegate> delegate = CreateApplicationHostDelegate();
    ASSERT_NE(delegate, nullptr);

    FSnapshotTestBackend backend;
    ImApplication application;
    backend.SetApplication(&application);
    application.SetIniSettingsPath({});
    delegate->ConfigureApplication(application);
    ASSERT_TRUE(delegate->InitializeApplication(application, backend));
    AdvanceEditorHost(*delegate, application, 3);

    std::shared_ptr<WidgetPaletteItemButton> paletteButton = FindFirstPaletteButton(application.GetRootWidget());
    ASSERT_NE(paletteButton, nullptr);

    const FButtonStyle& style = paletteButton->GetStyle();
    const FColor expectedBackground = FColor::FromBytes(34, 40, 49);
    const FColor expectedTextColor = GetEditorPanelTitleColor();
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.R, expectedBackground.R);
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.G, expectedBackground.G);
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.B, expectedBackground.B);
    EXPECT_FLOAT_EQ(style.Normal.BackgroundColor.A, expectedBackground.A);
    EXPECT_FLOAT_EQ(style.Normal.TextColor.R, expectedTextColor.R);
    EXPECT_FLOAT_EQ(style.Normal.TextColor.G, expectedTextColor.G);
    EXPECT_FLOAT_EQ(style.Normal.TextColor.B, expectedTextColor.B);
    EXPECT_FLOAT_EQ(style.Normal.TextColor.A, expectedTextColor.A);

    ShutdownEditorHostForTest(*delegate, application);
}
