#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;

namespace {

class FTestDragPayload : public FDragDropPayload {
    DECLARE_OBJECT_WITH_PARENT(FTestDragPayload, FDragDropPayload)
    END_DECLARE_OBJECT()
};

class DragTestWidget : public ImWidget {
public:
    explicit DragTestWidget(const std::string& name, std::vector<std::string>* log = nullptr)
        : Log(log)
    {
        SetName(name);
    }

    bool bArmDragOnMouseDown = false;
    bool bCreateOperation = false;
    bool bCreateImagePreview = false;
    bool bAcceptDrag = false;
    bool bHandleDrop = false;
    std::vector<std::string>* Log = nullptr;

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (event.Type == EInputEventType::MouseButtonDown && bArmDragOnMouseDown) {
            if (Log) {
                Log->push_back("arm:" + GetName());
            }
            return FReply::Handled().DetectDrag(shared_from_this(), event.MouseButton);
        }
        return FReply::Unhandled();
    }

    std::shared_ptr<FDragDropOperation> OnDragDetected(const FDragDetectEvent& event) override
    {
        if (Log) {
            Log->push_back("detected:" + GetName());
        }
        if (!bCreateOperation) {
            return nullptr;
        }

        auto operation = std::make_shared<FDragDropOperation>();
        operation->Payload = std::make_shared<FTestDragPayload>();
        if (bCreateImagePreview) {
            auto previewRow = std::make_shared<ImHorizontalBox>();
            previewRow->SetHitTestVisible(false);

            auto previewImage = std::make_shared<ImImage>();
            previewImage->SetDesiredSize(FVector2(16.0f, 16.0f));
            previewImage->SetHitTestVisible(false);
            previewRow->AddChild(previewImage);

            auto previewText = std::make_shared<ImTextBlock>();
            previewText->SetText("Preview");
            previewText->SetHitTestVisible(false);
            previewRow->AddChild(previewText);

            operation->PreviewWidget = previewRow;
        } else {
            auto preview = std::make_shared<ImTextBlock>();
            preview->SetText("Preview");
            preview->SetHitTestVisible(false);
            operation->PreviewWidget = preview;
        }
        operation->PreviewOffset = FVector2(8.0f, 10.0f);
        return operation;
    }

    FReply OnDragEvent(const FDragDropEvent& event) override
    {
        if (Log) {
            Log->push_back(EventName(event.Type) + ":" + GetName());
        }

        if (event.Type == EDragDropEventType::DragOver && bAcceptDrag) {
            return FReply::Handled();
        }

        if (event.Type == EDragDropEventType::Drop && bHandleDrop) {
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

private:
    static std::string EventName(EDragDropEventType type)
    {
        switch (type) {
        case EDragDropEventType::DragStart: return "start";
        case EDragDropEventType::DragUpdate: return "update";
        case EDragDropEventType::DragEnd: return "end";
        case EDragDropEventType::DragEnter: return "enter";
        case EDragDropEventType::DragOver: return "over";
        case EDragDropEventType::DragLeave: return "leave";
        case EDragDropEventType::Drop: return "drop";
        default: return "unknown";
        }
    }
};

FInputEvent MouseEvent(EInputEventType type, const FVector2& position)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = EMouseButton::Left;
    return event;
}

FInputEvent KeyEvent(EInputEventType type, EKey key)
{
    FInputEvent event;
    event.Type = type;
    event.Key = key;
    return event;
}

} // namespace

class DragDropTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        App = std::make_shared<ImApplication>();

    Root = std::make_shared<DragTestWidget>("root", &Log);
    Source = std::make_shared<DragTestWidget>("source", &Log);
    Target = std::make_shared<DragTestWidget>("target", &Log);
    Rejector = std::make_shared<DragTestWidget>("rejector", &Log);

        Root->AddChild(Source);
        Root->AddChild(Target);
        Root->AddChild(Rejector);

        Root->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(420.0f, 180.0f)));
        Source->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(120.0f, 180.0f)));
        Target->SetGeometry(FGeometry(FVector2(140.0f, 0.0f), FVector2(120.0f, 180.0f)));
        Rejector->SetGeometry(FGeometry(FVector2(280.0f, 0.0f), FVector2(120.0f, 180.0f)));

        App->SetRootWidget(Root);
    }

    void Advance(const std::vector<FInputEvent>& events)
    {
        FFrameContext frameContext;
        frameContext.InputEvents = &events;
        frameContext.FrameInfo.ViewportSize = FVector2(420.0f, 180.0f);
        App->AdvanceFrame(frameContext);
    }

    std::shared_ptr<ImApplication> App;
    std::shared_ptr<DragTestWidget> Root;
    std::shared_ptr<DragTestWidget> Source;
    std::shared_ptr<DragTestWidget> Target;
    std::shared_ptr<DragTestWidget> Rejector;
    std::vector<std::string> Log;
};

TEST_F(DragDropTest, DragDoesNotStartBeforeThreshold)
{
    Source->bArmDragOnMouseDown = true;
    Source->bCreateOperation = true;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(21.0f, 21.0f))
    });

    EXPECT_FALSE(App->IsDragDropActive());
    EXPECT_EQ(std::find(Log.begin(), Log.end(), "detected:source"), Log.end());
}

TEST_F(DragDropTest, DragLifecycleRoutesThroughAcceptedTarget)
{
    Source->bArmDragOnMouseDown = true;
    Source->bCreateOperation = true;
    Target->bAcceptDrag = true;
    Target->bHandleDrop = true;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(40.0f, 24.0f))
    });

    EXPECT_TRUE(App->IsDragDropActive());
    ASSERT_NE(App->GetCurrentDragDropOperation(), nullptr);
    EXPECT_EQ(App->GetCurrentDropTarget(), nullptr);
    EXPECT_NE(std::find(Log.begin(), Log.end(), "detected:source"), Log.end());
    EXPECT_NE(std::find(Log.begin(), Log.end(), "start:source"), Log.end());

    Log.clear();
    Advance({MouseEvent(EInputEventType::MouseMove, FVector2(170.0f, 30.0f))});

    EXPECT_TRUE(App->IsDragDropActive());
    EXPECT_EQ(App->GetCurrentDropTarget(), Target);
    EXPECT_NE(std::find(Log.begin(), Log.end(), "update:source"), Log.end());
    EXPECT_NE(std::find(Log.begin(), Log.end(), "over:target"), Log.end());
    EXPECT_NE(std::find(Log.begin(), Log.end(), "enter:target"), Log.end());

    Log.clear();
    Advance({MouseEvent(EInputEventType::MouseButtonUp, FVector2(170.0f, 30.0f))});

    EXPECT_FALSE(App->IsDragDropActive());
    EXPECT_EQ(App->GetCurrentDragDropOperation(), nullptr);
    EXPECT_EQ(App->GetCurrentDropTarget(), nullptr);
    EXPECT_NE(std::find(Log.begin(), Log.end(), "drop:target"), Log.end());
    EXPECT_NE(std::find(Log.begin(), Log.end(), "end:source"), Log.end());
}

TEST_F(DragDropTest, EscapeCancelsActiveDrag)
{
    Source->bArmDragOnMouseDown = true;
    Source->bCreateOperation = true;
    Target->bAcceptDrag = true;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(28.0f, 24.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(170.0f, 30.0f))
    });

    ASSERT_TRUE(App->IsDragDropActive());
    ASSERT_EQ(App->GetCurrentDropTarget(), Target);

    Log.clear();
    Advance({KeyEvent(EInputEventType::KeyDown, EKey::Escape)});

    EXPECT_FALSE(App->IsDragDropActive());
    EXPECT_EQ(App->GetCurrentDropTarget(), nullptr);
    EXPECT_NE(std::find(Log.begin(), Log.end(), "leave:target"), Log.end());
    EXPECT_NE(std::find(Log.begin(), Log.end(), "end:source"), Log.end());
    EXPECT_EQ(std::find(Log.begin(), Log.end(), "drop:target"), Log.end());
}

TEST_F(DragDropTest, DragPreviewSubtreeGetsApplicationAssigned)
{
    Source->bArmDragOnMouseDown = true;
    Source->bCreateOperation = true;
    Source->bCreateImagePreview = true;

    Advance({
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)),
        MouseEvent(EInputEventType::MouseMove, FVector2(40.0f, 24.0f))
    });

    ASSERT_TRUE(App->IsDragDropActive());
    const std::shared_ptr<FDragDropOperation> operation = App->GetCurrentDragDropOperation();
    ASSERT_NE(operation, nullptr);
    ASSERT_NE(operation->PreviewWidget, nullptr);
    EXPECT_EQ(operation->PreviewWidget->GetApplication(), App.get());

    const auto& previewChildren = operation->PreviewWidget->GetChildren();
    ASSERT_FALSE(previewChildren.empty());
    EXPECT_EQ(previewChildren.front()->GetApplication(), App.get());
}
