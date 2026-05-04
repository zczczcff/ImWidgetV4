#pragma once

#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/input/Input.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/style/StyleSet.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

class ImApplication {
public:
    ImApplication();
    virtual ~ImApplication();

    void SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget);
    const std::shared_ptr<ImWidget>& GetRootWidget() const;

    void SetStyleSet(const FStyleSet& styleSet);
    const FStyleSet& GetStyleSet() const;
    FStyleSet& GetStyleSet();

    void RegisterThemePack(FThemePack&& themePack);
    bool SetActiveTheme(const std::string& name);
    const std::string& GetActiveThemeName() const;
    const std::vector<FThemePack>& GetThemePacks() const;
    void EnsureDefaultFontConfigured();

    void AdvanceFrame(const FFrameContext& frameContext);
    FSnapshotImage CaptureSnapshot(const FFrameContext& frameContext, const FSnapshotOptions& options);
    bool ExportSnapshotToPng(
        const std::filesystem::path& filePath,
        const FFrameContext& frameContext,
        const FSnapshotOptions& options);

    void EnqueueInput(const FInputEvent& inputEvent);
    const std::vector<FInputEvent>& GetLastFrameEvents() const;

    void SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget);
    void ClearKeyboardFocus();
    const std::shared_ptr<ImWidget>& GetKeyboardFocus() const;
    std::vector<std::shared_ptr<ImWidget>> GetFocusPath() const;

    void SetMouseCapture(const std::shared_ptr<ImWidget>& widget, EMouseButton button);
    void ReleaseMouseCapture();
    const std::shared_ptr<ImWidget>& GetMouseCapture() const;
    EMouseButton GetCapturedMouseButton() const;

    ImWindowManager& GetWindowManager();
    const ImWindowManager& GetWindowManager() const;

    std::uint64_t GetFrameNumber() const { return FrameNumber_; }

private:
    class FInputQueue;
    class FInteractionState;
    class FEventRouter;
    class FWidgetPathResolver;

    struct FWindowWidgetTarget;

    FStyleSet StyleSet_;
    std::vector<FThemePack> ThemePacks_;
    std::string ActiveThemeName_;
    ImWindowManager WindowManager_;

    std::unique_ptr<FInputQueue> InputQueue_;
    std::unique_ptr<FInteractionState> InteractionState_;
    std::unique_ptr<FEventRouter> EventRouter_;
    std::unique_ptr<FWidgetPathResolver> PathResolver_;

    FGeometry LastFrameGeometry_;
    bool bHasLastFrameGeometry_ = false;
    bool bDefaultFontConfigured_ = false;
    std::uint64_t FrameNumber_ = 0;

    std::vector<FInputEvent> CollectFrameInputs(const FFrameContext& frameContext);
    void RouteInputEvents();
    void UpdateHoveredWidget(
        const std::shared_ptr<ImWidget>& hoveredWidget,
        const FVector2& cursorPosition,
        double timestamp);
    void ProcessReply(const FReply& reply);
    void ResetInteractionState();
    void CleanupInteractionState();
    void ClearKeyboardFocusIfOutsideActiveWindow(const std::shared_ptr<ImWindow>& activeWindow);

    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(const std::shared_ptr<ImWidget>& widget) const;
    FReply RouteEvent(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath);
    void PerformLayoutPass();
    std::shared_ptr<ImWindow> EnsureMainWindow();
    void PaintWindows(const FFrameContext& frameContext, const FGeometry& viewportGeometry);
    FWindowWidgetTarget ResolveMouseTarget(const FVector2& position) const;
    std::shared_ptr<ImWidget> ResolveHoveredWidget(const FVector2& position) const;
};

} // namespace ImWidgetV4
