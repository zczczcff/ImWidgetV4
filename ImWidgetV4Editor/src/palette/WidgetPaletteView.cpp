#include "WidgetPaletteView.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImTextBlock> MakePreviewText(const std::string& text)
{
    auto preview = std::make_shared<ImTextBlock>();
    preview->SetText(text);
    preview->SetFontSize(14.0f);
    preview->SetTextColor(FColor::Black);
    preview->SetHitTestVisible(false);
    return preview;
}

std::string ResolvePaletteLabel(const FWidgetPaletteEntry& entry)
{
    const std::string resolved = entry.LabelText.Resolve();
    return resolved.empty() ? entry.Label : resolved;
}

std::shared_ptr<ImWidget> MakePreviewContent(const FImageBrush& iconBrush, const std::string& text)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);
    row->SetHitTestVisible(false);

    if (iconBrush.IsValid()) {
        auto image = std::make_shared<ImImage>();
        image->SetBrush(iconBrush);
        image->SetDesiredSize(FVector2(32.0f, 32.0f));
        image->SetBackgroundColor(FColor::Transparent);
        image->SetCornerRadius(0.0f);
        image->SetHitTestVisible(false);
        row->AddChild(image);
    }

    row->AddChild(MakePreviewText(text));
    return row;
}

} // namespace

WidgetPaletteItemButton::WidgetPaletteItemButton(const FWidgetPaletteEntry& entry)
    : m_Entry(entry)
{
    EnsureVisualContent();
}

void WidgetPaletteItemButton::Paint(const FPaintContext& paintContext)
{
    EnsureVisualContent();
    ImButton::Paint(paintContext);
}

void WidgetPaletteItemButton::EnsureVisualContent()
{
    if (m_bVisualContentInitialized) {
        if (m_IconImage != nullptr && GetApplication() != nullptr) {
            m_IconImage->SetBrush(GetApplication()->GetCoreIconBrush(m_Entry.Icon, FColor::Black));
        }
        return;
    }

    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);
    row->SetHitTestVisible(false);

    m_IconImage = std::make_shared<ImImage>();
    m_IconImage->SetDesiredSize(FVector2(32.0f, 32.0f));
    m_IconImage->SetBackgroundColor(FColor::Transparent);
    m_IconImage->SetCornerRadius(0.0f);
    m_IconImage->SetHitTestVisible(false);
    row->AddChild(m_IconImage);

    m_LabelText = std::make_shared<ImTextBlock>();
    m_LabelText->SetText(m_Entry.LabelText);
    m_LabelText->SetFontSize(14.0f);
    m_LabelText->SetWrapText(false);
    m_LabelText->SetTextColor(FColor::Black);
    m_LabelText->SetHitTestVisible(false);
    row->AddChild(m_LabelText);

    SetContent(row);
    m_bVisualContentInitialized = true;
    EnsureVisualContent();
}

FReply WidgetPaletteItemButton::OnInputEvent(const FInputEvent& event)
{
    EnsureVisualContent();
    FReply reply = ImButton::OnInputEvent(event);
    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left) {
        return FReply::Handled().DetectDrag(shared_from_this(), EMouseButton::Left);
    }
    return reply;
}

FReply WidgetPaletteItemButton::OnDragEvent(const FDragDropEvent& event)
{
    if (event.SourceWidget.get() == this &&
        (event.Type == EDragDropEventType::DragStart || event.Type == EDragDropEventType::DragEnd)) {
        SetPressed(false);

        if (GetApplication() != nullptr && GetApplication()->GetMouseCapture().get() == this) {
            GetApplication()->ReleaseMouseCapture();
        }
    }

    return ImButton::OnDragEvent(event);
}

std::shared_ptr<FDragDropOperation> WidgetPaletteItemButton::OnDragDetected(const FDragDetectEvent&)
{
    EnsureVisualContent();
    auto operation = std::make_shared<FDragDropOperation>();
    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = m_Entry.TypeName;
    payload->Label = ResolvePaletteLabel(m_Entry);
    operation->Payload = payload;
    const FImageBrush iconBrush = GetApplication() != nullptr
        ? GetApplication()->GetCoreIconBrush(m_Entry.Icon, FColor::Black)
        : FImageBrush();
    operation->PreviewWidget = MakePreviewContent(iconBrush, payload->Label);
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
