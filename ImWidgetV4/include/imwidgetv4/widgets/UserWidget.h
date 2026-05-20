#pragma once

#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace ImWidgetV4 {

class ImUserWidget : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImUserWidget"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImUserWidget();
    virtual ~ImUserWidget() = default;

    void SetRootWidget(const Ptr& rootWidget);
    std::shared_ptr<ImWidget> GetRootWidget() const;

    void Rebuild();
    bool IsRootBuilt() const { return m_bRootBuilt; }

    std::shared_ptr<ImWidget> FindWidgetByName(const std::string& name) const;

    template<typename T>
    std::shared_ptr<T> FindWidgetAs(const std::string& name) const
    {
        return std::dynamic_pointer_cast<T>(FindWidgetByName(name));
    }

    void Paint(const FPaintContext& paintContext) override;
    FVector2 GetMinSize() const override;
    bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    void AddChild(const Ptr& child) override;
    void ClearChildren() override;

protected:
    virtual Ptr RebuildWidget();
    virtual void OnRootWidgetRebuilt();

private:
    void EnsureRootBuilt();
    void ResetRootWidget(const Ptr& newRootWidget);
    void SyncRootGeometry();
    void RebuildNamedWidgetCache();
    void CleanupInteractionStateForSubtree(const Ptr& subtreeRoot);

    Ptr m_RootWidget;
    Ptr m_ConfiguredRootWidget;
    bool m_bRootBuilt = false;
    std::unordered_map<std::string, std::weak_ptr<ImWidget>> m_NamedWidgets;
};

} // namespace ImWidgetV4
