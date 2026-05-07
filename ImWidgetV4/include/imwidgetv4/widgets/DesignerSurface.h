#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/widgets/UserWidget.h>

namespace ImWidgetV4 {

class ImCanvasPanel;
class ImCanvasPanelSlot;

enum class EDesignerTransformHandle : std::uint8_t {
    None,
    Move,
    ResizeBottomRight
};

class ImDesignerSurface : public ImUserWidget {
    DECLARE_OBJECT_WITH_PARENT(ImDesignerSurface, ImUserWidget)
    END_DECLARE_OBJECT()

public:
    using FSelectionChangedEvent = TMulticastDelegate<ImDesignerSurface&, std::shared_ptr<ImWidget>>;
    using FDeleteRequestedEvent = TMulticastDelegate<ImDesignerSurface&>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImDesignerSurface&, std::shared_ptr<ImWidget>, FVector2>;
    using FTransformStartedEvent = TMulticastDelegate<
        ImDesignerSurface&,
        std::shared_ptr<ImWidget>,
        EDesignerTransformHandle>;
    using FTransformFinishedEvent = TMulticastDelegate<
        ImDesignerSurface&,
        std::shared_ptr<ImWidget>,
        EDesignerTransformHandle,
        bool>;
    using FDropEvent = TMulticastDelegate<
        ImDesignerSurface&,
        const std::shared_ptr<FDragDropOperation>&,
        const FVector2&,
        bool&>;
    using FDropTestEvent = TMulticastDelegate<
        ImDesignerSurface&,
        const std::shared_ptr<FDragDropOperation>&,
        const FVector2&,
        std::shared_ptr<ImWidget>&,
        bool&>;

    ImDesignerSurface();
    virtual ~ImDesignerSurface() = default;

    void SetContentRoot(const Ptr& rootWidget);
    std::shared_ptr<ImWidget> GetContentRoot() const;

    void SetSelectedWidget(const std::shared_ptr<ImWidget>& widget);
    std::shared_ptr<ImWidget> GetSelectedWidget() const { return m_SelectedWidget; }
    void ClearSelection();

    void SetSelectionBorderColor(const FColor& color);
    const FColor& GetSelectionBorderColor() const { return m_SelectionBorderColor; }

    void SetSelectionFillColor(const FColor& color);
    const FColor& GetSelectionFillColor() const { return m_SelectionFillColor; }

    void SetSelectionBorderThickness(float thickness);
    float GetSelectionBorderThickness() const { return m_SelectionBorderThickness; }

    FSelectionChangedEvent OnSelectionChanged;
    FDeleteRequestedEvent OnDeleteRequested;
    FContextMenuRequestedEvent OnContextMenuRequested;
    FTransformStartedEvent OnTransformStarted;
    FTransformFinishedEvent OnTransformFinished;
    FDropTestEvent OnDropTest;
    FDropEvent OnDropReceived;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FReply OnPreviewInputEvent(const FInputEvent& event) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual FReply OnDragEvent(const FDragDropEvent& event) override;

private:
    std::shared_ptr<ImWidget> ResolveSelectableWidgetAt(const FVector2& position) const;
    bool ContainsWidgetInContent(const std::shared_ptr<ImWidget>& widget) const;
    bool ContainsWidgetRecursive(const std::shared_ptr<ImWidget>& root, const std::shared_ptr<ImWidget>& target) const;
    void PaintDropPreviewOverlay(const FPaintContext& paintContext) const;
    void PaintSelectionOverlay(const FPaintContext& paintContext) const;
    bool ResolveCanvasSelectionContext(std::shared_ptr<ImCanvasPanel>& outCanvas, ImCanvasPanelSlot*& outSlot) const;
    EDesignerTransformHandle HitTestTransformHandle(const FVector2& position) const;
    void UpdateHoveredTransformHandle(const FVector2& position);
    bool BeginTransform(EDesignerTransformHandle handle, const FVector2& mousePosition);
    bool UpdateTransform(const FVector2& mousePosition);
    void EndTransform();
    void CancelTransform();

    std::shared_ptr<ImWidget> m_SelectedWidget;
    FColor m_SelectionBorderColor = FColor::FromBytes(103, 177, 255);
    FColor m_SelectionFillColor = FColor::FromBytes(103, 177, 255, 36);
    float m_SelectionBorderThickness = 2.0f;
    float m_TransformHandleSize = 10.0f;
    std::shared_ptr<ImWidget> m_DropPreviewWidget;
    bool m_bDropPreviewAccepted = false;
    FColor m_DropPreviewBorderColor = FColor::FromBytes(92, 214, 141);
    FColor m_DropPreviewFillColor = FColor::FromBytes(92, 214, 141, 34);
    EDesignerTransformHandle m_HoveredTransformHandle = EDesignerTransformHandle::None;
    EDesignerTransformHandle m_ActiveTransformHandle = EDesignerTransformHandle::None;
    bool m_bTransformChanged = false;
    FVector2 m_TransformStartMousePosition {0.0f, 0.0f};
    FVector2 m_TransformStartRelativePosition {0.0f, 0.0f};
    FVector2 m_TransformStartRelativeSize {0.0f, 0.0f};
    bool m_bTransformStartAutoSize = true;
};

} // namespace ImWidgetV4
