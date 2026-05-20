#pragma once

#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

class ImApplicationBackend;

struct FTitleBarStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FTitleBarStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(28, 33, 41);
    FColor BorderColor = FColor::FromBytes(16, 19, 24);
    float BorderThickness = 1.0f;
    FMargin Padding = FMargin(0.0f);
    float ItemSpacing = 0.0f;
    float Height = 34.0f;
    float DragRegionMinWidth = 34.0f;
    float SystemButtonSize = 34.0f;
    float SystemButtonSpacing = 0.0f;
    FColor SystemButtonGlyphColor = FColor::FromBytes(244, 247, 251);
    FColor HoveredSystemButtonColor = FColor::FromBytes(255, 255, 255, 24);
    FColor PressedSystemButtonColor = FColor::FromBytes(255, 255, 255, 40);
    FColor CloseButtonHoveredColor = FColor::FromBytes(212, 58, 76, 224);
    FColor CloseButtonPressedColor = FColor::FromBytes(188, 46, 66, 240);
    FVector2 MinDesiredSize {240.0f, 34.0f};
};

class ImTitleBar : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTitleBar"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImTitleBar();
    virtual ~ImTitleBar() = default;

    void AddLeadingItem(const std::shared_ptr<ImWidget>& widget);
    void AddTrailingItem(const std::shared_ptr<ImWidget>& widget);
    void ClearLeadingItems();
    void ClearTrailingItems();
    std::size_t GetLeadingItemCount() const { return LeadingItems_.size(); }
    std::size_t GetTrailingItemCount() const { return TrailingItems_.size(); }
    std::shared_ptr<ImWidget> GetLeadingItemAt(std::size_t index) const;
    std::shared_ptr<ImWidget> GetTrailingItemAt(std::size_t index) const;
    bool RemoveLeadingItem(const std::shared_ptr<ImWidget>& widget);
    bool RemoveTrailingItem(const std::shared_ptr<ImWidget>& widget);
    bool InsertLeadingItem(std::size_t index, const std::shared_ptr<ImWidget>& widget);
    bool InsertTrailingItem(std::size_t index, const std::shared_ptr<ImWidget>& widget);

    void SetShowSystemButtons(bool value);
    void SetShowMinimizeButton(bool value);
    void SetShowMaximizeButton(bool value);
    void SetShowCloseButton(bool value);
    bool GetShowSystemButtons() const { return bShowSystemButtons_; }
    bool GetShowMinimizeButton() const { return bShowMinimizeButton_; }
    bool GetShowMaximizeButton() const { return bShowMaximizeButton_; }
    bool GetShowCloseButton() const { return bShowCloseButton_; }
    void SetHostWindowActionsEnabled(bool bEnabled);
    bool AreHostWindowActionsEnabled() const { return bHostWindowActionsEnabled_; }
    void SetDragRegionMinWidth(float width);
    void SetStyle(const FTitleBarStyle& style);
    const FTitleBarStyle& GetStyle() const { return GetEffectiveStyle(); }

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnPreviewInputEvent(const FInputEvent& event) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void FromJson(const json& j) override;

private:
    enum class ESystemButton : std::uint8_t {
        None,
        Minimize,
        Maximize,
        Close
    };

    struct FChildLayout {
        Ptr Widget;
        FGeometry Geometry;
    };

    void Relayout() const;
    void DetachItems(std::vector<Ptr>& items);
    void AttachItem(std::vector<Ptr>& destination, const Ptr& widget);
    void SyncChildGeometries() const;
    void ClearHoveredState();
    void UpdateHoveredState(const FVector2& mousePosition);
    void UpdateToolTipForHoveredState();
    void PaintChildren(const FPaintContext& paintContext) const;
    void PaintSystemButtons(const FPaintContext& paintContext) const;
    void PaintSystemButton(const FPaintContext& paintContext, ESystemButton button, const FGeometry& geometry) const;
    void DrawSystemButtonGlyph(const FPaintContext& paintContext, ESystemButton button, const FGeometry& geometry) const;
    const FTitleBarStyle& GetEffectiveStyle() const;
    bool HasSystemButtons() const;
    bool IsSystemButtonVisible(ESystemButton button) const;
    float GetResolvedDragRegionMinWidth() const;
    ESystemButton HitTestSystemButton(const FVector2& position) const;
    bool HitTestsChildWidgets(const FVector2& position, std::vector<Ptr>* outPath = nullptr) const;
    bool IsPointInHostDragArea(const FVector2& position) const;
    ImApplicationBackend* GetBackend() const;
    bool HandleSystemButtonClick(ESystemButton button);
    void MarkLayoutDirty();

    FTitleBarStyle Style_;
    mutable FTitleBarStyle ResolvedThemeStyle_;
    std::vector<Ptr> LeadingItems_;
    std::vector<Ptr> TrailingItems_;
    mutable std::vector<FChildLayout> LeadingLayouts_;
    mutable std::vector<FChildLayout> TrailingLayouts_;
    mutable FGeometry DragRegionGeometry_;
    mutable FGeometry MinimizeButtonGeometry_;
    mutable FGeometry MaximizeButtonGeometry_;
    mutable FGeometry CloseButtonGeometry_;
    mutable FGeometry LastLayoutGeometry_;
    mutable bool bHasLastLayoutGeometry_ = false;
    mutable bool bLayoutDirty_ = true;
    bool bShowSystemButtons_ = true;
    bool bShowMinimizeButton_ = true;
    bool bShowMaximizeButton_ = true;
    bool bShowCloseButton_ = true;
    bool bHasExplicitStyle_ = false;
    bool bHostWindowActionsEnabled_ = true;
    float ReflectedDragRegionMinWidth_ = -1.0f;
    ESystemButton HoveredSystemButton_ = ESystemButton::None;
    ESystemButton PressedSystemButton_ = ESystemButton::None;
};

} // namespace ImWidgetV4
