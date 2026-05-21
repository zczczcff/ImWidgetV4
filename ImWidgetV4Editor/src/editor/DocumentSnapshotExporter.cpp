#include "DocumentSnapshotExporter.h"

#include "EditorDocument.h"

#include <imwidgetv4/core/Application.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>

namespace ImWidgetV4Editor {

namespace {

class FImGuiScope {
public:
    FImGuiScope()
    {
        IMGUI_CHECKVERSION();
        PreviousContext_ = ImGui::GetCurrentContext();
        Context_ = ImGui::CreateContext();
        ImGui::SetCurrentContext(Context_);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);
        io.DeltaTime = 1.0f / 60.0f;
        if (io.Fonts != nullptr) {
            io.Fonts->Build();
        }
    }

    ~FImGuiScope()
    {
        ImGui::SetCurrentContext(PreviousContext_);
        ImGui::DestroyContext(Context_);
    }

private:
    ImGuiContext* PreviousContext_ = nullptr;
    ImGuiContext* Context_ = nullptr;
};

class FCurrentPathScope {
public:
    explicit FCurrentPathScope(const std::filesystem::path& path)
        : PreviousPath_(std::filesystem::current_path())
    {
        std::error_code error;
        std::filesystem::current_path(path, error);
        bApplied_ = !error;
    }

    ~FCurrentPathScope()
    {
        if (!bApplied_) {
            return;
        }

        std::error_code error;
        std::filesystem::current_path(PreviousPath_, error);
    }

private:
    std::filesystem::path PreviousPath_;
    bool bApplied_ = false;
};

int ResolveExportDimension(
    std::optional<int> overrideValue,
    float preferredValue,
    int fallbackValue)
{
    if (overrideValue.has_value() && *overrideValue > 0) {
        return *overrideValue;
    }

    if (std::isfinite(static_cast<double>(preferredValue)) && preferredValue > 0.0f) {
        return std::max(1, static_cast<int>(std::lround(preferredValue)));
    }

    return std::max(1, fallbackValue);
}

ImWidgetV4::FVector2 ResolveExportSize(
    const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget,
    const FDocumentSnapshotExportRequest& request)
{
    const ImWidgetV4::FVector2 preferredSize = rootWidget ? rootWidget->GetMinSize() : ImWidgetV4::FVector2();
    return ImWidgetV4::FVector2(
        static_cast<float>(ResolveExportDimension(request.Width, preferredSize.X, 1280)),
        static_cast<float>(ResolveExportDimension(request.Height, preferredSize.Y, 720)));
}

} // namespace

FDocumentSnapshotExportResult DocumentSnapshotExporter::ExportToPng(const FDocumentSnapshotExportRequest& request)
{
    FDocumentSnapshotExportResult result;
    result.OutputPath = request.OutputPath;

    if (request.InputPath.empty()) {
        result.ErrorMessage = "Input UI document path is required.";
        return result;
    }
    if (request.OutputPath.empty()) {
        result.ErrorMessage = "Output PNG path is required.";
        return result;
    }

    std::error_code pathError;
    const std::filesystem::path absoluteInputPath = std::filesystem::absolute(request.InputPath, pathError).lexically_normal();
    if (pathError) {
        result.ErrorMessage = "Failed to resolve input document path: " + pathError.message();
        return result;
    }

    const std::filesystem::path absoluteOutputPath = std::filesystem::absolute(request.OutputPath, pathError).lexically_normal();
    if (pathError) {
        result.ErrorMessage = "Failed to resolve output path: " + pathError.message();
        return result;
    }

    FCurrentPathScope currentPathScope(absoluteInputPath.parent_path());

    EditorDocument document;
    std::string loadError;
    if (!document.Load(absoluteInputPath, &loadError)) {
        result.ErrorMessage = "Failed to load UI document: " + loadError;
        return result;
    }

    const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget = document.GetRootWidget();
    if (!rootWidget) {
        result.ErrorMessage = "UI document does not contain a root widget.";
        return result;
    }

    const ImWidgetV4::FVector2 exportSize = ResolveExportSize(rootWidget, request);
    result.ExportSize = exportSize;

    const std::filesystem::path outputParentPath = absoluteOutputPath.parent_path();
    if (!outputParentPath.empty()) {
        std::filesystem::create_directories(outputParentPath, pathError);
        if (pathError) {
            result.ErrorMessage = "Failed to create output directory: " + pathError.message();
            return result;
        }
    }

    FImGuiScope imguiScope;
    ImWidgetV4::ImApplication application;
    application.SetIniSettingsPath({});
    application.SetRootWidget(rootWidget);

    ImWidgetV4::FFrameContext frameContext;
    frameContext.FrameInfo.ViewportPosition = ImWidgetV4::FVector2(0.0f, 0.0f);
    frameContext.FrameInfo.ViewportSize = exportSize;
    frameContext.FrameInfo.DeltaTime = 1.0f / 60.0f;

    const bool bExported = application.ExportSnapshotToPng(
        absoluteOutputPath,
        frameContext,
        ImWidgetV4::FSnapshotOptions {
            static_cast<int>(exportSize.X),
            static_cast<int>(exportSize.Y),
            ImWidgetV4::FColor::FromBytes(8, 10, 14, 255)});
    if (!bExported) {
        result.ErrorMessage = "Failed to export snapshot PNG.";
        return result;
    }

    result.bSuccess = true;
    result.OutputPath = absoluteOutputPath;
    return result;
}

} // namespace ImWidgetV4Editor
