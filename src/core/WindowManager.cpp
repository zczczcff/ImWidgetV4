#include <imwidgetv4/core/WindowManager.h>
#include <algorithm>

namespace ImWidgetV4 {

ImWindowManager::ImWindowManager() = default;

ImWindowManager::Ptr ImWindowManager::CreateWindow(const FWindowOptions& options)
{
    Ptr window = std::shared_ptr<ImWindow>(new ImWindow(this, EWindowKind::Normal, options));
    Windows_.push_back(window);
    OpenWindowInternal(window);
    return window;
}

ImWindowManager::Ptr ImWindowManager::CreatePopup(const FPopupOptions& options)
{
    Ptr window = std::shared_ptr<ImWindow>(new ImWindow(this, EWindowKind::Popup, options));
    if (options.ParentWindow) {
        options.ParentWindow->AddChildWindow(window);
    }

    Windows_.push_back(window);
    OpenWindowInternal(window);
    PopupStack_.push_back(window);
    return window;
}

ImWindowManager::Ptr ImWindowManager::CreateModal(const FPopupOptions& options)
{
    if (ModalWindow_ && ModalWindow_->IsOpen()) {
        CloseWindow(ModalWindow_);
    }

    Ptr window = std::shared_ptr<ImWindow>(new ImWindow(this, EWindowKind::Modal, options));
    if (options.ParentWindow) {
        options.ParentWindow->AddChildWindow(window);
    }

    Windows_.push_back(window);
    ModalWindow_ = window;
    OpenWindowInternal(window);
    return window;
}

void ImWindowManager::CloseWindow(const Ptr& window)
{
    if (!window) {
        return;
    }

    CloseWindowRecursive(window);
}

void ImWindowManager::BringToFront(const Ptr& window)
{
    if (!window) {
        return;
    }

    auto it = std::find(Windows_.begin(), Windows_.end(), window);
    if (it == Windows_.end()) {
        return;
    }

    Ptr keepAlive = *it;
    Windows_.erase(it);
    const std::size_t index =
        keepAlive->Kind_ == EWindowKind::Normal && keepAlive->UsesViewportFill()
            ? FindInsertIndexForViewportFillNormal(keepAlive)
            : FindInsertIndexForFront(keepAlive->Kind_);
    Windows_.insert(Windows_.begin() + static_cast<std::ptrdiff_t>(index), keepAlive);
}

std::vector<ImWindowManager::Ptr> ImWindowManager::GetOpenWindows() const
{
    std::vector<Ptr> openWindows;
    for (const Ptr& window : Windows_) {
        if (window && window->IsOpen()) {
            openWindows.push_back(window);
        }
    }

    return openWindows;
}

void ImWindowManager::OpenWindowInternal(const Ptr& window)
{
    if (!window) {
        return;
    }

    window->OpenInternal();

    if (window->Kind_ == EWindowKind::Popup) {
        RemovePopupFromStack(window);
        PopupStack_.push_back(window);
    } else if (window->Kind_ == EWindowKind::Modal) {
        ModalWindow_ = window;
    }

    BringToFront(window);
    SetActiveWindowInternal(window);
}

void ImWindowManager::SetMainWindowInternal(const Ptr& window)
{
    MainWindow_ = window;
    if (MainWindow_) {
        BringToFront(MainWindow_);
    }
}

void ImWindowManager::SetActiveWindowInternal(const Ptr& window)
{
    if (ActiveWindow_ == window) {
        return;
    }

    if (ActiveWindow_) {
        ActiveWindow_->bIsActive_ = false;
    }

    ActiveWindow_ = window;
    if (ActiveWindow_) {
        ActiveWindow_->bIsActive_ = true;
    }
}

void ImWindowManager::SyncViewportFilledWindows(const FGeometry& viewportGeometry)
{
    for (const Ptr& window : Windows_) {
        if (!window || !window->IsOpen() || !window->UsesViewportFill()) {
            continue;
        }

        window->SetPosition(viewportGeometry.Position);
        window->SetSize(viewportGeometry.Size);
    }

    for (const Ptr& window : Windows_) {
        if (!window || !window->IsOpen() || window->Kind_ == EWindowKind::Normal) {
            continue;
        }

        ClampPopupToViewport(window, viewportGeometry);
    }
}

void ImWindowManager::ClampPopupToViewport(const Ptr& window, const FGeometry& viewportGeometry)
{
    if (!window || !window->IsOpen()) {
        return;
    }

    FVector2 position = window->GetPosition();
    const FVector2 size = window->GetSize();
    const float minX = viewportGeometry.Position.X;
    const float minY = viewportGeometry.Position.Y;
    const float maxX = viewportGeometry.Position.X + viewportGeometry.Size.X;
    const float maxY = viewportGeometry.Position.Y + viewportGeometry.Size.Y;

    position.X = (std::max)(minX, (std::min)(position.X, maxX - size.X));
    position.Y = (std::max)(minY, (std::min)(position.Y, maxY - size.Y));

    window->SetPosition(position);
}

ImWindowManager::Ptr ImWindowManager::HitTestWindow(const FVector2& position) const
{
    const Ptr modalWindow = GetTopmostModalWindow();
    if (modalWindow && modalWindow->IsOpen()) {
        return modalWindow->ContainsPoint(position) ? modalWindow : nullptr;
    }

    for (auto it = Windows_.rbegin(); it != Windows_.rend(); ++it) {
        const Ptr& window = *it;
        if (!window || !window->IsOpen()) {
            continue;
        }

        if (window->ContainsPoint(position)) {
            return window;
        }
    }

    return nullptr;
}

ImWindowManager::Ptr ImWindowManager::HitTestContentWindow(const FVector2& position) const
{
    const Ptr window = HitTestWindow(position);
    if (window && window->IsPointInContent(position)) {
        return window;
    }

    return nullptr;
}

ImWindowManager::Ptr ImWindowManager::FindOwningWindow(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget) {
        return nullptr;
    }

    for (const Ptr& window : Windows_) {
        if (!window || !window->IsOpen() || !window->RootWidget_) {
            continue;
        }

        std::vector<std::shared_ptr<ImWidget>> path;
        window->RootWidget_->BuildHitTestPath(widget->GetGeometry().GetCenter(), path);
        for (const std::shared_ptr<ImWidget>& candidate : path) {
            if (candidate == widget) {
                return window;
            }
        }

        std::shared_ptr<ImWidget> current = widget;
        while (current) {
            if (current == window->RootWidget_) {
                return window;
            }
            current = current->GetParent();
        }
    }

    return nullptr;
}

ImWindowManager::Ptr ImWindowManager::GetTopmostModalWindow() const
{
    return ModalWindow_ && ModalWindow_->IsOpen() ? ModalWindow_ : nullptr;
}

ImWindowManager::Ptr ImWindowManager::GetTopmostPopupWindow() const
{
    for (auto it = PopupStack_.rbegin(); it != PopupStack_.rend(); ++it) {
        if (*it && (*it)->IsOpen()) {
            return *it;
        }
    }

    return nullptr;
}

bool ImWindowManager::IsBlockedByModal(const Ptr& window) const
{
    const Ptr modalWindow = GetTopmostModalWindow();
    return modalWindow && modalWindow != window;
}

bool ImWindowManager::IsPopupChainHit(const FVector2& position) const
{
    for (auto it = PopupStack_.rbegin(); it != PopupStack_.rend(); ++it) {
        if (*it && (*it)->IsOpen() && (*it)->ContainsPoint(position)) {
            return true;
        }
    }

    return false;
}

void ImWindowManager::CloseTopPopupChain()
{
    Ptr topPopup = GetTopmostPopupWindow();
    if (!topPopup) {
        return;
    }

    Ptr rootPopup = topPopup;
    while (Ptr parent = rootPopup->ParentWindow_.lock()) {
        if (parent->Kind_ != EWindowKind::Popup) {
            break;
        }
        rootPopup = parent;
    }

    CloseWindowRecursive(rootPopup);
}

void ImWindowManager::RemovePopupFromStack(const Ptr& window)
{
    PopupStack_.erase(
        std::remove(PopupStack_.begin(), PopupStack_.end(), window),
        PopupStack_.end());
}

void ImWindowManager::CloseWindowRecursive(const Ptr& window)
{
    if (!window || !window->IsOpen()) {
        return;
    }

    window->CloseChildrenRecursive();

    if (Ptr parent = window->ParentWindow_.lock()) {
        parent->RemoveChildWindow(window);
    }

    RemovePopupFromStack(window);
    if (ModalWindow_ == window) {
        ModalWindow_.reset();
    }

    const bool bWasActive = ActiveWindow_ == window;
    window->CloseInternal();

    if (bWasActive) {
        ActiveWindow_.reset();
        for (auto it = Windows_.rbegin(); it != Windows_.rend(); ++it) {
            if (*it && (*it)->IsOpen()) {
                SetActiveWindowInternal(*it);
                break;
            }
        }
    }
}

void ImWindowManager::PruneExpiredChildren(ImWindow& window) const
{
    window.ChildWindows_.erase(
        std::remove_if(
            window.ChildWindows_.begin(),
            window.ChildWindows_.end(),
            [](const std::weak_ptr<ImWindow>& child) {
                return child.expired();
            }),
        window.ChildWindows_.end());
}

std::size_t ImWindowManager::FindInsertIndexForFront(EWindowKind kind) const
{
    if (kind == EWindowKind::Modal) {
        return Windows_.size();
    }

    if (kind == EWindowKind::Popup) {
        for (std::size_t index = 0; index < Windows_.size(); ++index) {
            if (Windows_[index] && Windows_[index]->Kind_ == EWindowKind::Modal) {
                return index;
            }
        }
        return Windows_.size();
    }

    for (std::size_t index = 0; index < Windows_.size(); ++index) {
        if (Windows_[index] &&
            (Windows_[index]->Kind_ == EWindowKind::Popup || Windows_[index]->Kind_ == EWindowKind::Modal)) {
            return index;
        }
    }

    return Windows_.size();
}

std::size_t ImWindowManager::FindInsertIndexForViewportFillNormal(const Ptr& window) const
{
    std::size_t index = 0;
    for (; index < Windows_.size(); ++index) {
        const Ptr& candidate = Windows_[index];
        if (!candidate) {
            continue;
        }

        if (candidate->Kind_ != EWindowKind::Normal) {
            break;
        }

        if (!candidate->UsesViewportFill()) {
            break;
        }
    }

    return index;
}

} // namespace ImWidgetV4
