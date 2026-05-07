#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <nlohmann/json.hpp>
#include <memory>

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

    FPropertiesChangedEvent OnPropertiesChanged;
    FPropertyValueCommittedEvent OnPropertyValueCommitted;

protected:
    virtual Ptr RebuildWidget() override;

private:
    std::shared_ptr<ImWidgetV4::ImWidget> BuildEmptyState() const;
    void BuildCommonSection(
        ImWidgetV4::ImOutlineView& outlineView,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;
    void BuildObjectSection(
        ImWidgetV4::ImOutlineView& outlineView,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& object,
        const std::string& title) const;
    void BuildPropertyItems(
        ImWidgetV4::ImOutlineView& outlineView,
        ImWidgetV4::ImOutlineItem* parentItem,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& object) const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildPropertyEditorRow(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::ReflectableObject::ROPProperty& property,
        const nlohmann::ordered_json& objectJson) const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildStructPropertyEditorRow(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::ReflectableObject::ROPProperty& property,
        const std::string& propertyClassName,
        const std::string& propertyName,
        const nlohmann::ordered_json& propertyValueJson) const;
    std::string DescribePropertyValue(
        const ImWidgetV4::ReflectableObject::ROPProperty& property,
        const nlohmann::ordered_json& objectJson) const;
    std::shared_ptr<ImWidgetV4::ReflectableObject> ResolveNestedObject(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::ReflectableObject::ROPProperty& property) const;

    std::shared_ptr<ImWidgetV4::ReflectableObject> m_Target;
    std::shared_ptr<ImWidgetV4::ImSlot> m_SlotTarget;
};

} // namespace ImWidgetV4Editor
