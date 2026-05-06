#pragma once

#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

class ImWindowManager;

enum class EWindowKind {
    Normal,
    Popup,
    Modal,
    Tooltip
};

struct FWindowStyle {
    float TitleBarHeight = 32.0f;
    float TitleFontSize = 16.0f;
    float CloseButtonSize = 14.0f;
    float CornerRadius = 8.0f;
    float BorderThickness = 1.0f;
    bool bDrawShadow = true;
    FVector2 ShadowOffset {4.0f, 8.0f};
    FColor ShadowColor = FColor::FromBytes(0, 0, 0, 72);
    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor InactiveBackgroundColor = FColor::FromBytes(20, 24, 30);
    FColor TitleBarColor = FColor::FromBytes(36, 42, 52);
    FColor ActiveTitleBarColor = FColor::FromBytes(59, 93, 143);
    FColor BorderColor = FColor::FromBytes(76, 88, 106);
    FColor ActiveBorderColor = FColor::FromBytes(110, 168, 255);
    FColor TitleTextColor = FColor::FromBytes(242, 245, 248);
    FColor InactiveTitleTextColor = FColor::FromBytes(172, 180, 191);
    FColor CloseButtonColor = FColor::FromBytes(130, 136, 145);
    FColor CloseButtonHoveredColor = FColor::FromBytes(204, 82, 82);
    FColor CloseButtonPressedColor = FColor::FromBytes(168, 55, 55);
    FColor CloseGlyphColor = FColor::FromBytes(250, 250, 250);
    FColor ModalOverlayColor = FColor::FromBytes(7, 10, 16, 168);
};

struct FWindowOptions {
    std::string Title = "Window";
    FVector2 Position {96.0f, 96.0f};
    FVector2 Size {320.0f, 220.0f};
    std::shared_ptr<ImWidget> RootWidget;
    FWindowStyle Style {};
    bool bIsOpen = true;
    bool bMovable = true;
    bool bClosable = true;
    bool bHasTitleBar = true;
    bool bHasBackground = true;
    bool bFillViewport = false;
};

struct FPopupOptions {
    std::string Title = "Popup";
    FVector2 Position {120.0f, 120.0f};
    FVector2 Size {240.0f, 160.0f};
    std::shared_ptr<ImWidget> RootWidget;
    std::shared_ptr<class ImWindow> ParentWindow;
    FWindowStyle Style {};
    bool bIsOpen = true;
    bool bMovable = false;
    bool bClosable = true;
    bool bHasTitleBar = false;
    bool bHasBackground = true;
    bool bCloseOnClickOutside = true;
    bool bHitTestVisible = true;
    bool bActivatable = true;
};

class ImWindow : public std::enable_shared_from_this<ImWindow> {
public:
    using Ptr = std::shared_ptr<ImWindow>;

    void SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget);
    const std::shared_ptr<ImWidget>& GetRootWidget() const { return RootWidget_; }

    void SetTitle(const std::string& title) { Title_ = title; }
    const std::string& GetTitle() const { return Title_; }

    void SetPosition(const FVector2& position) { Position_ = position; }
    const FVector2& GetPosition() const { return Position_; }

    void SetSize(const FVector2& size);
    const FVector2& GetSize() const { return Size_; }

    void Open();
    void Close();
    bool IsOpen() const { return bIsOpen_; }

    void SetMovable(bool bMovable) { bIsMovable_ = bMovable; }
    bool IsMovable() const { return bIsMovable_; }

    void SetClosable(bool bClosable) { bIsClosable_ = bClosable; }
    bool IsClosable() const { return bIsClosable_; }
    bool HasTitleBar() const { return bHasTitleBar_; }
    bool HasBackground() const { return bHasBackground_; }
    bool IsActive() const { return bIsActive_; }
    bool IsHitTestVisible() const { return bIsHitTestVisible_; }
    bool IsActivatable() const { return bIsActivatable_; }
    bool IsCloseButtonHovered() const { return bCloseButtonHovered_; }
    bool IsCloseButtonPressed() const { return bCloseButtonPressed_; }

    void SetStyle(const FWindowStyle& style) { Style_ = style; }
    const FWindowStyle& GetStyle() const { return Style_; }

    EWindowKind GetKind() const { return Kind_; }

    FGeometry GetWindowGeometry() const;
    FGeometry GetTitleBarGeometry() const;
    FGeometry GetContentGeometry() const;
    FGeometry GetCloseButtonGeometry() const;
    bool ContainsPoint(const FVector2& point) const;
    bool IsPointInContent(const FVector2& point) const;
    bool IsPointInTitleBar(const FVector2& point) const;
    bool IsPointInCloseButton(const FVector2& point) const;

private:
    friend class ImApplication;
    friend class ImWindowManager;

    ImWindow(ImWindowManager* manager, EWindowKind kind, const FWindowOptions& options);
    ImWindow(ImWindowManager* manager, EWindowKind kind, const FPopupOptions& options);

    void OpenInternal();
    void CloseInternal();
    void SetParentWindow(const Ptr& parentWindow);
    void AddChildWindow(const Ptr& childWindow);
    void RemoveChildWindow(const Ptr& childWindow);
    void CloseChildrenRecursive();
    bool IsDescendantOf(const Ptr& ancestor) const;
    bool UsesViewportFill() const { return bFillViewport_; }
    bool ClosesOnClickOutside() const { return bCloseOnClickOutside_; }

    ImWindowManager* Manager_ = nullptr;
    std::shared_ptr<ImWidget> RootWidget_;
    std::weak_ptr<ImWindow> ParentWindow_;
    std::vector<std::weak_ptr<ImWindow>> ChildWindows_;
    std::string Title_;
    FVector2 Position_ {0.0f, 0.0f};
    FVector2 Size_ {0.0f, 0.0f};
    FWindowStyle Style_ {};
    EWindowKind Kind_ = EWindowKind::Normal;
    bool bIsOpen_ = true;
    bool bIsActive_ = false;
    bool bIsMovable_ = true;
    bool bIsClosable_ = true;
    bool bHasTitleBar_ = true;
    bool bHasBackground_ = true;
    bool bFillViewport_ = false;
    bool bCloseOnClickOutside_ = false;
    bool bIsHitTestVisible_ = true;
    bool bIsActivatable_ = true;
    bool bCloseButtonHovered_ = false;
    bool bCloseButtonPressed_ = false;
};

} // namespace ImWidgetV4
