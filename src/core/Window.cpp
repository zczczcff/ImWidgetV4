#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/core/WindowManager.h>
#include <algorithm>

namespace ImWidgetV4 {

namespace {

FVector2 SanitizeSize(const FVector2& size)
{
    return FVector2((std::max)(1.0f, size.X), (std::max)(1.0f, size.Y));
}

} // namespace

ImWindow::ImWindow(ImWindowManager* manager, EWindowKind kind, const FWindowOptions& options)
    : Manager_(manager)
    , RootWidget_(options.RootWidget)
    , Title_(options.Title)
    , Position_(options.Position)
    , Size_(SanitizeSize(options.Size))
    , Style_(options.Style)
    , Kind_(kind)
    , bIsOpen_(options.bIsOpen)
    , bIsMovable_(options.bMovable)
    , bIsClosable_(options.bClosable)
    , bHasTitleBar_(options.bHasTitleBar)
    , bHasBackground_(options.bHasBackground)
    , bFillViewport_(options.bFillViewport)
{
}

ImWindow::ImWindow(ImWindowManager* manager, EWindowKind kind, const FPopupOptions& options)
    : Manager_(manager)
    , RootWidget_(options.RootWidget)
    , Title_(options.Title)
    , Position_(options.Position)
    , Size_(SanitizeSize(options.Size))
    , Style_(options.Style)
    , Kind_(kind)
    , bIsOpen_(options.bIsOpen)
    , bIsMovable_(kind == EWindowKind::Modal ? false : options.bMovable)
    , bIsClosable_(options.bClosable)
    , bHasTitleBar_(kind == EWindowKind::Modal ? true : options.bHasTitleBar)
    , bHasBackground_(options.bHasBackground)
    , bCloseOnClickOutside_(options.bCloseOnClickOutside && kind == EWindowKind::Popup)
{
}

void ImWindow::SetRootWidget(const std::shared_ptr<ImWidget>& rootWidget)
{
    RootWidget_ = rootWidget;
}

void ImWindow::SetSize(const FVector2& size)
{
    Size_ = SanitizeSize(size);
}

void ImWindow::Open()
{
    if (Manager_ == nullptr) {
        OpenInternal();
        return;
    }

    Manager_->OpenWindowInternal(shared_from_this());
}

void ImWindow::Close()
{
    if (Manager_ == nullptr) {
        CloseInternal();
        return;
    }

    Manager_->CloseWindow(shared_from_this());
}

FGeometry ImWindow::GetWindowGeometry() const
{
    return FGeometry(Position_, Size_);
}

FGeometry ImWindow::GetTitleBarGeometry() const
{
    if (!bHasTitleBar_) {
        return FGeometry(Position_, FVector2(Size_.X, 0.0f));
    }

    return FGeometry(Position_, FVector2(Size_.X, Style_.TitleBarHeight));
}

FGeometry ImWindow::GetContentGeometry() const
{
    const float contentY = Position_.Y + (bHasTitleBar_ ? Style_.TitleBarHeight : 0.0f);
    const float contentHeight = (std::max)(0.0f, Size_.Y - (contentY - Position_.Y));
    return FGeometry(FVector2(Position_.X, contentY), FVector2(Size_.X, contentHeight));
}

FGeometry ImWindow::GetCloseButtonGeometry() const
{
    if (!bHasTitleBar_ || !bIsClosable_) {
        return FGeometry(FVector2(Position_.X, Position_.Y), FVector2(0.0f, 0.0f));
    }

    const float buttonSize = Style_.CloseButtonSize;
    const float margin = (std::max)(6.0f, (Style_.TitleBarHeight - buttonSize) * 0.5f);
    const float x = Position_.X + Size_.X - margin - buttonSize;
    const float y = Position_.Y + (Style_.TitleBarHeight - buttonSize) * 0.5f;
    return FGeometry(FVector2(x, y), FVector2(buttonSize, buttonSize));
}

bool ImWindow::ContainsPoint(const FVector2& point) const
{
    return GetWindowGeometry().Contains(point);
}

bool ImWindow::IsPointInContent(const FVector2& point) const
{
    return GetContentGeometry().Contains(point);
}

bool ImWindow::IsPointInTitleBar(const FVector2& point) const
{
    if (!bHasTitleBar_ || !GetTitleBarGeometry().Contains(point)) {
        return false;
    }

    return !IsPointInCloseButton(point);
}

bool ImWindow::IsPointInCloseButton(const FVector2& point) const
{
    return bHasTitleBar_ && bIsClosable_ && GetCloseButtonGeometry().Contains(point);
}

void ImWindow::OpenInternal()
{
    bIsOpen_ = true;
}

void ImWindow::CloseInternal()
{
    bIsOpen_ = false;
    bIsActive_ = false;
    bCloseButtonHovered_ = false;
    bCloseButtonPressed_ = false;
}

void ImWindow::SetParentWindow(const Ptr& parentWindow)
{
    ParentWindow_ = parentWindow;
}

void ImWindow::AddChildWindow(const Ptr& childWindow)
{
    if (!childWindow || childWindow.get() == this) {
        return;
    }

    for (const std::weak_ptr<ImWindow>& child : ChildWindows_) {
        if (child.lock() == childWindow) {
            return;
        }
    }

    ChildWindows_.push_back(childWindow);
    childWindow->SetParentWindow(shared_from_this());
}

void ImWindow::RemoveChildWindow(const Ptr& childWindow)
{
    ChildWindows_.erase(
        std::remove_if(
            ChildWindows_.begin(),
            ChildWindows_.end(),
            [&](const std::weak_ptr<ImWindow>& child) {
                return child.lock() == childWindow;
            }),
        ChildWindows_.end());

    if (childWindow && childWindow->ParentWindow_.lock().get() == this) {
        childWindow->ParentWindow_.reset();
    }
}

void ImWindow::CloseChildrenRecursive()
{
    std::vector<Ptr> children;
    for (const std::weak_ptr<ImWindow>& child : ChildWindows_) {
        if (Ptr locked = child.lock()) {
            children.push_back(locked);
        }
    }

    for (const Ptr& child : children) {
        if (Manager_ != nullptr) {
            Manager_->CloseWindow(child);
        } else {
            child->CloseInternal();
        }
    }
}

bool ImWindow::IsDescendantOf(const Ptr& ancestor) const
{
    if (!ancestor) {
        return false;
    }

    Ptr current = ParentWindow_.lock();
    while (current) {
        if (current == ancestor) {
            return true;
        }
        current = current->ParentWindow_.lock();
    }

    return false;
}

} // namespace ImWidgetV4
