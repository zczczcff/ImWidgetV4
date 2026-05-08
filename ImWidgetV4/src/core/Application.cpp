#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include "CoreIconData.h"
#include "../snapshot/TextureRegistry.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <codecvt>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <locale>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#ifdef DrawText
#undef DrawText
#endif
#ifdef CreateWindow
#undef CreateWindow
#endif
#endif

namespace ImWidgetV4 {

class ImApplication::FInputQueue {
public:
    void Enqueue(const FInputEvent& inputEvent)
    {
        PendingInput_.push_back(inputEvent);
    }

    void BeginFrame(std::vector<FInputEvent>&& frameEvents)
    {
        LastFrameEvents_.clear();
        LastFrameEvents_.reserve(PendingInput_.size() + frameEvents.size());
        LastFrameEvents_.insert(LastFrameEvents_.end(), PendingInput_.begin(), PendingInput_.end());
        LastFrameEvents_.insert(
            LastFrameEvents_.end(),
            std::make_move_iterator(frameEvents.begin()),
            std::make_move_iterator(frameEvents.end()));
        PendingInput_.clear();
    }

    const std::vector<FInputEvent>& GetLastFrameEvents() const
    {
        return LastFrameEvents_;
    }

private:
    std::vector<FInputEvent> PendingInput_;
    std::vector<FInputEvent> LastFrameEvents_;
};

class ImApplication::FInteractionState {
public:
    std::shared_ptr<ImWidget> FocusedWidget_;
    std::shared_ptr<ImWidget> CapturedMouseWidget_;
    EMouseButton CapturedMouseButton_ = EMouseButton::Left;
    std::weak_ptr<ImWidget> HoveredWidget_;
    FVector2 LastCursorPosition_ {0.0f, 0.0f};
    bool bHasCursorPosition_ = false;
    std::shared_ptr<ImWindow> DraggedWindow_;
    FVector2 WindowDragOffset_ {0.0f, 0.0f};
    std::shared_ptr<ImWindow> PressedCloseWindow_;
};

class ImApplication::FToolTipState {
public:
    std::weak_ptr<ImWidget> SourceWidget_;
    std::shared_ptr<ImWidget> ContentWidget_;
    std::shared_ptr<ImWindow> Window_;
    std::shared_ptr<ImWidget> SourceToolTipWidget_;
    std::string SourceToolTipText_;
    FVector2 AnchorCursorPosition_ {0.0f, 0.0f};
    double HoverStartTime_ = 0.0;
    bool bPendingDisplay_ = false;
    bool bSuppressForCurrentFrame_ = false;
};

class ImApplication::FDragDropState {
public:
    std::shared_ptr<ImWidget> ArmedSourceWidget_;
    EMouseButton ArmedButton_ = EMouseButton::Left;
    FVector2 PressPosition_ {0.0f, 0.0f};
    FInputModifiers PressModifiers_;
    double PressTimestamp_ = 0.0;
    bool bArmed_ = false;

    std::shared_ptr<FDragDropOperation> ActiveOperation_;
    std::shared_ptr<ImWidget> ActiveSourceWidget_;
    std::shared_ptr<ImWidget> CurrentDropTarget_;
    FVector2 CurrentPosition_ {0.0f, 0.0f};
    bool bActive_ = false;
};

class ImApplication::FWidgetPathResolver {
public:
    std::vector<std::shared_ptr<ImWidget>> BuildPathToSceneRoot(
        const std::shared_ptr<ImWidget>& sceneRoot,
        const std::shared_ptr<ImWidget>& widget) const
    {
        std::vector<std::shared_ptr<ImWidget>> path;
        if (!sceneRoot || !widget) {
            return path;
        }

        std::shared_ptr<ImWidget> current = widget;
        while (current) {
            path.push_back(current);
            if (current == sceneRoot) {
                std::reverse(path.begin(), path.end());
                return path;
            }
            current = current->GetParent();
        }

        path.clear();
        return path;
    }

    std::vector<std::shared_ptr<ImWidget>> BuildHitTestPath(
        const std::shared_ptr<ImWidget>& sceneRoot,
        const FVector2& position) const
    {
        std::vector<std::shared_ptr<ImWidget>> path;
        if (!sceneRoot) {
            return path;
        }

        sceneRoot->BuildHitTestPath(position, path);
        return path;
    }
};

class ImApplication::FEventRouter {
public:
    FReply Route(const FInputEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath) const
    {
        for (const auto& widget : eventPath) {
            FReply reply = widget->OnPreviewInputEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        for (auto it = eventPath.rbegin(); it != eventPath.rend(); ++it) {
            FReply reply = (*it)->OnInputEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        return FReply::Unhandled();
    }

    FReply RouteDrag(const FDragDropEvent& event, const std::vector<std::shared_ptr<ImWidget>>& eventPath) const
    {
        for (const auto& widget : eventPath) {
            FReply reply = widget->OnPreviewDragEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        for (auto it = eventPath.rbegin(); it != eventPath.rend(); ++it) {
            FReply reply = (*it)->OnDragEvent(event);
            if (reply.IsHandled()) {
                return reply;
            }
        }

        return FReply::Unhandled();
    }
};

struct ImApplication::FWindowWidgetTarget {
    std::shared_ptr<ImWindow> Window;
    std::vector<std::shared_ptr<ImWidget>> WidgetPath;
};

namespace {

class ImToolTipHostWidget : public ImWidget {
public:
    explicit ImToolTipHostWidget(const FToolTipStyle& style)
        : Style_(style)
    {
        SetHitTestVisible(false);
    }

    void SetContent(const std::shared_ptr<ImWidget>& content)
    {
        ClearChildren();
        Content_ = content;
        if (Content_) {
            AddChild(Content_);
        }
    }

    FVector2 GetMinSize() const override
    {
        if (!Content_) {
            return Style_.MinDesiredSize;
        }

        const float maxContentWidth = std::max(1.0f, Style_.MaxWidth - Style_.Padding.Left - Style_.Padding.Right);

        // First measure unconstrained width so wrapped text can decide whether it really
        // needs wrapping. Then remeasure using the clamped final width to get a stable height.
        Content_->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(0.0f, 0.0f)));
        const FVector2 preferredSize = Content_->GetMinSize();
        const float finalContentWidth = std::clamp(preferredSize.X, 1.0f, maxContentWidth);

        Content_->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(finalContentWidth, 0.0f)));
        const FVector2 contentSize = Content_->GetMinSize();
        return FVector2(
            std::max(Style_.MinDesiredSize.X, finalContentWidth + Style_.Padding.Left + Style_.Padding.Right),
            std::max(Style_.MinDesiredSize.Y, contentSize.Y + Style_.Padding.Top + Style_.Padding.Bottom));
    }

    void Paint(const FPaintContext& paintContext) override
    {
        if (!Content_) {
            return;
        }

        const FGeometry contentGeometry(
            FVector2(
                m_Geometry.Position.X + Style_.Padding.Left,
                m_Geometry.Position.Y + Style_.Padding.Top),
            FVector2(
                std::max(0.0f, m_Geometry.Size.X - Style_.Padding.Left - Style_.Padding.Right),
                std::max(0.0f, m_Geometry.Size.Y - Style_.Padding.Top - Style_.Padding.Bottom)));
        Content_->SetGeometry(contentGeometry);

        FPaintContext childPaintContext(
            paintContext.DrawContext_,
            contentGeometry,
            paintContext.StyleSet,
            paintContext.CursorPosition,
            paintContext.bHasCursorPosition,
            paintContext.DeltaTime);
        Content_->Paint(childPaintContext);
    }

private:
    FToolTipStyle Style_;
    std::shared_ptr<ImWidget> Content_;
};

struct FSoftwareTextureToken {
    std::uint64_t Id = 0;
};

std::uint64_t GNextSoftwareTextureTokenId = 1;

FSnapshotOptions ResolveSnapshotOptions(const FFrameContext& frameContext, const FSnapshotOptions& requestedOptions)
{
    FSnapshotOptions resolvedOptions = requestedOptions;
    if (resolvedOptions.Width <= 0) {
        resolvedOptions.Width = std::max(
            1,
            static_cast<int>(std::lround(std::max(1.0f, frameContext.FrameInfo.ViewportSize.X))));
    }

    if (resolvedOptions.Height <= 0) {
        resolvedOptions.Height = std::max(
            1,
            static_cast<int>(std::lround(std::max(1.0f, frameContext.FrameInfo.ViewportSize.Y))));
    }

    return resolvedOptions;
}

ImVec2 ResolveSnapshotDisplaySize(const FFrameContext& frameContext, const FSnapshotOptions& options)
{
    return ImVec2(
        frameContext.FrameInfo.ViewportSize.X > 0.0f ? frameContext.FrameInfo.ViewportSize.X
                                                     : static_cast<float>(options.Width),
        frameContext.FrameInfo.ViewportSize.Y > 0.0f ? frameContext.FrameInfo.ViewportSize.Y
                                                     : static_cast<float>(options.Height));
}

FVector2 MeasureText(const std::string& text, float fontSize)
{
    if (text.empty()) {
        return FVector2(0.0f, fontSize);
    }

    if (ImGui::GetCurrentContext() != nullptr) {
        const ImFont* font = ImGui::GetFont();
        if (font != nullptr) {
            const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str());
            return FVector2(size);
        }
    }

    return FVector2(fontSize * 0.55f * static_cast<float>(text.size()), fontSize);
}

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty()) {
        return {};
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(text);
}

std::string PathToUtf8(const std::filesystem::path& path)
{
#if defined(_WIN32)
    return WideToUtf8(path.wstring());
#else
    return path.string();
#endif
}

std::array<std::uint8_t, 4> SamplePlaceholderColor(
    int x,
    int y,
    int width,
    int height)
{
    static constexpr const char* GlyphRows[] = {
        "..####..",
        ".##..##.",
        ".....##.",
        "....##..",
        "...##...",
        "...##...",
        "...##...",
        "........",
        "...##...",
        "...##...",
        "........",
        "........"
    };
    static constexpr int GlyphWidth = 8;
    static constexpr int GlyphHeight = 12;

    const std::uint8_t backgroundR = 58;
    const std::uint8_t backgroundG = 66;
    const std::uint8_t backgroundB = 78;
    const std::uint8_t foregroundR = 233;
    const std::uint8_t foregroundG = 238;
    const std::uint8_t foregroundB = 244;

    const int clampedWidth = std::max(1, width);
    const int clampedHeight = std::max(1, height);
    const int sourceX = std::clamp((x * GlyphWidth) / clampedWidth, 0, GlyphWidth - 1);
    const int sourceY = std::clamp((y * GlyphHeight) / clampedHeight, 0, GlyphHeight - 1);
    const bool bForeground = GlyphRows[sourceY][sourceX] == '#';

    return bForeground
        ? std::array<std::uint8_t, 4> {foregroundR, foregroundG, foregroundB, 255}
        : std::array<std::uint8_t, 4> {backgroundR, backgroundG, backgroundB, 255};
}

std::filesystem::path GetWindowsFontDirectory()
{
#if defined(_WIN32)
    wchar_t windowsDirectory[MAX_PATH] = {};
    const UINT length = GetWindowsDirectoryW(windowsDirectory, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(windowsDirectory) / L"Fonts";
    }
#endif

    return {};
}

std::vector<std::filesystem::path> GetPreferredSystemFontCandidates()
{
    const std::filesystem::path fontDirectory = GetWindowsFontDirectory();
    if (fontDirectory.empty()) {
        return {};
    }

    return {
        fontDirectory / L"msyh.ttc",
        fontDirectory / L"msyh.ttf",
        fontDirectory / L"msjh.ttc",
        fontDirectory / L"simhei.ttf",
        fontDirectory / L"simsun.ttc",
        fontDirectory / L"segoeui.ttf",
        fontDirectory / L"arial.ttf"
    };
}

void DrawWindowChrome(
    DrawContext& drawContext,
    const ImWindow& window,
    const FGeometry& viewportGeometry)
{
    const FWindowStyle& style = window.GetStyle();
    const FGeometry windowGeometry = window.GetWindowGeometry();
    const FVector2 windowMin = windowGeometry.GetMin();
    const FVector2 windowMax = windowGeometry.GetMax();
    const bool bActive = window.IsActive();

    if (window.GetKind() == EWindowKind::Modal) {
        drawContext.DrawRectFilled(
            viewportGeometry.GetMin(),
            viewportGeometry.GetMax(),
            style.ModalOverlayColor);
    }

    if (window.HasBackground()) {
        if (style.bDrawShadow) {
            drawContext.DrawRectFilled(
                windowMin + style.ShadowOffset,
                windowMax + style.ShadowOffset,
                style.ShadowColor,
                style.CornerRadius);
        }

        drawContext.DrawRectFilled(
            windowMin,
            windowMax,
            bActive ? style.BackgroundColor : style.InactiveBackgroundColor,
            style.CornerRadius);
    }

    if (window.HasTitleBar()) {
        const FGeometry titleBarGeometry = window.GetTitleBarGeometry();
        drawContext.DrawRectFilled(
            titleBarGeometry.GetMin(),
            titleBarGeometry.GetMax(),
            bActive ? style.ActiveTitleBarColor : style.TitleBarColor,
            style.CornerRadius);

        const FVector2 titleTextSize = MeasureText(window.GetTitle(), style.TitleFontSize);
        const float titleX = titleBarGeometry.Position.X + 12.0f;
        const float titleY = titleBarGeometry.Position.Y +
            std::max(0.0f, (titleBarGeometry.Size.Y - titleTextSize.Y) * 0.5f);
        drawContext.DrawText(
            FVector2(titleX, titleY),
            bActive ? style.TitleTextColor : style.InactiveTitleTextColor,
            window.GetTitle(),
            style.TitleFontSize);

        if (window.IsClosable()) {
            const FGeometry closeGeometry = window.GetCloseButtonGeometry();
            FColor closeColor = style.CloseButtonColor;
            if (window.IsCloseButtonPressed()) {
                closeColor = style.CloseButtonPressedColor;
            } else if (window.IsCloseButtonHovered()) {
                closeColor = style.CloseButtonHoveredColor;
            }

            drawContext.DrawRectFilled(
                closeGeometry.GetMin(),
                closeGeometry.GetMax(),
                closeColor,
                4.0f);

            const FVector2 glyphInset(3.5f, 3.5f);
            drawContext.DrawLine(
                closeGeometry.GetMin() + glyphInset,
                closeGeometry.GetMax() - glyphInset,
                style.CloseGlyphColor,
                1.5f);
            drawContext.DrawLine(
                FVector2(closeGeometry.GetMin().X + glyphInset.X, closeGeometry.GetMax().Y - glyphInset.Y),
                FVector2(closeGeometry.GetMax().X - glyphInset.X, closeGeometry.GetMin().Y + glyphInset.Y),
                style.CloseGlyphColor,
                1.5f);
        }
    }

    drawContext.DrawRect(
        windowMin,
        windowMax,
        bActive ? style.ActiveBorderColor : style.BorderColor,
        style.CornerRadius,
        style.BorderThickness);
}

class FScopedSnapshotImGuiFrame {
public:
    FScopedSnapshotImGuiFrame(const FFrameContext& frameContext, const FSnapshotOptions& options)
    {
        Context_ = ImGui::GetCurrentContext();
        if (Context_ == nullptr) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts != nullptr && io.Fonts->TexID == nullptr) {
            SavedFontTextureId_ = io.Fonts->TexID;
            io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(io.Fonts));
            bRestoreFontTextureId_ = true;
        }

        if (Context_->WithinFrameScope) {
            return;
        }

        SavedDisplaySize_ = io.DisplaySize;
        SavedDeltaTime_ = io.DeltaTime;
        io.DisplaySize = ResolveSnapshotDisplaySize(frameContext, options);
        io.DeltaTime = frameContext.FrameInfo.DeltaTime > 0.0f ? frameContext.FrameInfo.DeltaTime : 1.0f / 60.0f;
        ImGui::NewFrame();
        bOwnsFrame_ = true;
    }

    ~FScopedSnapshotImGuiFrame()
    {
        if (Context_ == nullptr) {
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        if (bOwnsFrame_) {
            ImGui::EndFrame();
            io.DisplaySize = SavedDisplaySize_;
            io.DeltaTime = SavedDeltaTime_;
        }

        if (bRestoreFontTextureId_ && io.Fonts != nullptr) {
            io.Fonts->SetTexID(SavedFontTextureId_);
        }
    }

private:
    ImGuiContext* Context_ = nullptr;
    bool bOwnsFrame_ = false;
    bool bRestoreFontTextureId_ = false;
    ImTextureID SavedFontTextureId_ = nullptr;
    ImVec2 SavedDisplaySize_ = ImVec2(0.0f, 0.0f);
    float SavedDeltaTime_ = 0.0f;
};

} // namespace

ImApplication::ImApplication()
    : InputQueue_(std::make_unique<FInputQueue>())
    , InteractionState_(std::make_unique<FInteractionState>())
    , EventRouter_(std::make_unique<FEventRouter>())
    , PathResolver_(std::make_unique<FWidgetPathResolver>())
    , ToolTipState_(std::make_unique<FToolTipState>())
    , DragDropState_(std::make_unique<FDragDropState>())
{
    EnsureDefaultFontConfigured();

    WindowManager_.SetOwnerApplication(this);

    auto defaultStyleSet = FStyleSetFactory::CreateDefault();
    if (defaultStyleSet) {
        StyleSet_ = std::move(*defaultStyleSet);
    }

    {
        FThemePack defaultTheme("Default");
        auto styleSet = FStyleSetFactory::CreateDefault();
        if (styleSet) {
            defaultTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(defaultTheme));
    }

    {
        FThemePack darkTheme("Dark");
        auto styleSet = FStyleSetFactory::CreateDarkTheme();
        if (styleSet) {
            darkTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(darkTheme));
    }

    {
        FThemePack lightTheme("Light");
        auto styleSet = FStyleSetFactory::CreateLightTheme();
        if (styleSet) {
            lightTheme.StyleSet = std::move(*styleSet);
        }
        RegisterThemePack(std::move(lightTheme));
    }

    SetActiveTheme("Default");
}

ImApplication::~ImApplication()
{
    DismissToolTip();

    std::vector<ImTextureID> textureIds;
    textureIds.reserve(RuntimeTextures_.size());
    for (const auto& entry : RuntimeTextures_) {
        textureIds.push_back(entry.first);
    }

    for (ImTextureID textureId : textureIds) {
        ReleaseRuntimeTexture(textureId);
    }
}

std::shared_ptr<ImWindow> ImApplication::EnsureMainWindow()
{
    if (std::shared_ptr<ImWindow> mainWindow = WindowManager_.GetMainWindow()) {
        return mainWindow;
    }

    FWindowOptions options;
    options.Title = "Main";
    options.Position = FVector2(0.0f, 0.0f);
    options.Size = FVector2(1.0f, 1.0f);
    options.bMovable = false;
    options.bClosable = false;
    options.bHasTitleBar = false;
    options.bHasBackground = false;
    options.bFillViewport = true;
    std::shared_ptr<ImWindow> mainWindow = WindowManager_.CreateWindow(options);
    WindowManager_.SetMainWindowInternal(mainWindow);
    return mainWindow;
}

void ImApplication::SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget)
{
    ResetInteractionState();
    DismissToolTip();
    EnsureMainWindow()->SetRootWidget(rootWidget);
}

const std::shared_ptr<ImWidget>& ImApplication::GetRootWidget() const
{
    static const std::shared_ptr<ImWidget> NullRootWidget;
    const std::shared_ptr<ImWindow> mainWindow = WindowManager_.GetMainWindow();
    return mainWindow ? mainWindow->GetRootWidget() : NullRootWidget;
}

void ImApplication::SetStyleSet(const FStyleSet& styleSet)
{
    StyleSet_.Clear();
    StyleSet_.Merge(styleSet);
}

const FStyleSet& ImApplication::GetStyleSet() const
{
    return StyleSet_;
}

FStyleSet& ImApplication::GetStyleSet()
{
    return StyleSet_;
}

void ImApplication::RegisterThemePack(FThemePack&& themePack)
{
    ThemePacks_.push_back(std::move(themePack));
}

bool ImApplication::SetActiveTheme(const std::string& name)
{
    for (const auto& pack : ThemePacks_) {
        if (pack.Name == name) {
            ActiveThemeName_ = name;
            StyleSet_.Clear();
            StyleSet_.Merge(pack.StyleSet);
            return true;
        }
    }

    return false;
}

const std::string& ImApplication::GetActiveThemeName() const
{
    return ActiveThemeName_;
}

const std::vector<FThemePack>& ImApplication::GetThemePacks() const
{
    return ThemePacks_;
}

void ImApplication::SetApplicationTitle(const std::string& title)
{
    if (ApplicationTitle_ == title) {
        return;
    }

    ApplicationTitle_ = title;
    SyncApplicationTitle();
}

const std::string& ImApplication::GetApplicationTitle() const
{
    return ApplicationTitle_;
}

void ImApplication::SetApplicationIcon(const FImageBrush& brush)
{
    ApplicationIcon_ = brush;
    PromoteBrushToBackendTexture(ApplicationIcon_);
    SyncApplicationIcon();
}

void ImApplication::SetApplicationIcon(ImTextureID texture, const FVector2& sourceSize)
{
    FImageBrush brush;
    brush.TextureId = texture;
    brush.SourceSize = sourceSize;
    SetApplicationIcon(brush);
}

const FImageBrush& ImApplication::GetApplicationIcon() const
{
    return ApplicationIcon_;
}

FPathDialogResult ImApplication::OpenFileDialog(const FOpenFileDialogOptions& options) const
{
    if (Backend_ == nullptr) {
        return FPathDialogResult();
    }

    return Backend_->OpenFileDialog(options);
}

FPathDialogResult ImApplication::OpenFolderDialog(const FOpenFolderDialogOptions& options) const
{
    if (Backend_ == nullptr) {
        return FPathDialogResult();
    }

    return Backend_->OpenFolderDialog(options);
}

FPathDialogResult ImApplication::SaveFileDialog(const FSaveFileDialogOptions& options) const
{
    if (Backend_ == nullptr) {
        return FPathDialogResult();
    }

    return Backend_->SaveFileDialog(options);
}

void ImApplication::SetBackend(ImApplicationBackend* backend)
{
    Backend_ = backend;
    if (Backend_ != nullptr) {
        PromoteBrushToBackendTexture(ApplicationIcon_);
        for (FApplicationTitleBarTab& tab : TitleBarTabMenus_) {
            if (tab.LabelKind == EApplicationTitleBarTabLabelKind::Icon) {
                PromoteBrushToBackendTexture(tab.Icon);
            }

            for (FApplicationMenuItem& item : tab.Items) {
                PromoteBrushToBackendTexture(item.Icon);
            }
        }
    }

    if (Backend_ != nullptr && DefaultImagePlaceholderBrush_.IsValid()) {
        FRuntimeTextureData placeholderData;
        if (FindRuntimeTextureData(DefaultImagePlaceholderBrush_.TextureId, placeholderData) &&
            !placeholderData.bUsesBackendTexture) {
            ReleaseRuntimeTexture(DefaultImagePlaceholderBrush_.TextureId);
        }
    }

    if (Backend_ != nullptr && CoreIconAtlasTexture_ != nullptr) {
        FRuntimeTextureData iconAtlasData;
        if (FindRuntimeTextureData(CoreIconAtlasTexture_, iconAtlasData) &&
            !iconAtlasData.bUsesBackendTexture) {
            ReleaseRuntimeTexture(CoreIconAtlasTexture_);
        }
    }

    SyncApplicationTitle();
    SyncApplicationIcon();
}

ImApplicationBackend* ImApplication::GetBackend() const
{
    return Backend_;
}

bool ImApplication::ClearTitleBarTabMenus()
{
    if (!CanMutateTitleBarTabMenus()) {
        return false;
    }

    TitleBarTabMenus_.clear();
    return true;
}

bool ImApplication::AddTitleBarTabMenu(const std::string& text, std::vector<FApplicationMenuItem> items)
{
    if (!CanMutateTitleBarTabMenus()) {
        return false;
    }

    FApplicationTitleBarTab tab;
    tab.LabelKind = EApplicationTitleBarTabLabelKind::Text;
    tab.Text = text;
    tab.Items = std::move(items);
    for (FApplicationMenuItem& item : tab.Items) {
        PromoteBrushToBackendTexture(item.Icon);
    }
    TitleBarTabMenus_.push_back(std::move(tab));
    return true;
}

bool ImApplication::AddTitleBarTabMenu(const FImageBrush& icon, std::vector<FApplicationMenuItem> items)
{
    if (!CanMutateTitleBarTabMenus()) {
        return false;
    }

    FApplicationTitleBarTab tab;
    tab.LabelKind = EApplicationTitleBarTabLabelKind::Icon;
    tab.Icon = icon;
    PromoteBrushToBackendTexture(tab.Icon);
    tab.Items = std::move(items);
    for (FApplicationMenuItem& item : tab.Items) {
        PromoteBrushToBackendTexture(item.Icon);
    }
    TitleBarTabMenus_.push_back(std::move(tab));
    return true;
}

const std::vector<FApplicationTitleBarTab>& ImApplication::GetTitleBarTabMenus() const
{
    return TitleBarTabMenus_;
}

bool ImApplication::ClearTitleBarActionButtons()
{
    if (!CanMutateTitleBarActions()) {
        return false;
    }

    TitleBarActionButtons_.clear();
    return true;
}

bool ImApplication::AddTitleBarActionButton(FApplicationTitleBarActionButton button)
{
    if (!CanMutateTitleBarActions()) {
        return false;
    }

    if (button.Icon.IsValid()) {
        PromoteBrushToBackendTexture(button.Icon);
    }

    TitleBarActionButtons_.push_back(std::move(button));
    return true;
}

const std::vector<FApplicationTitleBarActionButton>& ImApplication::GetTitleBarActionButtons() const
{
    return TitleBarActionButtons_;
}

void ImApplication::SetToolTipStyle(const FToolTipStyle& style)
{
    ToolTipStyle_ = style;
    if (ToolTipState_->Window_ && ToolTipState_->Window_->IsOpen()) {
        DismissToolTip();
    }
}

const FToolTipStyle& ImApplication::GetToolTipStyle() const
{
    return ToolTipStyle_;
}

void ImApplication::SetIniSettingsPath(const std::filesystem::path& path)
{
    IniSettingsPath_ = path;
    IniSettingsPathUtf8Cache_ = IniSettingsPath_.empty() ? std::string() : PathToUtf8(IniSettingsPath_);

    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = IniSettingsPathUtf8Cache_.empty() ? nullptr : IniSettingsPathUtf8Cache_.c_str();
}

const std::filesystem::path& ImApplication::GetIniSettingsPath() const
{
    return IniSettingsPath_;
}

void ImApplication::EnsureDefaultFontConfigured()
{
    if (bDefaultFontConfigured_) {
        return;
    }

    ImGuiContext* context = ImGui::GetCurrentContext();
    if (context == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.Fonts == nullptr) {
        return;
    }

    if (context->WithinFrameScope || io.Fonts->Locked) {
        return;
    }

    if (io.Fonts->Fonts.Size > 0) {
        if (io.FontDefault == nullptr && !io.Fonts->Fonts.empty()) {
            io.FontDefault = io.Fonts->Fonts[0];
        }
        bDefaultFontConfigured_ = true;
        return;
    }

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;
    fontConfig.PixelSnapH = false;

    ImFont* loadedFont = nullptr;
    for (const std::filesystem::path& candidate : GetPreferredSystemFontCandidates()) {
        std::error_code errorCode;
        if (!std::filesystem::exists(candidate, errorCode) || errorCode) {
            continue;
        }

        const std::string utf8Path = WideToUtf8(candidate.wstring());
        loadedFont = io.Fonts->AddFontFromFileTTF(
            utf8Path.c_str(),
            18.0f,
            &fontConfig,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (loadedFont != nullptr) {
            break;
        }
    }

    if (loadedFont == nullptr) {
        loadedFont = io.Fonts->AddFontDefault();
    }

    if (loadedFont != nullptr) {
        io.FontDefault = loadedFont;
        io.Fonts->Build();
        bDefaultFontConfigured_ = true;
    }
}

ImTextureID ImApplication::CreateRuntimeTextureFromRgba(
    const std::vector<std::uint8_t>& pixels,
    int width,
    int height)
{
    if (width <= 0 || height <= 0 ||
        pixels.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U) {
        return nullptr;
    }

    ImTextureID textureId = nullptr;
    bool bUsesBackendTexture = false;
    if (Backend_ != nullptr) {
        textureId = Backend_->CreateTextureFromRGBA(pixels.data(), width, height);
        bUsesBackendTexture = textureId != nullptr;
    }

    if (textureId == nullptr) {
        textureId = new FSoftwareTextureToken {GNextSoftwareTextureTokenId++};
    }

    FRuntimeTextureData textureData;
    textureData.Pixels = pixels;
    textureData.Width = width;
    textureData.Height = height;
    textureData.BytesPerPixel = 4;
    textureData.bUsesBackendTexture = bUsesBackendTexture;

    RuntimeTextures_[textureId] = textureData;

    SnapshotInternal::FRegisteredTextureData snapshotData;
    snapshotData.Pixels = textureData.Pixels;
    snapshotData.Width = textureData.Width;
    snapshotData.Height = textureData.Height;
    snapshotData.BytesPerPixel = textureData.BytesPerPixel;
    SnapshotInternal::RegisterTexture(textureId, snapshotData);
    return textureId;
}

void ImApplication::ReleaseRuntimeTexture(ImTextureID textureId)
{
    const auto it = RuntimeTextures_.find(textureId);
    if (it == RuntimeTextures_.end()) {
        return;
    }

    if (it->second.bUsesBackendTexture) {
        if (Backend_ != nullptr) {
            Backend_->ReleaseTexture(textureId);
        }
    } else {
        if (it->second.PromotedTextureId != nullptr && Backend_ != nullptr) {
            Backend_->ReleaseTexture(it->second.PromotedTextureId);
        }
        delete static_cast<FSoftwareTextureToken*>(textureId);
    }

    SnapshotInternal::UnregisterTexture(textureId);
    RuntimeTextures_.erase(it);

    if (ApplicationIcon_.TextureId == textureId) {
        ApplicationIcon_ = FImageBrush();
        SyncApplicationIcon();
    }

    for (FApplicationTitleBarTab& tab : TitleBarTabMenus_) {
        if (tab.Icon.TextureId == textureId) {
            tab.Icon = FImageBrush();
        }

        for (FApplicationMenuItem& item : tab.Items) {
            if (item.Icon.TextureId == textureId) {
                item.Icon = FImageBrush();
            }
        }
    }

    if (DefaultImagePlaceholderBrush_.TextureId == textureId) {
        DefaultImagePlaceholderBrush_ = FImageBrush();
        bDefaultImagePlaceholderInitialized_ = false;
    }

    if (CoreIconAtlasTexture_ == textureId) {
        CoreIconAtlasTexture_ = nullptr;
        bCoreIconAtlasInitialized_ = false;
        CoreIconBrushes_.clear();
    }
}

bool ImApplication::FindRuntimeTextureData(ImTextureID textureId, FRuntimeTextureData& outData) const
{
    const auto it = RuntimeTextures_.find(textureId);
    if (it == RuntimeTextures_.end()) {
        return false;
    }

    outData = it->second;
    return true;
}

ImTextureID ImApplication::ResolveTextureForPaint(ImTextureID textureId)
{
    if (textureId == nullptr) {
        return nullptr;
    }

    const auto it = RuntimeTextures_.find(textureId);
    if (it == RuntimeTextures_.end()) {
        return textureId;
    }

    FRuntimeTextureData& textureData = it->second;
    if (textureData.bUsesBackendTexture) {
        return textureId;
    }

    if (textureData.PromotedTextureId != nullptr) {
        return textureData.PromotedTextureId;
    }

    if (Backend_ == nullptr) {
        return textureId;
    }

    textureData.PromotedTextureId = Backend_->CreateTextureFromRGBA(
        textureData.Pixels.data(),
        textureData.Width,
        textureData.Height);
    if (textureData.PromotedTextureId == nullptr) {
        return textureId;
    }

    return textureData.PromotedTextureId;
}

const FImageBrush& ImApplication::GetDefaultImagePlaceholderBrush() const
{
    EnsureDefaultImagePlaceholderInitialized();
    return DefaultImagePlaceholderBrush_;
}

FImageBrush ImApplication::GetCoreIconBrush(ECoreIcon icon, const FColor& tint) const
{
    EnsureCoreIconAtlasInitialized();

    const int iconIndex = static_cast<int>(icon);
    if (!bCoreIconAtlasInitialized_ ||
        iconIndex < 0 ||
        iconIndex >= static_cast<int>(CoreIconBrushes_.size())) {
        return FImageBrush();
    }

    FImageBrush brush = CoreIconBrushes_[static_cast<std::size_t>(iconIndex)];
    brush.TintColor = tint;
    return brush;
}

void ImApplication::EnqueueInput(const FInputEvent& inputEvent)
{
    InputQueue_->Enqueue(inputEvent);
}

const std::vector<FInputEvent>& ImApplication::GetLastFrameEvents() const
{
    return InputQueue_->GetLastFrameEvents();
}

void ImApplication::SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        ClearKeyboardFocus();
        return;
    }

    if (!widget->SupportsKeyboardFocus()) {
        return;
    }

    if (BuildPathToSceneRoot(widget).empty()) {
        return;
    }

    const std::shared_ptr<ImWindow> ownerWindow = WindowManager_.FindOwningWindow(widget);
    if (!ownerWindow || WindowManager_.IsBlockedByModal(ownerWindow)) {
        return;
    }

    if (InteractionState_->FocusedWidget_ == widget) {
        return;
    }

    if (InteractionState_->FocusedWidget_) {
        InteractionState_->FocusedWidget_->NotifyFocusChanged(false);
    }

    InteractionState_->FocusedWidget_ = widget;
    InteractionState_->FocusedWidget_->NotifyFocusChanged(true);
}

void ImApplication::ClearKeyboardFocus()
{
    if (InteractionState_->FocusedWidget_) {
        InteractionState_->FocusedWidget_->NotifyFocusChanged(false);
        InteractionState_->FocusedWidget_.reset();
    }
}

const std::shared_ptr<ImWidget>& ImApplication::GetKeyboardFocus() const
{
    return InteractionState_->FocusedWidget_;
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::GetFocusPath() const
{
    return BuildPathToSceneRoot(InteractionState_->FocusedWidget_);
}

void ImApplication::SetMouseCapture(const std::shared_ptr<ImWidget>& widget, EMouseButton button)
{
    if (!widget) {
        ReleaseMouseCapture();
        return;
    }

    if (BuildPathToSceneRoot(widget).empty()) {
        return;
    }

    const std::shared_ptr<ImWindow> ownerWindow = WindowManager_.FindOwningWindow(widget);
    if (!ownerWindow || WindowManager_.IsBlockedByModal(ownerWindow)) {
        return;
    }

    InteractionState_->CapturedMouseWidget_ = widget;
    InteractionState_->CapturedMouseButton_ = button;
}

void ImApplication::ReleaseMouseCapture()
{
    InteractionState_->CapturedMouseWidget_.reset();
    if (!DragDropState_->bActive_) {
        ClearArmedDrag();
    }
}

const std::shared_ptr<ImWidget>& ImApplication::GetMouseCapture() const
{
    return InteractionState_->CapturedMouseWidget_;
}

EMouseButton ImApplication::GetCapturedMouseButton() const
{
    return InteractionState_->CapturedMouseButton_;
}

bool ImApplication::IsDragDropActive() const
{
    return DragDropState_->bActive_;
}

std::shared_ptr<FDragDropOperation> ImApplication::GetCurrentDragDropOperation() const
{
    return DragDropState_->ActiveOperation_;
}

std::shared_ptr<ImWidget> ImApplication::GetCurrentDropTarget() const
{
    return DragDropState_->CurrentDropTarget_;
}

ImWindowManager& ImApplication::GetWindowManager()
{
    return WindowManager_;
}

const ImWindowManager& ImApplication::GetWindowManager() const
{
    return WindowManager_;
}

void ImApplication::AdvanceFrame(const FFrameContext& frameContext)
{
    EnsureDefaultFontConfigured();
    ++FrameNumber_;
    ToolTipState_->bSuppressForCurrentFrame_ = false;

    InputQueue_->BeginFrame(CollectFrameInputs(frameContext));

    const FGeometry viewportGeometry(
        frameContext.FrameInfo.ViewportPosition,
        frameContext.FrameInfo.ViewportSize);

    WindowManager_.SyncViewportFilledWindows(viewportGeometry);
    PerformLayoutPass();
    RouteInputEvents();
    CleanupInteractionState();

    const auto& lastFrameEvents = InputQueue_->GetLastFrameEvents();
    const bool bHadMouseEvent = std::any_of(
        lastFrameEvents.begin(),
        lastFrameEvents.end(),
        [](const FInputEvent& event) {
            return event.IsMouseEvent() &&
                   event.Type != EInputEventType::MouseEnter &&
                   event.Type != EInputEventType::MouseLeave;
        });

    if (InteractionState_->bHasCursorPosition_ && !bHadMouseEvent) {
        UpdateHoveredWidget(
            ResolveHoveredWidget(InteractionState_->LastCursorPosition_),
            InteractionState_->LastCursorPosition_,
            frameContext.FrameInfo.CurrentTime);
    }

    UpdateToolTip(viewportGeometry, frameContext.FrameInfo.CurrentTime);
    PerformLayoutPass();

    if (frameContext.DrawContext_ != nullptr) {
        PaintWindows(frameContext, viewportGeometry);
    }

    LastFrameGeometry_ = viewportGeometry;
    bHasLastFrameGeometry_ = true;
}

FSnapshotImage ImApplication::CaptureSnapshot(
    const FFrameContext& frameContext,
    const FSnapshotOptions& options)
{
    EnsureDefaultFontConfigured();
    const FSnapshotOptions resolvedOptions = ResolveSnapshotOptions(frameContext, options);
    const ImVec2 displaySize = ResolveSnapshotDisplaySize(frameContext, resolvedOptions);

    ImDrawData emptyDrawData;
    emptyDrawData.Valid = true;
    emptyDrawData.DisplayPos = frameContext.FrameInfo.ViewportPosition.ToImVec2();
    emptyDrawData.DisplaySize = displaySize;
    emptyDrawData.FramebufferScale = ImVec2(1.0f, 1.0f);

    if (ImGui::GetCurrentContext() == nullptr) {
        return FSnapshotRenderer::Rasterize(emptyDrawData, resolvedOptions);
    }

    FScopedSnapshotImGuiFrame scopedSnapshotFrame(frameContext, resolvedOptions);
    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();

    if (ImGui::GetIO().Fonts != nullptr) {
        drawList.PushTextureID(ImGui::GetIO().Fonts->TexID);
    }

    drawList.PushClipRect(
        frameContext.FrameInfo.ViewportPosition.ToImVec2(),
        ImVec2(
            frameContext.FrameInfo.ViewportPosition.X + displaySize.x,
            frameContext.FrameInfo.ViewportPosition.Y + displaySize.y),
        false);

    DrawContext drawContext(&drawList);
    FFrameContext captureContext = frameContext;
    captureContext.DrawContext_ = &drawContext;
    AdvanceFrame(captureContext);

    drawList.PopClipRect();
    if (ImGui::GetIO().Fonts != nullptr) {
        drawList.PopTextureID();
    }

    ImDrawData drawData;
    drawData.Valid = true;
    drawData.CmdLists.push_back(&drawList);
    drawData.CmdListsCount = 1;
    drawData.TotalIdxCount = drawList.IdxBuffer.Size;
    drawData.TotalVtxCount = drawList.VtxBuffer.Size;
    drawData.DisplayPos = frameContext.FrameInfo.ViewportPosition.ToImVec2();
    drawData.DisplaySize = displaySize;
    drawData.FramebufferScale = ImVec2(1.0f, 1.0f);

    return FSnapshotRenderer::Rasterize(drawData, resolvedOptions);
}

bool ImApplication::ExportSnapshotToPng(
    const std::filesystem::path& filePath,
    const FFrameContext& frameContext,
    const FSnapshotOptions& options)
{
    return FSnapshotRenderer::SavePng(filePath, CaptureSnapshot(frameContext, options));
}

std::vector<FInputEvent> ImApplication::CollectFrameInputs(const FFrameContext& frameContext)
{
    std::vector<FInputEvent> frameInputs;

    if (frameContext.InputSource != nullptr) {
        std::vector<FInputEvent> polledEvents = frameContext.InputSource->Poll(frameContext.FrameInfo);
        frameInputs.insert(
            frameInputs.end(),
            std::make_move_iterator(polledEvents.begin()),
            std::make_move_iterator(polledEvents.end()));
    }

    if (frameContext.InputEvents != nullptr) {
        frameInputs.insert(
            frameInputs.end(),
            frameContext.InputEvents->begin(),
            frameContext.InputEvents->end());
    }

    return frameInputs;
}

void ImApplication::RouteInputEvents()
{
    const auto openWindows = WindowManager_.GetOpenWindows();
    if (openWindows.empty()) {
        return;
    }

    for (const std::shared_ptr<ImWindow>& window : openWindows) {
        window->bCloseButtonHovered_ = false;
    }

    for (const FInputEvent& inputEvent : InputQueue_->GetLastFrameEvents()) {
        if (inputEvent.IsMouseEvent() &&
            inputEvent.Type != EInputEventType::MouseEnter &&
            inputEvent.Type != EInputEventType::MouseLeave) {
            InteractionState_->LastCursorPosition_ = inputEvent.MousePosition;
            InteractionState_->bHasCursorPosition_ = true;
            if (DragDropState_->bActive_) {
                DragDropState_->CurrentPosition_ = inputEvent.MousePosition;
            }
        }

        if (inputEvent.Type == EInputEventType::MouseButtonDown ||
            inputEvent.Type == EInputEventType::MouseWheel ||
            inputEvent.Type == EInputEventType::KeyDown) {
            ToolTipState_->bSuppressForCurrentFrame_ = true;
            DismissToolTip();
        }

        if (DragDropState_->bActive_ &&
            inputEvent.Type == EInputEventType::KeyDown &&
            inputEvent.Key == EKey::Escape) {
            CompleteDragDrop(inputEvent, true);
            CleanupInteractionState();
            continue;
        }

        if (inputEvent.IsMouseEvent()) {
            std::shared_ptr<ImWindow> hoveredWindow = WindowManager_.HitTestWindow(inputEvent.MousePosition);
            for (const std::shared_ptr<ImWindow>& window : openWindows) {
                window->bCloseButtonHovered_ =
                    hoveredWindow == window && window->IsPointInCloseButton(inputEvent.MousePosition);
            }
        }

        if (DragDropState_->bActive_) {
            if (inputEvent.Type == EInputEventType::MouseMove) {
                UpdateDragDrop(inputEvent);
            } else if (inputEvent.Type == EInputEventType::MouseButtonUp &&
                       inputEvent.MouseButton == DragDropState_->ActiveOperation_->TriggerButton) {
                UpdateDragDrop(inputEvent);
                CompleteDragDrop(inputEvent, false);
            }

            CleanupInteractionState();
            continue;
        }

        if (inputEvent.Type == EInputEventType::MouseMove && InteractionState_->DraggedWindow_) {
            InteractionState_->DraggedWindow_->SetPosition(
                inputEvent.MousePosition - InteractionState_->WindowDragOffset_);
            UpdateHoveredWidget(nullptr, inputEvent.MousePosition, inputEvent.Timestamp);
            continue;
        }

        if (inputEvent.Type == EInputEventType::MouseButtonUp &&
            inputEvent.MouseButton == EMouseButton::Left &&
            InteractionState_->DraggedWindow_) {
            InteractionState_->DraggedWindow_->SetPosition(
                inputEvent.MousePosition - InteractionState_->WindowDragOffset_);
            InteractionState_->DraggedWindow_.reset();
            UpdateHoveredWidget(
                ResolveHoveredWidget(inputEvent.MousePosition),
                inputEvent.MousePosition,
                inputEvent.Timestamp);
            continue;
        }

        if (inputEvent.Type == EInputEventType::MouseButtonDown &&
            inputEvent.MouseButton == EMouseButton::Left) {
            if (WindowManager_.GetTopmostModalWindow() &&
                !WindowManager_.GetTopmostModalWindow()->ContainsPoint(inputEvent.MousePosition)) {
                UpdateHoveredWidget(nullptr, inputEvent.MousePosition, inputEvent.Timestamp);
                continue;
            }

            const std::shared_ptr<ImWindow> topPopup = WindowManager_.GetTopmostPopupWindow();
            if (topPopup &&
                topPopup->ClosesOnClickOutside() &&
                !WindowManager_.IsPopupChainHit(inputEvent.MousePosition)) {
                WindowManager_.CloseTopPopupChain();
                CleanupInteractionState();
                UpdateHoveredWidget(
                    ResolveHoveredWidget(inputEvent.MousePosition),
                    inputEvent.MousePosition,
                    inputEvent.Timestamp);
                continue;
            }
        }

        if (inputEvent.Type == EInputEventType::MouseButtonUp &&
            inputEvent.MouseButton == EMouseButton::Left &&
            InteractionState_->PressedCloseWindow_) {
            const std::shared_ptr<ImWindow> closeWindow = InteractionState_->PressedCloseWindow_;
            closeWindow->bCloseButtonPressed_ = false;
            InteractionState_->PressedCloseWindow_.reset();

            if (closeWindow->IsOpen() && closeWindow->IsPointInCloseButton(inputEvent.MousePosition)) {
                WindowManager_.CloseWindow(closeWindow);
                CleanupInteractionState();
            }

            UpdateHoveredWidget(
                ResolveHoveredWidget(inputEvent.MousePosition),
                inputEvent.MousePosition,
                inputEvent.Timestamp);
            continue;
        }

        if (inputEvent.Type == EInputEventType::MouseButtonDown &&
            inputEvent.MouseButton == EMouseButton::Left &&
            !InteractionState_->CapturedMouseWidget_) {
            std::shared_ptr<ImWindow> hitWindow = WindowManager_.HitTestWindow(inputEvent.MousePosition);
            if (hitWindow) {
                WindowManager_.SetActiveWindowInternal(hitWindow);
                WindowManager_.BringToFront(hitWindow);
                ClearKeyboardFocusIfOutsideActiveWindow(hitWindow);

                if (hitWindow->IsPointInCloseButton(inputEvent.MousePosition) && hitWindow->IsClosable()) {
                    hitWindow->bCloseButtonPressed_ = true;
                    InteractionState_->PressedCloseWindow_ = hitWindow;
                    UpdateHoveredWidget(nullptr, inputEvent.MousePosition, inputEvent.Timestamp);
                    continue;
                }

                if (hitWindow->IsPointInTitleBar(inputEvent.MousePosition) && hitWindow->IsMovable()) {
                    InteractionState_->DraggedWindow_ = hitWindow;
                    InteractionState_->WindowDragOffset_ = inputEvent.MousePosition - hitWindow->GetPosition();
                    UpdateHoveredWidget(nullptr, inputEvent.MousePosition, inputEvent.Timestamp);
                    continue;
                }
            } else {
                ClearKeyboardFocusIfOutsideActiveWindow(nullptr);
            }
        }

        std::vector<std::shared_ptr<ImWidget>> eventPath;
        if (inputEvent.IsKeyboardEvent()) {
            eventPath = BuildPathToSceneRoot(InteractionState_->FocusedWidget_);
        } else if (inputEvent.IsMouseEvent()) {
            if (InteractionState_->CapturedMouseWidget_ &&
                inputEvent.Type != EInputEventType::MouseEnter &&
                inputEvent.Type != EInputEventType::MouseLeave) {
                eventPath = BuildPathToSceneRoot(InteractionState_->CapturedMouseWidget_);
            } else {
                FWindowWidgetTarget mouseTarget = ResolveMouseTarget(inputEvent.MousePosition);
                eventPath = std::move(mouseTarget.WidgetPath);
            }
        }

        if (inputEvent.Type == EInputEventType::MouseMove) {
            if (TryStartDragDrop(inputEvent)) {
                CleanupInteractionState();
                continue;
            }

            UpdateHoveredWidget(
                eventPath.empty() ? nullptr : eventPath.back(),
                inputEvent.MousePosition,
                inputEvent.Timestamp);
        }

        if (!eventPath.empty()) {
            ProcessReply(RouteEvent(inputEvent, eventPath));
            CleanupInteractionState();
        }

        if (inputEvent.Type == EInputEventType::MouseButtonUp &&
            DragDropState_->bArmed_ &&
            inputEvent.MouseButton == DragDropState_->ArmedButton_) {
            ClearArmedDrag();
        }
    }
}

void ImApplication::UpdateHoveredWidget(
    const std::shared_ptr<ImWidget>& hoveredWidget,
    const FVector2& cursorPosition,
    double timestamp)
{
    std::shared_ptr<ImWidget> lastHoveredWidget = InteractionState_->HoveredWidget_.lock();
    if (lastHoveredWidget == hoveredWidget) {
        ToolTipState_->AnchorCursorPosition_ = cursorPosition;
        return;
    }

    if (lastHoveredWidget) {
        FInputEvent leaveEvent;
        leaveEvent.Type = EInputEventType::MouseLeave;
        leaveEvent.MousePosition = cursorPosition;
        leaveEvent.Timestamp = timestamp;

        const std::vector<std::shared_ptr<ImWidget>> leavePath = BuildPathToSceneRoot(lastHoveredWidget);
        if (!leavePath.empty()) {
            ProcessReply(RouteEvent(leaveEvent, leavePath));
        }
    }

    DismissToolTip();
    InteractionState_->HoveredWidget_ = hoveredWidget;
    ResetToolTipState();
    ToolTipState_->SourceWidget_ = hoveredWidget;
    ToolTipState_->SourceToolTipWidget_ = hoveredWidget ? hoveredWidget->GetToolTipWidget() : nullptr;
    ToolTipState_->SourceToolTipText_ = hoveredWidget ? hoveredWidget->GetToolTipText() : std::string();
    ToolTipState_->AnchorCursorPosition_ = cursorPosition;
    ToolTipState_->HoverStartTime_ = timestamp;
    ToolTipState_->bPendingDisplay_ = hoveredWidget && hoveredWidget->HasToolTip();

    if (hoveredWidget) {
        FInputEvent enterEvent;
        enterEvent.Type = EInputEventType::MouseEnter;
        enterEvent.MousePosition = cursorPosition;
        enterEvent.Timestamp = timestamp;

        const std::vector<std::shared_ptr<ImWidget>> enterPath = BuildPathToSceneRoot(hoveredWidget);
        if (!enterPath.empty()) {
            ProcessReply(RouteEvent(enterEvent, enterPath));
        }
    }
}

void ImApplication::ProcessReply(const FReply& reply)
{
    if (reply.bDetectDrag && reply.DragDetectTarget) {
        FInputEvent dragDetectEvent;
        dragDetectEvent.Type = EInputEventType::MouseButtonDown;
        dragDetectEvent.MouseButton = reply.DragDetectButton;
        dragDetectEvent.MousePosition = InteractionState_->LastCursorPosition_;
        BeginArmedDrag(reply.DragDetectTarget, reply.DragDetectButton, dragDetectEvent);
    }

    if (reply.bReleaseMouseCapture) {
        ReleaseMouseCapture();
    }
    if (reply.MouseCaptureTarget) {
        SetMouseCapture(reply.MouseCaptureTarget, reply.MouseCaptureButton);
    }

    if (reply.bClearKeyboardFocus) {
        ClearKeyboardFocus();
    }
    if (reply.FocusTarget) {
        SetKeyboardFocus(reply.FocusTarget);
    }
}

void ImApplication::ResetInteractionState()
{
    CancelDragDrop();
    ClearKeyboardFocus();
    ReleaseMouseCapture();
    InteractionState_->HoveredWidget_.reset();
    InteractionState_->bHasCursorPosition_ = false;
    InteractionState_->LastCursorPosition_ = FVector2(0.0f, 0.0f);
    InteractionState_->DraggedWindow_.reset();
    InteractionState_->PressedCloseWindow_.reset();
    ResetToolTipState();
    DismissToolTip();
}

void ImApplication::ResetToolTipState()
{
    ToolTipState_->SourceWidget_.reset();
    ToolTipState_->ContentWidget_.reset();
    ToolTipState_->SourceToolTipWidget_.reset();
    ToolTipState_->SourceToolTipText_.clear();
    ToolTipState_->HoverStartTime_ = 0.0;
    ToolTipState_->bPendingDisplay_ = false;
}

void ImApplication::ResetDragDropState()
{
    DragDropState_->ArmedSourceWidget_.reset();
    DragDropState_->ArmedButton_ = EMouseButton::Left;
    DragDropState_->PressPosition_ = FVector2(0.0f, 0.0f);
    DragDropState_->PressModifiers_ = FInputModifiers();
    DragDropState_->PressTimestamp_ = 0.0;
    DragDropState_->bArmed_ = false;
    DragDropState_->ActiveOperation_.reset();
    DragDropState_->ActiveSourceWidget_.reset();
    DragDropState_->CurrentDropTarget_.reset();
    DragDropState_->CurrentPosition_ = FVector2(0.0f, 0.0f);
    DragDropState_->bActive_ = false;
}

void ImApplication::ClearArmedDrag()
{
    DragDropState_->ArmedSourceWidget_.reset();
    DragDropState_->ArmedButton_ = EMouseButton::Left;
    DragDropState_->PressPosition_ = FVector2(0.0f, 0.0f);
    DragDropState_->PressModifiers_ = FInputModifiers();
    DragDropState_->PressTimestamp_ = 0.0;
    DragDropState_->bArmed_ = false;
}

void ImApplication::DismissToolTip()
{
    if (ToolTipState_->Window_) {
        if (ToolTipState_->Window_->IsOpen()) {
            WindowManager_.CloseWindow(ToolTipState_->Window_);
        }
        ToolTipState_->Window_.reset();
    }

    ToolTipState_->ContentWidget_.reset();
    ToolTipState_->SourceToolTipWidget_.reset();
    ToolTipState_->SourceToolTipText_.clear();
}

void ImApplication::BeginArmedDrag(const std::shared_ptr<ImWidget>& widget, EMouseButton button, const FInputEvent& event)
{
    if (!widget || !IsWidgetInActiveTree(widget)) {
        return;
    }

    DragDropState_->ArmedSourceWidget_ = widget;
    DragDropState_->ArmedButton_ = button;
    DragDropState_->PressPosition_ = event.MousePosition;
    DragDropState_->PressModifiers_ = event.Modifiers;
    DragDropState_->PressTimestamp_ = event.Timestamp;
    DragDropState_->CurrentPosition_ = event.MousePosition;
    DragDropState_->bArmed_ = true;
}

bool ImApplication::TryStartDragDrop(const FInputEvent& event)
{
    if (!DragDropState_->bArmed_ || DragDropState_->bActive_) {
        return false;
    }

    if (!DragDropState_->ArmedSourceWidget_ || !IsWidgetInActiveTree(DragDropState_->ArmedSourceWidget_)) {
        ClearArmedDrag();
        return false;
    }

    const FVector2 delta = event.MousePosition - DragDropState_->PressPosition_;
    if (delta.LengthSquared() <= 4.0f) {
        return false;
    }

    FDragDetectEvent detectEvent;
    detectEvent.SourceWidget = DragDropState_->ArmedSourceWidget_;
    detectEvent.TriggerButton = DragDropState_->ArmedButton_;
    detectEvent.PressPosition = DragDropState_->PressPosition_;
    detectEvent.CurrentPosition = event.MousePosition;
    detectEvent.Modifiers = event.Modifiers;
    detectEvent.Timestamp = event.Timestamp;

    std::shared_ptr<FDragDropOperation> operation = DragDropState_->ArmedSourceWidget_->OnDragDetected(detectEvent);
    if (!operation || !operation->IsValid()) {
        ClearArmedDrag();
        return false;
    }

    if (!operation->Payload) {
        operation->Payload = std::make_shared<FDragDropPayload>();
    }
    if (operation->PreviewWidget) {
        operation->PreviewWidget->SetHitTestVisible(false);
    }
    operation->TriggerButton = DragDropState_->ArmedButton_;

    DragDropState_->ActiveOperation_ = operation;
    DragDropState_->ActiveSourceWidget_ = DragDropState_->ArmedSourceWidget_;
    DragDropState_->CurrentDropTarget_.reset();
    DragDropState_->CurrentPosition_ = event.MousePosition;
    DragDropState_->bActive_ = true;
    ToolTipState_->bSuppressForCurrentFrame_ = true;
    DismissToolTip();

    FDragDropEvent dragStartEvent;
    dragStartEvent.Type = EDragDropEventType::DragStart;
    dragStartEvent.Operation = operation;
    dragStartEvent.SourceWidget = DragDropState_->ActiveSourceWidget_;
    dragStartEvent.PressPosition = DragDropState_->PressPosition_;
    dragStartEvent.CurrentPosition = event.MousePosition;
    dragStartEvent.Modifiers = event.Modifiers;
    dragStartEvent.Timestamp = event.Timestamp;

    std::vector<std::shared_ptr<ImWidget>> sourcePath = BuildPathToSceneRoot(DragDropState_->ActiveSourceWidget_);
    if (!sourcePath.empty()) {
        ProcessReply(RouteDragEvent(dragStartEvent, sourcePath));
    }

    ClearArmedDrag();
    UpdateDragDrop(event);
    return true;
}

void ImApplication::UpdateDragDrop(const FInputEvent& event)
{
    if (!DragDropState_->bActive_ || !DragDropState_->ActiveOperation_ || !DragDropState_->ActiveSourceWidget_) {
        return;
    }

    if (!IsWidgetInActiveTree(DragDropState_->ActiveSourceWidget_)) {
        CompleteDragDrop(event, true);
        return;
    }

    DragDropState_->CurrentPosition_ = event.MousePosition;

    FDragDropEvent dragUpdateEvent;
    dragUpdateEvent.Type = EDragDropEventType::DragUpdate;
    dragUpdateEvent.Operation = DragDropState_->ActiveOperation_;
    dragUpdateEvent.SourceWidget = DragDropState_->ActiveSourceWidget_;
    dragUpdateEvent.TargetWidget = DragDropState_->CurrentDropTarget_;
    dragUpdateEvent.PressPosition = DragDropState_->PressPosition_;
    dragUpdateEvent.CurrentPosition = event.MousePosition;
    dragUpdateEvent.Modifiers = event.Modifiers;
    dragUpdateEvent.Timestamp = event.Timestamp;
    std::vector<std::shared_ptr<ImWidget>> sourcePath = BuildPathToSceneRoot(DragDropState_->ActiveSourceWidget_);
    if (!sourcePath.empty()) {
        ProcessReply(RouteDragEvent(dragUpdateEvent, sourcePath));
    }

    FWindowWidgetTarget mouseTarget = ResolveMouseTarget(event.MousePosition);
    FDragDropEvent dragOverEvent;
    dragOverEvent.Type = EDragDropEventType::DragOver;
    dragOverEvent.Operation = DragDropState_->ActiveOperation_;
    dragOverEvent.SourceWidget = DragDropState_->ActiveSourceWidget_;
    dragOverEvent.PressPosition = DragDropState_->PressPosition_;
    dragOverEvent.CurrentPosition = event.MousePosition;
    dragOverEvent.Modifiers = event.Modifiers;
    dragOverEvent.Timestamp = event.Timestamp;

    std::shared_ptr<ImWidget> acceptedTarget;
    FReply dragOverReply = FReply::Unhandled();
    const std::vector<std::shared_ptr<ImWidget>> dragPath =
        ResolveDragEventPath(dragOverEvent, mouseTarget.WidgetPath, acceptedTarget, dragOverReply);
    if (!dragPath.empty()) {
        ProcessReply(dragOverReply);
    }

    if (DragDropState_->CurrentDropTarget_ != acceptedTarget) {
        if (DragDropState_->CurrentDropTarget_) {
            FDragDropEvent leaveEvent;
            leaveEvent.Type = EDragDropEventType::DragLeave;
            leaveEvent.Operation = DragDropState_->ActiveOperation_;
            leaveEvent.SourceWidget = DragDropState_->ActiveSourceWidget_;
            leaveEvent.TargetWidget = DragDropState_->CurrentDropTarget_;
            leaveEvent.PressPosition = DragDropState_->PressPosition_;
            leaveEvent.CurrentPosition = event.MousePosition;
            leaveEvent.Modifiers = event.Modifiers;
            leaveEvent.Timestamp = event.Timestamp;
            const std::vector<std::shared_ptr<ImWidget>> leavePath =
                BuildPathToSceneRoot(DragDropState_->CurrentDropTarget_);
            if (!leavePath.empty()) {
                ProcessReply(RouteDragEvent(leaveEvent, leavePath));
            }
        }

        DragDropState_->CurrentDropTarget_ = acceptedTarget;
        if (DragDropState_->CurrentDropTarget_) {
            FDragDropEvent enterEvent;
            enterEvent.Type = EDragDropEventType::DragEnter;
            enterEvent.Operation = DragDropState_->ActiveOperation_;
            enterEvent.SourceWidget = DragDropState_->ActiveSourceWidget_;
            enterEvent.TargetWidget = DragDropState_->CurrentDropTarget_;
            enterEvent.PressPosition = DragDropState_->PressPosition_;
            enterEvent.CurrentPosition = event.MousePosition;
            enterEvent.Modifiers = event.Modifiers;
            enterEvent.Timestamp = event.Timestamp;
            const std::vector<std::shared_ptr<ImWidget>> enterPath =
                BuildPathToSceneRoot(DragDropState_->CurrentDropTarget_);
            if (!enterPath.empty()) {
                ProcessReply(RouteDragEvent(enterEvent, enterPath));
            }
        }
    }

    UpdateHoveredWidget(nullptr, event.MousePosition, event.Timestamp);
}

void ImApplication::CompleteDragDrop(const FInputEvent& event, bool bCancelled)
{
    if (!DragDropState_->bActive_ || !DragDropState_->ActiveOperation_) {
        ClearArmedDrag();
        return;
    }

    const std::shared_ptr<FDragDropOperation> operation = DragDropState_->ActiveOperation_;
    const std::shared_ptr<ImWidget> sourceWidget = DragDropState_->ActiveSourceWidget_;
    const std::shared_ptr<ImWidget> targetWidget = DragDropState_->CurrentDropTarget_;

    if (bCancelled && targetWidget) {
        FDragDropEvent leaveEvent;
        leaveEvent.Type = EDragDropEventType::DragLeave;
        leaveEvent.Operation = operation;
        leaveEvent.SourceWidget = sourceWidget;
        leaveEvent.TargetWidget = targetWidget;
        leaveEvent.PressPosition = DragDropState_->PressPosition_;
        leaveEvent.CurrentPosition = event.MousePosition;
        leaveEvent.Modifiers = event.Modifiers;
        leaveEvent.Timestamp = event.Timestamp;
        const std::vector<std::shared_ptr<ImWidget>> leavePath = BuildPathToSceneRoot(targetWidget);
        if (!leavePath.empty()) {
            ProcessReply(RouteDragEvent(leaveEvent, leavePath));
        }
    }

    std::shared_ptr<ImWidget> dropSucceededTarget;
    if (!bCancelled && targetWidget && IsWidgetInActiveTree(targetWidget)) {
        FDragDropEvent dropEvent;
        dropEvent.Type = EDragDropEventType::Drop;
        dropEvent.Operation = operation;
        dropEvent.SourceWidget = sourceWidget;
        dropEvent.TargetWidget = targetWidget;
        dropEvent.PressPosition = DragDropState_->PressPosition_;
        dropEvent.CurrentPosition = event.MousePosition;
        dropEvent.Modifiers = event.Modifiers;
        dropEvent.Timestamp = event.Timestamp;
        const std::vector<std::shared_ptr<ImWidget>> dropPath = BuildPathToSceneRoot(targetWidget);
        if (!dropPath.empty()) {
            const FReply dropReply = RouteDragEvent(dropEvent, dropPath);
            ProcessReply(dropReply);
            if (dropReply.IsHandled()) {
                dropSucceededTarget = targetWidget;
            }
        }
    }

    if (sourceWidget && IsWidgetInActiveTree(sourceWidget)) {
        FDragDropEvent endEvent;
        endEvent.Type = EDragDropEventType::DragEnd;
        endEvent.Operation = operation;
        endEvent.SourceWidget = sourceWidget;
        endEvent.TargetWidget = dropSucceededTarget;
        endEvent.PressPosition = DragDropState_->PressPosition_;
        endEvent.CurrentPosition = event.MousePosition;
        endEvent.Modifiers = event.Modifiers;
        endEvent.Timestamp = event.Timestamp;
        const std::vector<std::shared_ptr<ImWidget>> endPath = BuildPathToSceneRoot(sourceWidget);
        if (!endPath.empty()) {
            ProcessReply(RouteDragEvent(endEvent, endPath));
        }
    }

    ResetDragDropState();
    UpdateHoveredWidget(ResolveHoveredWidget(event.MousePosition), event.MousePosition, event.Timestamp);
}

void ImApplication::CancelDragDrop()
{
    if (!DragDropState_->bActive_) {
        ClearArmedDrag();
        return;
    }

    FInputEvent cancelEvent;
    cancelEvent.Type = EInputEventType::MouseMove;
    cancelEvent.MousePosition = DragDropState_->CurrentPosition_;
    CompleteDragDrop(cancelEvent, true);
}

std::shared_ptr<ImWidget> ImApplication::BuildToolTipContentForWidget(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget || !widget->HasToolTip()) {
        return nullptr;
    }

    if (widget->GetToolTipWidget()) {
        return widget->GetToolTipWidget();
    }

    if (!widget->GetToolTipText().empty()) {
        auto textBlock = std::make_shared<ImTextBlock>();
        textBlock->SetText(widget->GetToolTipText());
        textBlock->SetTextColor(ToolTipStyle_.TextColor);
        textBlock->SetFontSize(ToolTipStyle_.FontSize);
        textBlock->SetHitTestVisible(false);
        return textBlock;
    }

    return nullptr;
}

void ImApplication::UpdateToolTip(const FGeometry& viewportGeometry, double currentTime)
{
    if (ToolTipState_->bSuppressForCurrentFrame_) {
        return;
    }

    const std::shared_ptr<ImWidget> hoveredWidget = InteractionState_->HoveredWidget_.lock();
    if (!InteractionState_->bHasCursorPosition_ ||
        InteractionState_->DraggedWindow_ ||
        InteractionState_->CapturedMouseWidget_ ||
        InteractionState_->PressedCloseWindow_ ||
        !hoveredWidget ||
        !hoveredWidget->HasToolTip()) {
        DismissToolTip();
        return;
    }

    const std::shared_ptr<ImWindow> ownerWindow = WindowManager_.FindOwningWindow(hoveredWidget);
    if (!ownerWindow || WindowManager_.IsBlockedByModal(ownerWindow)) {
        DismissToolTip();
        return;
    }

    const std::shared_ptr<ImWidget> currentToolTipWidget = hoveredWidget->GetToolTipWidget();
    const std::string currentToolTipText = hoveredWidget->GetToolTipText();
    const bool bToolTipSpecChanged =
        currentToolTipWidget != ToolTipState_->SourceToolTipWidget_ ||
        currentToolTipText != ToolTipState_->SourceToolTipText_;
    if (bToolTipSpecChanged) {
        DismissToolTip();
        ToolTipState_->SourceToolTipWidget_ = currentToolTipWidget;
        ToolTipState_->SourceToolTipText_ = currentToolTipText;
        ToolTipState_->HoverStartTime_ = currentTime;
        ToolTipState_->bPendingDisplay_ = hoveredWidget && hoveredWidget->HasToolTip();
    }

    ToolTipState_->AnchorCursorPosition_ = InteractionState_->LastCursorPosition_;
    const auto updateWindowPlacement = [&](const std::shared_ptr<ImWindow>& window) {
        if (!window) {
            return;
        }

        FVector2 position = ToolTipState_->AnchorCursorPosition_ + ToolTipStyle_.CursorOffset;
        const FVector2 size = window->GetSize();
        const float minX = viewportGeometry.Position.X;
        const float minY = viewportGeometry.Position.Y;
        const float maxX = viewportGeometry.Position.X + viewportGeometry.Size.X;
        const float maxY = viewportGeometry.Position.Y + viewportGeometry.Size.Y;

        position.X = std::clamp(position.X, minX, std::max(minX, maxX - size.X));
        position.Y = std::clamp(position.Y, minY, std::max(minY, maxY - size.Y));
        window->SetPosition(position);
    };

    if (ToolTipState_->Window_ && ToolTipState_->Window_->IsOpen()) {
        updateWindowPlacement(ToolTipState_->Window_);
        return;
    }

    const bool bDelayElapsed =
        !ToolTipState_->bPendingDisplay_ ||
        (currentTime - ToolTipState_->HoverStartTime_) >= ToolTipStyle_.ShowDelaySeconds;
    if (!bDelayElapsed) {
        return;
    }

    std::shared_ptr<ImWidget> toolTipContent = BuildToolTipContentForWidget(hoveredWidget);
    if (!toolTipContent) {
        ResetToolTipState();
        return;
    }

    auto host = std::make_shared<ImToolTipHostWidget>(ToolTipStyle_);
    host->SetContent(toolTipContent);
    const FVector2 toolTipSize = host->GetMinSize();

    FPopupOptions popupOptions;
    popupOptions.Title = "ToolTip";
    popupOptions.Position = ToolTipState_->AnchorCursorPosition_ + ToolTipStyle_.CursorOffset;
    popupOptions.Size = toolTipSize;
    popupOptions.RootWidget = host;
    popupOptions.ParentWindow = ownerWindow;
    popupOptions.bClosable = false;
    popupOptions.bCloseOnClickOutside = false;
    popupOptions.bHasBackground = true;
    popupOptions.bHasTitleBar = false;
    popupOptions.bHitTestVisible = false;
    popupOptions.bActivatable = false;
    popupOptions.Style.BackgroundColor = ToolTipStyle_.BackgroundColor;
    popupOptions.Style.InactiveBackgroundColor = ToolTipStyle_.BackgroundColor;
    popupOptions.Style.BorderColor = ToolTipStyle_.BorderColor;
    popupOptions.Style.ActiveBorderColor = ToolTipStyle_.BorderColor;
    popupOptions.Style.CornerRadius = ToolTipStyle_.CornerRadius;
    popupOptions.Style.BorderThickness = ToolTipStyle_.BorderThickness;
    popupOptions.Style.bDrawShadow = false;

    ToolTipState_->ContentWidget_ = toolTipContent;
    ToolTipState_->SourceToolTipWidget_ = currentToolTipWidget;
    ToolTipState_->SourceToolTipText_ = currentToolTipText;
    ToolTipState_->Window_ = WindowManager_.CreateToolTip(popupOptions);
    ToolTipState_->bPendingDisplay_ = false;
    updateWindowPlacement(ToolTipState_->Window_);
}

void ImApplication::CleanupInteractionState()
{
    const std::shared_ptr<ImWindow> modalWindow = WindowManager_.GetTopmostModalWindow();

    if (DragDropState_->bActive_) {
        if (!DragDropState_->ActiveSourceWidget_ || !IsWidgetInActiveTree(DragDropState_->ActiveSourceWidget_)) {
            CancelDragDrop();
        } else if (DragDropState_->CurrentDropTarget_ && !IsWidgetInActiveTree(DragDropState_->CurrentDropTarget_)) {
            DragDropState_->CurrentDropTarget_.reset();
        }
    } else if (DragDropState_->bArmed_ &&
               (!DragDropState_->ArmedSourceWidget_ || !IsWidgetInActiveTree(DragDropState_->ArmedSourceWidget_))) {
        ClearArmedDrag();
    }

    if (InteractionState_->FocusedWidget_) {
        const std::shared_ptr<ImWindow> focusWindow = WindowManager_.FindOwningWindow(InteractionState_->FocusedWidget_);
        if (!focusWindow || (modalWindow && focusWindow != modalWindow)) {
            ClearKeyboardFocus();
        }
    }

    if (InteractionState_->CapturedMouseWidget_) {
        const std::shared_ptr<ImWindow> captureWindow = WindowManager_.FindOwningWindow(InteractionState_->CapturedMouseWidget_);
        if (!captureWindow || (modalWindow && captureWindow != modalWindow)) {
            ReleaseMouseCapture();
        }
    }

    std::shared_ptr<ImWidget> hoveredWidget = InteractionState_->HoveredWidget_.lock();
    if (hoveredWidget) {
        const std::shared_ptr<ImWindow> hoverWindow = WindowManager_.FindOwningWindow(hoveredWidget);
        if (!hoverWindow || (modalWindow && hoverWindow != modalWindow)) {
            UpdateHoveredWidget(
                nullptr,
                InteractionState_->LastCursorPosition_,
                0.0);
        }
    }

    if (InteractionState_->DraggedWindow_ && !InteractionState_->DraggedWindow_->IsOpen()) {
        InteractionState_->DraggedWindow_.reset();
    }

    if (InteractionState_->PressedCloseWindow_ && !InteractionState_->PressedCloseWindow_->IsOpen()) {
        InteractionState_->PressedCloseWindow_.reset();
    }
}

void ImApplication::ClearKeyboardFocusIfOutsideActiveWindow(const std::shared_ptr<ImWindow>& activeWindow)
{
    if (!InteractionState_->FocusedWidget_) {
        return;
    }

    if (WindowManager_.FindOwningWindow(InteractionState_->FocusedWidget_) != activeWindow) {
        ClearKeyboardFocus();
    }
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::BuildPathToSceneRoot(
    const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget) {
        return {};
    }

    for (const std::shared_ptr<ImWindow>& window : WindowManager_.GetOpenWindows()) {
        if (!window->GetRootWidget()) {
            continue;
        }

        std::vector<std::shared_ptr<ImWidget>> path =
            PathResolver_->BuildPathToSceneRoot(window->GetRootWidget(), widget);
        if (!path.empty()) {
            return path;
        }
    }

    return {};
}

FReply ImApplication::RouteEvent(
    const FInputEvent& event,
    const std::vector<std::shared_ptr<ImWidget>>& eventPath)
{
    return EventRouter_->Route(event, eventPath);
}

FReply ImApplication::RouteDragEvent(
    const FDragDropEvent& event,
    const std::vector<std::shared_ptr<ImWidget>>& eventPath)
{
    return EventRouter_->RouteDrag(event, eventPath);
}

std::vector<std::shared_ptr<ImWidget>> ImApplication::ResolveDragEventPath(
    const FDragDropEvent& event,
    const std::vector<std::shared_ptr<ImWidget>>& hitPath,
    std::shared_ptr<ImWidget>& outAcceptedTarget,
    FReply& outReply)
{
    outAcceptedTarget.reset();
    outReply = FReply::Unhandled();
    if (hitPath.empty()) {
        return {};
    }

    std::vector<std::shared_ptr<ImWidget>> probePath = hitPath;
    while (!probePath.empty()) {
        FReply reply = RouteDragEvent(event, probePath);
        if (reply.IsHandled()) {
            outAcceptedTarget = probePath.back();
            outReply = reply;
            return probePath;
        }
        probePath.pop_back();
    }

    return {};
}

void ImApplication::PerformLayoutPass()
{
    for (const std::shared_ptr<ImWindow>& window : WindowManager_.GetOpenWindows()) {
        if (!window->GetRootWidget()) {
            continue;
        }

        window->GetRootWidget()->SetGeometry(window->GetContentGeometry());
    }
}

void ImApplication::PaintWindows(const FFrameContext& frameContext, const FGeometry& viewportGeometry)
{
    for (const std::shared_ptr<ImWindow>& window : WindowManager_.GetOpenWindows()) {
        DrawWindowChrome(*frameContext.DrawContext_, *window, viewportGeometry);

        if (!window->GetRootWidget()) {
            continue;
        }

        const FGeometry contentGeometry = window->GetContentGeometry();
        frameContext.DrawContext_->PushClipRect(contentGeometry.GetMin(), contentGeometry.GetMax(), true);

        FPaintContext paintContext(
            *frameContext.DrawContext_,
            contentGeometry,
            &StyleSet_,
            InteractionState_->LastCursorPosition_,
            InteractionState_->bHasCursorPosition_,
            frameContext.FrameInfo.DeltaTime);
        window->GetRootWidget()->Paint(paintContext);

        frameContext.DrawContext_->PopClipRect();
    }

    PaintDragDropPreview(frameContext);
}

ImApplication::FWindowWidgetTarget ImApplication::ResolveMouseTarget(const FVector2& position) const
{
    FWindowWidgetTarget result;
    result.Window = WindowManager_.HitTestContentWindow(position);
    if (!result.Window || !result.Window->GetRootWidget()) {
        return result;
    }

    result.WidgetPath = PathResolver_->BuildHitTestPath(result.Window->GetRootWidget(), position);
    if (result.WidgetPath.empty()) {
        result.Window.reset();
    }

    return result;
}

std::shared_ptr<ImWidget> ImApplication::ResolveHoveredWidget(const FVector2& position) const
{
    const FWindowWidgetTarget target = ResolveMouseTarget(position);
    return target.WidgetPath.empty() ? nullptr : target.WidgetPath.back();
}

void ImApplication::PaintDragDropPreview(const FFrameContext& frameContext)
{
    if (!frameContext.DrawContext_ ||
        !DragDropState_->bActive_ ||
        !DragDropState_->ActiveOperation_ ||
        !DragDropState_->ActiveOperation_->PreviewWidget) {
        return;
    }

    const std::shared_ptr<ImWidget>& previewWidget = DragDropState_->ActiveOperation_->PreviewWidget;
    previewWidget->SetHitTestVisible(false);
    const FVector2 previewSize = previewWidget->GetMinSize();
    const FVector2 previewPosition = DragDropState_->CurrentPosition_ + DragDropState_->ActiveOperation_->PreviewOffset;
    const FGeometry previewGeometry(previewPosition, previewSize);
    previewWidget->SetGeometry(previewGeometry);

    FPaintContext paintContext(
        *frameContext.DrawContext_,
        previewGeometry,
        &StyleSet_,
        InteractionState_->LastCursorPosition_,
        InteractionState_->bHasCursorPosition_,
        frameContext.FrameInfo.DeltaTime);
    previewWidget->Paint(paintContext);
}

bool ImApplication::IsWidgetInActiveTree(const std::shared_ptr<ImWidget>& widget) const
{
    return !BuildPathToSceneRoot(widget).empty();
}

bool ImApplication::CanMutateTitleBarTabMenus() const
{
    return Backend_ != nullptr && Backend_->IsUsingCustomHostChrome();
}

bool ImApplication::CanMutateTitleBarActions() const
{
    return Backend_ != nullptr && Backend_->IsUsingCustomHostChrome();
}

void ImApplication::SyncApplicationTitle()
{
    if (Backend_ == nullptr || ApplicationTitle_.empty()) {
        return;
    }

    Backend_->SetWindowTitle(ApplicationTitle_);
}

void ImApplication::SyncApplicationIcon()
{
    if (Backend_ == nullptr) {
        return;
    }

    if (!ApplicationIcon_.IsValid()) {
        Backend_->ClearWindowIcon();
        return;
    }

    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!TryResolveBrushPixels(ApplicationIcon_, pixels, width, height)) {
        return;
    }

    Backend_->SetWindowIconFromRGBA(pixels.data(), width, height);
}

bool ImApplication::TryResolveBrushPixels(
    const FImageBrush& brush,
    std::vector<std::uint8_t>& outPixels,
    int& outWidth,
    int& outHeight) const
{
    outPixels.clear();
    outWidth = 0;
    outHeight = 0;

    if (!brush.IsValid()) {
        return false;
    }

    FRuntimeTextureData textureData;
    if (!FindRuntimeTextureData(brush.TextureId, textureData) ||
        textureData.Width <= 0 ||
        textureData.Height <= 0 ||
        textureData.BytesPerPixel < 4) {
        return false;
    }

    const float uMin = std::clamp(std::min(brush.Uv0.X, brush.Uv1.X), 0.0f, 1.0f);
    const float uMax = std::clamp(std::max(brush.Uv0.X, brush.Uv1.X), 0.0f, 1.0f);
    const float vMin = std::clamp(std::min(brush.Uv0.Y, brush.Uv1.Y), 0.0f, 1.0f);
    const float vMax = std::clamp(std::max(brush.Uv0.Y, brush.Uv1.Y), 0.0f, 1.0f);

    outWidth = std::max(
        1,
        static_cast<int>(std::lround(
            brush.SourceSize.X > 0.0f
                ? brush.SourceSize.X
                : (uMax - uMin) * static_cast<float>(textureData.Width))));
    outHeight = std::max(
        1,
        static_cast<int>(std::lround(
            brush.SourceSize.Y > 0.0f
                ? brush.SourceSize.Y
                : (vMax - vMin) * static_cast<float>(textureData.Height))));
    outPixels.resize(static_cast<std::size_t>(outWidth) * static_cast<std::size_t>(outHeight) * 4U, 0U);

    for (int y = 0; y < outHeight; ++y) {
        const float v = vMin + ((static_cast<float>(y) + 0.5f) / static_cast<float>(outHeight)) * (vMax - vMin);
        const int sourceY = std::clamp(
            static_cast<int>(v * static_cast<float>(textureData.Height)),
            0,
            textureData.Height - 1);
        for (int x = 0; x < outWidth; ++x) {
            const float u = uMin + ((static_cast<float>(x) + 0.5f) / static_cast<float>(outWidth)) * (uMax - uMin);
            const int sourceX = std::clamp(
                static_cast<int>(u * static_cast<float>(textureData.Width)),
                0,
                textureData.Width - 1);

            const std::size_t sourceOffset =
                (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(textureData.Width) +
                 static_cast<std::size_t>(sourceX)) * 4U;
            const std::size_t destinationOffset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(outWidth) +
                 static_cast<std::size_t>(x)) * 4U;
            outPixels[destinationOffset] = textureData.Pixels[sourceOffset];
            outPixels[destinationOffset + 1] = textureData.Pixels[sourceOffset + 1];
            outPixels[destinationOffset + 2] = textureData.Pixels[sourceOffset + 2];
            outPixels[destinationOffset + 3] = textureData.Pixels[sourceOffset + 3];
        }
    }

    return true;
}

void ImApplication::PromoteBrushToBackendTexture(FImageBrush& brush)
{
    if (Backend_ == nullptr || !brush.IsValid()) {
        return;
    }

    FRuntimeTextureData textureData;
    if (!FindRuntimeTextureData(brush.TextureId, textureData) || textureData.bUsesBackendTexture) {
        return;
    }

    const std::vector<std::uint8_t> pixels = textureData.Pixels;
    const int width = textureData.Width;
    const int height = textureData.Height;
    const ImTextureID oldTextureId = brush.TextureId;
    ImTextureID newTextureId = CreateRuntimeTextureFromRgba(pixels, width, height);
    if (newTextureId == nullptr || newTextureId == oldTextureId) {
        return;
    }

    brush.TextureId = newTextureId;
    ReleaseRuntimeTexture(oldTextureId);
}

void ImApplication::EnsureDefaultImagePlaceholderInitialized() const
{
    if (bDefaultImagePlaceholderInitialized_ && DefaultImagePlaceholderBrush_.IsValid()) {
        return;
    }

    const std::vector<std::uint8_t> pixels = BuildDefaultImagePlaceholderPixels(16, 16);
    ImTextureID textureId = const_cast<ImApplication*>(this)->CreateRuntimeTextureFromRgba(pixels, 16, 16);
    DefaultImagePlaceholderBrush_.TextureId = textureId;
    DefaultImagePlaceholderBrush_.SourceSize = FVector2(16.0f, 16.0f);
    DefaultImagePlaceholderBrush_.Uv0 = FVector2(0.0f, 0.0f);
    DefaultImagePlaceholderBrush_.Uv1 = FVector2(1.0f, 1.0f);
    DefaultImagePlaceholderBrush_.TintColor = FColor::White;
    bDefaultImagePlaceholderInitialized_ = textureId != nullptr;
}

void ImApplication::EnsureCoreIconAtlasInitialized() const
{
    if (bCoreIconAtlasInitialized_ &&
        CoreIconAtlasTexture_ != nullptr &&
        CoreIconBrushes_.size() == static_cast<std::size_t>(CoreIconInternal::GetIconCount())) {
        return;
    }

    const std::vector<std::uint8_t>& atlasPixels = CoreIconInternal::GetAtlasPixels();
    ImTextureID textureId = const_cast<ImApplication*>(this)->CreateRuntimeTextureFromRgba(
        atlasPixels,
        CoreIconInternal::AtlasWidth,
        CoreIconInternal::AtlasHeight);
    if (textureId == nullptr) {
        return;
    }

    CoreIconAtlasTexture_ = textureId;
    CoreIconBrushes_.clear();
    CoreIconBrushes_.reserve(static_cast<std::size_t>(CoreIconInternal::GetIconCount()));
    for (int iconIndex = 0; iconIndex < CoreIconInternal::GetIconCount(); ++iconIndex) {
        CoreIconBrushes_.push_back(
            CoreIconInternal::MakeBrush(textureId, static_cast<ECoreIcon>(iconIndex)));
    }

    bCoreIconAtlasInitialized_ = true;
}

std::vector<std::uint8_t> ImApplication::BuildDefaultImagePlaceholderPixels(int width, int height)
{
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U,
        0U);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::array<std::uint8_t, 4> color = SamplePlaceholderColor(x, y, width, height);
            const std::size_t offset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            pixels[offset] = color[0];
            pixels[offset + 1] = color[1];
            pixels[offset + 2] = color[2];
            pixels[offset + 3] = color[3];
        }
    }

    return pixels;
}

} // namespace ImWidgetV4
