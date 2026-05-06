#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

class DemoDragPayload : public FDragDropPayload {
    DECLARE_OBJECT_WITH_PARENT(DemoDragPayload, FDragDropPayload)
    END_DECLARE_OBJECT()
};

class DemoDragSource : public ImButton {
public:
    explicit DemoDragSource(const std::string& text)
    {
        SetText(text);
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        FReply reply = ImButton::OnInputEvent(event);
        if (event.Type == EInputEventType::MouseButtonDown && event.MouseButton == EMouseButton::Left) {
            return FReply::Handled().DetectDrag(shared_from_this(), EMouseButton::Left);
        }
        return reply;
    }

    std::shared_ptr<FDragDropOperation> OnDragDetected(const FDragDetectEvent& event) override
    {
        auto op = std::make_shared<FDragDropOperation>();
        op->Payload = std::make_shared<DemoDragPayload>();
        auto preview = std::make_shared<ImTextBlock>();
        preview->SetText(GetText());
        preview->SetTextColor(FColor::White);
        preview->SetHitTestVisible(false);
        op->PreviewWidget = preview;
        op->PreviewOffset = FVector2(14.0f, 16.0f);
        return op;
    }
};

class DemoDropTarget : public ImWidget {
public:
    DemoDropTarget(const std::string& name, bool acceptDrop, const FColor& baseColor)
        : AcceptDrop_(acceptDrop)
        , BaseColor_(baseColor)
    {
        SetName(name);
    }

    void SetLabel(const std::shared_ptr<ImTextBlock>& label)
    {
        Label_ = label;
        ClearChildren();
        if (Label_) {
            AddChild(Label_);
        }
    }

    FVector2 GetMinSize() const override
    {
        return FVector2(220.0f, 120.0f);
    }

    void Paint(const FPaintContext& paintContext) override
    {
        FColor color = BaseColor_;
        if (Hovered_) {
            color = AcceptDrop_ ? FColor::FromBytes(44, 118, 74) : FColor::FromBytes(122, 66, 66);
        }

        paintContext.DrawContext_.DrawRectFilled(
            m_Geometry.GetMin(),
            m_Geometry.GetMax(),
            color,
            10.0f);
        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin(),
            m_Geometry.GetMax(),
            FColor::FromBytes(210, 218, 230),
            10.0f,
            1.0f);

        if (Label_) {
            const FVector2 labelSize = Label_->GetMinSize();
            const FVector2 labelPos(
                m_Geometry.Position.X + (m_Geometry.Size.X - labelSize.X) * 0.5f,
                m_Geometry.Position.Y + (m_Geometry.Size.Y - labelSize.Y) * 0.5f);
            Label_->SetGeometry(FGeometry(labelPos, labelSize));
            Label_->Paint(paintContext);
        }
    }

    FReply OnDragEvent(const FDragDropEvent& event) override
    {
        if (event.Type == EDragDropEventType::DragEnter) {
            Hovered_ = true;
            return AcceptDrop_ ? FReply::Handled() : FReply::Unhandled();
        }
        if (event.Type == EDragDropEventType::DragOver) {
            Hovered_ = true;
            return AcceptDrop_ ? FReply::Handled() : FReply::Unhandled();
        }
        if (event.Type == EDragDropEventType::DragLeave) {
            Hovered_ = false;
            return FReply::Unhandled();
        }
        if (event.Type == EDragDropEventType::Drop) {
            Hovered_ = false;
            if (AcceptDrop_ && Label_) {
                Label_->SetText("Dropped");
                return FReply::Handled();
            }
        }
        return FReply::Unhandled();
    }

private:
    bool AcceptDrop_ = false;
    bool Hovered_ = false;
    FColor BaseColor_;
    std::shared_ptr<ImTextBlock> Label_;
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(L"DragDrop Demo - ImWidgetV4", 960, 620);
    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"DragDropDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(16.0f);

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetText("Drag the left buttons into the green drop panel. The red panel rejects drops.");
    hint->SetFontSize(18.0f);
    root->AddChild(hint);

    auto content = std::make_shared<ImHorizontalBox>();
    content->SetSpacing(18.0f);
    root->AddChild(content);

    auto sourceColumn = std::make_shared<ImVerticalBox>();
    sourceColumn->SetSpacing(12.0f);
    content->AddChild(sourceColumn);

    auto sourceA = std::make_shared<DemoDragSource>("Palette Item A");
    auto sourceB = std::make_shared<DemoDragSource>("Palette Item B");
    sourceColumn->AddChild(sourceA);
    sourceColumn->AddChild(sourceB);

    auto acceptLabel = std::make_shared<ImTextBlock>();
    acceptLabel->SetText("Accept Drop");
    acceptLabel->SetFontSize(20.0f);
    auto acceptTarget = std::make_shared<DemoDropTarget>(
        "accept-target",
        true,
        FColor::FromBytes(37, 68, 50));
    acceptTarget->SetLabel(acceptLabel);

    auto rejectLabel = std::make_shared<ImTextBlock>();
    rejectLabel->SetText("Reject Drop");
    rejectLabel->SetFontSize(20.0f);
    auto rejectTarget = std::make_shared<DemoDropTarget>(
        "reject-target",
        false,
        FColor::FromBytes(69, 43, 43));
    rejectTarget->SetLabel(rejectLabel);

    content->AddChild(acceptTarget);
    content->AddChild(rejectTarget);

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();
    return 0;
}
