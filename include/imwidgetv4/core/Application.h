#pragma once

#include <imwidgetv4/core/Types.h>
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

    std::uint64_t GetFrameNumber() const { return FrameNumber_; }

private:
    class FInputQueue;
    class FInteractionState;
    class FEventRouter;
    class FWidgetPathResolver;

    std::shared_ptr<ImWidget> RootWidget_;
    std::shared_ptr<ImWidget> SceneRoot_;

    FStyleSet StyleSet_;
    std::vector<FThemePack> ThemePacks_;
    std::string ActiveThemeName_;

    std::unique_ptr<FInputQueue> InputQueue_;
    std::unique_ptr<FInteractionState> InteractionState_;
    std::unique_ptr<FEventRouter> EventRouter_;
    std::unique_ptr<FWidgetPathResolver> PathResolver_;

    FGeometry LastFrameGeometry_;
    bool bHasLastFrameGeometry_ = false;
    std::uint64_t FrameNumber_ = 0;

    std::vector<FInputEvent> CollectFrameInputs(const FFrameContext& frameContext);
    void RouteInputEvents();
    void UpdateHoveredWidget(const FVector2& cursorPosition, double timestamp);
    void ProcessReply(const FReply& reply);
    void ResetInteractionState();

    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(const std::shared_ptr<ImWidget>& widget) const;
    FReply RouteEvent(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath);
    void PerformLayoutPass(const FGeometry& frameGeometry);
    bool NeedsPrepassAndArrange(const FGeometry& frameGeometry) const;
};

} // namespace ImWidgetV4
