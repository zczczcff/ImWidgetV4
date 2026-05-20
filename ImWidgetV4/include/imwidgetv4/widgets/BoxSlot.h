#pragma once

#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

class ImBoxSlot : public ImPaddingSlot {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImBoxSlot"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImBoxSlot();
    virtual ~ImBoxSlot() = default;

    void SetFillCoefficient(float coefficient) { m_FillCoefficient = coefficient; }
    float GetFillCoefficient() const { return m_FillCoefficient; }

    void SetAutoFill(bool bAutoFill) { m_FillCoefficient = bAutoFill ? 1.0f : 0.0f; }
    bool IsAutoFill() const { return m_FillCoefficient > 0.0f; }

private:
    float m_FillCoefficient = 0.0f;
};

} // namespace ImWidgetV4
