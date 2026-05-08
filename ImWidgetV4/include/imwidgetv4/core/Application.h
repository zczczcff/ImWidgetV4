#pragma once

#include <imwidgetv4/core/CoreIcon.h>
#include <imwidgetv4/core/DragDrop.h>
#include <imwidgetv4/core/FileDialog.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/input/Input.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/style/StyleSet.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ImWidgetV4 {

class ImApplicationBackend;

using FApplicationMenuItem = FPopupMenuItem;

enum class EApplicationTitleBarTabLabelKind : std::uint8_t {
    Text,
    Icon
};

struct FApplicationTitleBarTab {
    EApplicationTitleBarTabLabelKind LabelKind = EApplicationTitleBarTabLabelKind::Text;
    std::string Text;
    FImageBrush Icon;
    std::vector<FApplicationMenuItem> Items;
};

struct FApplicationTitleBarActionButton {
    FImageBrush Icon;
    std::string ToolTip;
    std::function<void()> OnInvoked;
    std::function<bool()> IsEnabled;
    std::function<bool()> IsHighlighted;
};

struct FToolTipStyle {
    FColor BackgroundColor = FColor::FromBytes(30, 34, 42, 248);
    FColor BorderColor = FColor::FromBytes(88, 101, 120, 255);
    FColor TextColor = FColor::FromBytes(244, 247, 251, 255);
    FMargin Padding = FMargin(10.0f);
    FVector2 CursorOffset {16.0f, 20.0f};
    FVector2 MinDesiredSize {36.0f, 24.0f};
    float CornerRadius = 6.0f;
    float BorderThickness = 1.0f;
    float FontSize = 14.0f;
    float MaxWidth = 360.0f;
    double ShowDelaySeconds = 0.45;
};

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
    void SetIniSettingsPath(const std::filesystem::path& path);
    const std::filesystem::path& GetIniSettingsPath() const;
    void EnsureDefaultFontConfigured();
    void SetPreferredDefaultFontData(std::vector<std::uint8_t> fontData);
    void SetPreferredDefaultFontCandidates(std::vector<std::filesystem::path> fontCandidates);
    void SetApplicationTitle(const std::string& title);
    const std::string& GetApplicationTitle() const;
    void SetApplicationIcon(const FImageBrush& brush);
    void SetApplicationIcon(ImTextureID texture, const FVector2& sourceSize = {});
    const FImageBrush& GetApplicationIcon() const;
    FPathDialogResult OpenFileDialog(const FOpenFileDialogOptions& options) const;
    FPathDialogResult OpenFolderDialog(const FOpenFolderDialogOptions& options) const;
    FPathDialogResult SaveFileDialog(const FSaveFileDialogOptions& options) const;
    void SetBackend(ImApplicationBackend* backend);
    ImApplicationBackend* GetBackend() const;
    bool ClearTitleBarTabMenus();
    bool AddTitleBarTabMenu(const std::string& text, std::vector<FApplicationMenuItem> items);
    bool AddTitleBarTabMenu(const FImageBrush& icon, std::vector<FApplicationMenuItem> items);
    const std::vector<FApplicationTitleBarTab>& GetTitleBarTabMenus() const;
    bool ClearTitleBarActionButtons();
    bool AddTitleBarActionButton(FApplicationTitleBarActionButton button);
    const std::vector<FApplicationTitleBarActionButton>& GetTitleBarActionButtons() const;
    void SetToolTipStyle(const FToolTipStyle& style);
    const FToolTipStyle& GetToolTipStyle() const;

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
    bool IsDragDropActive() const;
    std::shared_ptr<FDragDropOperation> GetCurrentDragDropOperation() const;
    std::shared_ptr<ImWidget> GetCurrentDropTarget() const;

    ImWindowManager& GetWindowManager();
    const ImWindowManager& GetWindowManager() const;

    ImTextureID CreateRuntimeTextureFromRgba(const std::vector<std::uint8_t>& pixels, int width, int height);
    void ReleaseRuntimeTexture(ImTextureID textureId);

    struct FRuntimeTextureData {
        std::vector<std::uint8_t> Pixels;
        int Width = 0;
        int Height = 0;
        int BytesPerPixel = 4;
        bool bUsesBackendTexture = false;
        ImTextureID PromotedTextureId = nullptr;
    };

    bool FindRuntimeTextureData(ImTextureID textureId, FRuntimeTextureData& outData) const;
    ImTextureID ResolveTextureForPaint(ImTextureID textureId);
    void NotifyBackendTextureResourcesLost();

    const FImageBrush& GetDefaultImagePlaceholderBrush() const;
    FImageBrush GetCoreIconBrush(ECoreIcon icon, const FColor& tint = FColor::White) const;

    std::uint64_t GetFrameNumber() const { return FrameNumber_; }

private:
    class FInputQueue;
    class FInteractionState;
    class FEventRouter;
    class FWidgetPathResolver;
    class FToolTipState;
    class FDragDropState;

    struct FWindowWidgetTarget;

    FStyleSet StyleSet_;
    std::vector<FThemePack> ThemePacks_;
    std::string ActiveThemeName_;
    ImWindowManager WindowManager_;

    std::unique_ptr<FInputQueue> InputQueue_;
    std::unique_ptr<FInteractionState> InteractionState_;
    std::unique_ptr<FEventRouter> EventRouter_;
    std::unique_ptr<FWidgetPathResolver> PathResolver_;
    std::unique_ptr<FToolTipState> ToolTipState_;
    std::unique_ptr<FDragDropState> DragDropState_;

    FGeometry LastFrameGeometry_;
    bool bHasLastFrameGeometry_ = false;
    bool bDefaultFontConfigured_ = false;
    std::uint64_t FrameNumber_ = 0;
    std::filesystem::path IniSettingsPath_;
    std::string IniSettingsPathUtf8Cache_;
    std::vector<std::uint8_t> PreferredDefaultFontData_;
    std::vector<std::filesystem::path> PreferredDefaultFontCandidates_;
    std::string ApplicationTitle_;
    FImageBrush ApplicationIcon_;
    std::vector<FApplicationTitleBarTab> TitleBarTabMenus_;
    std::vector<FApplicationTitleBarActionButton> TitleBarActionButtons_;
    FToolTipStyle ToolTipStyle_ {};
    ImApplicationBackend* Backend_ = nullptr;
    std::unordered_map<ImTextureID, FRuntimeTextureData> RuntimeTextures_;
    mutable bool bDefaultImagePlaceholderInitialized_ = false;
    mutable FImageBrush DefaultImagePlaceholderBrush_ {};
    mutable bool bCoreIconAtlasInitialized_ = false;
    mutable ImTextureID CoreIconAtlasTexture_ = nullptr;
    mutable std::vector<FImageBrush> CoreIconBrushes_;

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
    void ResetToolTipState();
    void UpdateToolTip(const FGeometry& viewportGeometry, double currentTime);
    void DismissToolTip();
    std::shared_ptr<ImWidget> BuildToolTipContentForWidget(const std::shared_ptr<ImWidget>& widget) const;
    void ResetDragDropState();
    void ClearArmedDrag();
    void BeginArmedDrag(const std::shared_ptr<ImWidget>& widget, EMouseButton button, const FInputEvent& event);
    bool TryStartDragDrop(const FInputEvent& event);
    void UpdateDragDrop(const FInputEvent& event);
    void CompleteDragDrop(const FInputEvent& event, bool bCancelled);
    void CancelDragDrop();
    FReply RouteDragEvent(
        const FDragDropEvent& event,
        const std::vector<std::shared_ptr<ImWidget>>& eventPath);
    std::vector<std::shared_ptr<ImWidget>> ResolveDragEventPath(
        const FDragDropEvent& event,
        const std::vector<std::shared_ptr<ImWidget>>& hitPath,
        std::shared_ptr<ImWidget>& outAcceptedTarget,
        FReply& outReply);
    void PaintDragDropPreview(const FFrameContext& frameContext);
    bool IsWidgetInActiveTree(const std::shared_ptr<ImWidget>& widget) const;

    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(const std::shared_ptr<ImWidget>& widget) const;
    FReply RouteEvent(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath);
    void PerformLayoutPass();
    std::shared_ptr<ImWindow> EnsureMainWindow();
    void PaintWindows(const FFrameContext& frameContext, const FGeometry& viewportGeometry);
    FWindowWidgetTarget ResolveMouseTarget(const FVector2& position) const;
    std::shared_ptr<ImWidget> ResolveHoveredWidget(const FVector2& position) const;
    void EnsureDefaultImagePlaceholderInitialized() const;
    void EnsureCoreIconAtlasInitialized() const;
    bool CanMutateTitleBarTabMenus() const;
    bool CanMutateTitleBarActions() const;
    void SyncApplicationTitle();
    void SyncApplicationIcon();
    bool TryResolveBrushPixels(
        const FImageBrush& brush,
        std::vector<std::uint8_t>& outPixels,
        int& outWidth,
        int& outHeight) const;
    void PromoteBrushToBackendTexture(FImageBrush& brush);
    static std::vector<std::uint8_t> BuildDefaultImagePlaceholderPixels(int width, int height);
};

} // namespace ImWidgetV4
