#pragma once

#include <imwidgetv4/core/Window.h>
#include <memory>
#include <vector>

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace ImWidgetV4 {

class ImApplication;

class ImWindowManager {
public:
    using Ptr = std::shared_ptr<ImWindow>;

    ImWindowManager();

    Ptr CreateWindow(const FWindowOptions& options);
    Ptr CreatePopup(const FPopupOptions& options);
    Ptr CreateModal(const FPopupOptions& options);
    void CloseWindow(const Ptr& window);
    void BringToFront(const Ptr& window);

    Ptr GetActiveWindow() const { return ActiveWindow_; }
    Ptr GetMainWindow() const { return MainWindow_; }
    std::vector<Ptr> GetOpenWindows() const;
    Ptr FindWindowForWidget(const std::shared_ptr<ImWidget>& widget) const;

private:
    friend class ImApplication;
    friend class ImWindow;

    void SetOwnerApplication(ImApplication* application);
    void OpenWindowInternal(const Ptr& window);
    void SetMainWindowInternal(const Ptr& window);
    void SetActiveWindowInternal(const Ptr& window);
    void SyncViewportFilledWindows(const FGeometry& viewportGeometry);
    void ClampPopupToViewport(const Ptr& window, const FGeometry& viewportGeometry);
    Ptr HitTestWindow(const FVector2& position) const;
    Ptr HitTestContentWindow(const FVector2& position) const;
    Ptr FindOwningWindow(const std::shared_ptr<ImWidget>& widget) const;
    Ptr GetTopmostModalWindow() const;
    Ptr GetTopmostPopupWindow() const;
    bool IsBlockedByModal(const Ptr& window) const;
    bool IsPopupChainHit(const FVector2& position) const;
    void CloseTopPopupChain();
    void RemovePopupFromStack(const Ptr& window);
    void CloseWindowRecursive(const Ptr& window);
    void PruneExpiredChildren(ImWindow& window) const;
    std::size_t FindInsertIndexForFront(EWindowKind kind) const;
    std::size_t FindInsertIndexForViewportFillNormal(const Ptr& window) const;

    std::vector<Ptr> Windows_;
    std::vector<Ptr> PopupStack_;
    Ptr MainWindow_;
    Ptr ActiveWindow_;
    Ptr ModalWindow_;
    ImApplication* OwnerApplication_ = nullptr;
};

} // namespace ImWidgetV4
