#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/widgets/UserWidget.h>

namespace ImWidgetV4 {

class ImDesignerSurface : public ImUserWidget {
    DECLARE_OBJECT_WITH_PARENT(ImDesignerSurface, ImUserWidget)
    END_DECLARE_OBJECT()

public:
    using FSelectionChangedEvent = TMulticastDelegate<ImDesignerSurface&, std::shared_ptr<ImWidget>>;
    using FDropEvent = TMulticastDelegate<
        ImDesignerSurface&,
        const std::shared_ptr<FDragDropOperation>&,
        const FVector2&,
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
    FDropEvent OnDropReceived;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual FReply OnDragEvent(const FDragDropEvent& event) override;

private:
    std::shared_ptr<ImWidget> ResolveSelectableWidgetAt(const FVector2& position) const;
    bool ContainsWidgetInContent(const std::shared_ptr<ImWidget>& widget) const;
    bool ContainsWidgetRecursive(const std::shared_ptr<ImWidget>& root, const std::shared_ptr<ImWidget>& target) const;
    void PaintSelectionOverlay(const FPaintContext& paintContext) const;

    std::shared_ptr<ImWidget> m_SelectedWidget;
    FColor m_SelectionBorderColor = FColor::FromBytes(103, 177, 255);
    FColor m_SelectionFillColor = FColor::FromBytes(103, 177, 255, 36);
    float m_SelectionBorderThickness = 2.0f;
};

} // namespace ImWidgetV4
