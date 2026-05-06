#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <nlohmann/json.hpp>
#include <memory>

namespace ImWidgetV4Editor {

class ReflectionDetailsView : public ImWidgetV4::ImUserWidget {
public:
    using FPropertiesChangedEvent = ImWidgetV4::TMulticastDelegate<ReflectionDetailsView&>;

    ReflectionDetailsView();
    virtual ~ReflectionDetailsView() = default;

    void SetTarget(const std::shared_ptr<ImWidgetV4::ReflectableObject>& target);
    std::shared_ptr<ImWidgetV4::ReflectableObject> GetTarget() const { return m_Target; }
    void SetSlotTarget(const std::shared_ptr<ImWidgetV4::ImSlot>& slotTarget);
    std::shared_ptr<ImWidgetV4::ImSlot> GetSlotTarget() const { return m_SlotTarget; }

    FPropertiesChangedEvent OnPropertiesChanged;

protected:
    virtual Ptr RebuildWidget() override;

private:
    std::shared_ptr<ImWidgetV4::ImWidget> BuildEmptyState() const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildDetailsForObject(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& object,
        const std::string& title) const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildPropertyRows(
        ImWidgetV4::ReflectableObject& object,
        int indentLevel) const;
    std::shared_ptr<ImWidgetV4::ImWidget> BuildPropertyEditorRow(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const ImWidgetV4::ReflectableObject::ROPProperty& property,
        const nlohmann::ordered_json& objectJson,
        int indentLevel) const;
    std::string DescribePropertyValue(
        const ImWidgetV4::ReflectableObject::ROPProperty& property,
        const nlohmann::ordered_json& objectJson) const;
    std::shared_ptr<ImWidgetV4::ReflectableObject> ResolveNestedObject(
        const ImWidgetV4::ReflectableObject::ROPProperty& property) const;

    std::shared_ptr<ImWidgetV4::ReflectableObject> m_Target;
    std::shared_ptr<ImWidgetV4::ImSlot> m_SlotTarget;
};

} // namespace ImWidgetV4Editor
