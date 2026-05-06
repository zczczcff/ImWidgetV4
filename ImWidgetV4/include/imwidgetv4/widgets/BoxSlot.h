#pragma once

#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

class ImBoxSlot : public ImPaddingSlot {
    DECLARE_OBJECT_WITH_PARENT(ImBoxSlot, ImPaddingSlot)
    registrar
        .RegisterProperty(PropertyType::Float, "FillCoefficient", &ImBoxSlot::m_FillCoefficient, "Fill coefficient for proportional layout");
    END_DECLARE_OBJECT()

public:
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
