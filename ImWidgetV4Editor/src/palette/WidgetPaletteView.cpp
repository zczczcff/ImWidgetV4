#include "WidgetPaletteView.h"

#include <imwidgetv4/widgets/TextBlock.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImTextBlock> MakePreviewText(const std::string& text)
{
    auto preview = std::make_shared<ImTextBlock>();
    preview->SetText(text);
    preview->SetFontSize(14.0f);
    preview->SetTextColor(FColor::White);
    preview->SetHitTestVisible(false);
    return preview;
}

} // namespace

WidgetPaletteItemButton::WidgetPaletteItemButton(const FWidgetPaletteEntry& entry)
    : m_Entry(entry)
{
    SetText(entry.Label);
}

FReply WidgetPaletteItemButton::OnInputEvent(const FInputEvent& event)
{
    FReply reply = ImButton::OnInputEvent(event);
    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left) {
        return FReply::Handled().DetectDrag(shared_from_this(), EMouseButton::Left);
    }
    return reply;
}

std::shared_ptr<FDragDropOperation> WidgetPaletteItemButton::OnDragDetected(const FDragDetectEvent&)
{
    auto operation = std::make_shared<FDragDropOperation>();
    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = m_Entry.TypeName;
    payload->Label = m_Entry.Label;
    operation->Payload = payload;
    operation->PreviewWidget = MakePreviewText(m_Entry.Label);
    operation->PreviewOffset = FVector2(14.0f, 16.0f);
    return operation;
}

std::shared_ptr<ImVerticalBox> BuildWidgetPaletteView()
{
    auto panel = std::make_shared<ImVerticalBox>();
    panel->SetSpacing(8.0f);

    for (const auto& entry : BuildDefaultWidgetPaletteEntries()) {
        auto button = std::make_shared<WidgetPaletteItemButton>(entry);
        panel->AddChild(button, FMargin(4.0f, 2.0f, 4.0f, 2.0f));
    }

    return panel;
}

} // namespace ImWidgetV4Editor
