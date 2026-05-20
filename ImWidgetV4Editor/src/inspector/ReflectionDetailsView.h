#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace ImWidgetV4 {
class ImOutlineItem;
class ImOutlineView;
}

namespace ImWidgetV4Editor {

class ReflectionDetailsView : public ImWidgetV4::ImUserWidget {
public:
    using FPropertiesChangedEvent = ImWidgetV4::TMulticastDelegate<ReflectionDetailsView&>;
    using FPropertyValueCommittedEvent = ImWidgetV4::TMulticastDelegate<
        ReflectionDetailsView&,
        std::shared_ptr<ImWidgetV4::ReflectableObject>,
        std::string,
        std::string,
        nlohmann::ordered_json>;

    ReflectionDetailsView();
    virtual ~ReflectionDetailsView() = default;

    void SetTargets(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& target,
        const std::shared_ptr<ImWidgetV4::ImSlot>& slotTarget);
    void SetTarget(const std::shared_ptr<ImWidgetV4::ReflectableObject>& target);
    std::shared_ptr<ImWidgetV4::ReflectableObject> GetTarget() const { return m_Target; }
    void SetSlotTarget(const std::shared_ptr<ImWidgetV4::ImSlot>& slotTarget);
    std::shared_ptr<ImWidgetV4::ImSlot> GetSlotTarget() const { return m_SlotTarget; }
    void RebuildPreservingViewState();
    bool SetSectionExpanded(const std::string& path, bool expanded);
    bool IsSectionExpanded(const std::string& path) const;

    FPropertiesChangedEvent OnPropertiesChanged;
    FPropertyValueCommittedEvent OnPropertyValueCommitted;

protected:
    virtual Ptr RebuildWidget() override;

private:
    std::shared_ptr<ImWidgetV4::ImWidget> BuildEmptyState() const;
    void BuildCommonSection(
        ImWidgetV4::ImOutlineView& outlineView,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    void BuildObjectSection(
        ImWidgetV4::ImOutlineView& outlineView,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& object,
        const std::string& title);
    void BuildPropertyItems(
        ImWidgetV4::ImOutlineView& outlineView,
        ImWidgetV4::ImOutlineItem* parentItem,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& object,
        const std::string& parentPath);
    std::shared_ptr<ImWidgetV4::ImWidget> BuildPropertyEditorRow(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::Reflection::FPropertyDesc& property,
        const nlohmann::ordered_json& objectJson) const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildStructPropertyEditorRow(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::Reflection::FPropertyDesc& property,
        const std::string& propertyClassName,
        const std::string& propertyName,
        const nlohmann::ordered_json& propertyValueJson) const;
    std::string DescribePropertyValue(
        const ImWidgetV4::Reflection::FPropertyDesc& property,
        const nlohmann::ordered_json& objectJson) const;
    std::shared_ptr<ImWidgetV4::ReflectableObject> ResolveNestedObject(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::Reflection::FPropertyDesc& property) const;
    std::shared_ptr<ImWidgetV4::ImOutlineView> GetCurrentOutlineView() const;
    std::string BuildCurrentStateKey() const;
    void CaptureCurrentViewState();
    void RestoreCurrentViewState();
    bool ResolveInitialExpandedState(const std::string& path, bool bDefaultExpanded) const;
    ImWidgetV4::ImOutlineItem* AddTrackedGroupItem(
        ImWidgetV4::ImOutlineView& outlineView,
        ImWidgetV4::ImOutlineItem* parentItem,
        const std::string& path,
        const std::string& title,
        bool bDefaultExpanded);

    std::shared_ptr<ImWidgetV4::ReflectableObject> m_Target;
    std::shared_ptr<ImWidgetV4::ImSlot> m_SlotTarget;
    struct FInspectorViewState {
        float ScrollOffset = 0.0f;
        std::unordered_map<std::string, bool> ExpandedByPath;
    };
    std::unordered_map<std::string, FInspectorViewState> m_ViewStatesByKey;
    std::unordered_map<const ImWidgetV4::ImOutlineItem*, std::string> m_CurrentItemPaths;
    std::unordered_map<std::string, ImWidgetV4::ImOutlineItem*> m_CurrentPathItems;
    std::string m_CurrentStateKey;
};

} // namespace ImWidgetV4Editor
