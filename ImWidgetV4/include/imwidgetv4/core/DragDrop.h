#pragma once

#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/input/Input.h>
#include <cstdint>
#include <memory>

namespace ImWidgetV4 {

class ImWidget;

enum class EDragDropEventType : std::uint8_t {
    DragStart,
    DragUpdate,
    DragEnd,
    DragEnter,
    DragOver,
    DragLeave,
    Drop
};

class FDragDropPayload : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FDragDropPayload"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    virtual ~FDragDropPayload() = default;
};

struct FDragDropOperation {
    std::shared_ptr<FDragDropPayload> Payload;
    std::shared_ptr<ImWidget> PreviewWidget;
    FVector2 PreviewOffset {12.0f, 12.0f};
    EMouseButton TriggerButton = EMouseButton::Left;

    bool IsValid() const
    {
        return Payload != nullptr || PreviewWidget != nullptr;
    }
};

struct FDragDetectEvent {
    std::shared_ptr<ImWidget> SourceWidget;
    EMouseButton TriggerButton = EMouseButton::Left;
    FVector2 PressPosition {0.0f, 0.0f};
    FVector2 CurrentPosition {0.0f, 0.0f};
    FInputModifiers Modifiers;
    double Timestamp = 0.0;
};

struct FDragDropEvent {
    EDragDropEventType Type = EDragDropEventType::DragStart;
    std::shared_ptr<FDragDropOperation> Operation;
    std::shared_ptr<ImWidget> SourceWidget;
    std::shared_ptr<ImWidget> TargetWidget;
    FVector2 PressPosition {0.0f, 0.0f};
    FVector2 CurrentPosition {0.0f, 0.0f};
    FInputModifiers Modifiers;
    double Timestamp = 0.0;
};

} // namespace ImWidgetV4
