#include <gtest/gtest.h>

#include "../src/commands/DocumentSnapshotCommand.h"
#include "../src/commands/AddWidgetCommand.h"
#include "../src/commands/MoveWidgetCommand.h"
#include "../src/commands/ReflectablePropertyCommand.h"
#include "../src/commands/RemoveWidgetCommand.h"
#include "../src/editor/EditorDocument.h"
#include "../src/editor/EditorShellHost.h"
#include "../src/editor/EditorSession.h"
#include "../src/editor/EditorWorkspaceController.h"
#include "../src/editor/SelectionModel.h"
#include "../src/palette/WidgetPaletteDragDrop.h"
#include "../src/palette/WidgetPaletteView.h"
#include "../src/serialization/WidgetFactory.h"
#include "../src/tree/DocumentTreeViewBinder.h"
#include "../src/tree/WidgetTreeDragDrop.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include "../src/inspector/ReflectionDetailsView.h"

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

class DesignerTestWidget : public ImWidget {
public:
    explicit DesignerTestWidget(const FVector2& minSize = FVector2(100.0f, 30.0f))
        : MinSize(minSize)
    {
        SetHitTestVisible(true);
    }

    FVector2 GetMinSize() const override
    {
        return MinSize;
    }

    FVector2 MinSize;
};

std::shared_ptr<ImWidget> BuildDocumentRoot()
{
    auto root = std::make_shared<ImVerticalBox>();
    root->SetName("Root");

    auto child = std::make_shared<ImButton>();
    child->SetName("Child");

    auto label = std::make_shared<ImTextBlock>();
    label->SetName("Label");
    label->SetText("Child");
    child->SetContent(label);

    root->AddChild(child);

    return root;
}

std::shared_ptr<ImWidget> BuildDesignerCanvasDocumentRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("CanvasRoot");
    canvas->SetDesiredSize(FVector2(400.0f, 300.0f));

    auto child = std::make_shared<DesignerTestWidget>();
    child->SetName("CanvasChild");

    canvas->AddChildAt(child, FVector2(0.10f, 0.15f));
    return canvas;
}

std::shared_ptr<ImWidget> BuildDesignerCanvasButtonDocumentRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("CanvasRoot");
    canvas->SetDesiredSize(FVector2(400.0f, 300.0f));

    auto button = std::make_shared<ImButton>();
    button->SetName("CanvasButton");
    button->SetText("Button");

    auto label = std::make_shared<ImTextBlock>();
    label->SetName("ButtonLabel");
    label->SetText("Button");
    button->SetContent(label);

    canvas->AddChildAt(button, FVector2(0.10f, 0.15f));
    return canvas;
}

FInputEvent MouseEvent(
    EInputEventType type,
    const FVector2& position,
    EMouseButton button = EMouseButton::Left)
{
    FInputEvent event;
    event.Type = type;
    event.MousePosition = position;
    event.MouseButton = button;
    return event;
}

void AdvanceAppWithDraw(
    ImApplication& app,
    const std::vector<FInputEvent>& events,
    const FVector2& viewportSize)
{
    if (!ImGui::GetCurrentContext()) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        io.Fonts->Build();
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = viewportSize.ToImVec2();
    io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();
    ImDrawList drawList(ImGui::GetDrawListSharedData());
    drawList._ResetForNewFrame();
    DrawContext drawContext(&drawList);

    FFrameContext frameContext;
    frameContext.InputEvents = &events;
    frameContext.FrameInfo.ViewportSize = viewportSize;
    frameContext.DrawContext_ = &drawContext;
    app.AdvanceFrame(frameContext);

    ImGui::EndFrame();
}

std::shared_ptr<DesignerTestWidget> GetCanvasDocumentButton(const std::shared_ptr<EditorSession>& session)
{
    auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(session->GetDocument()->GetRootWidget());
    if (!canvas || canvas->GetChildren().empty()) {
        return nullptr;
    }

    return std::dynamic_pointer_cast<DesignerTestWidget>(canvas->GetChildren().front());
}

ImCanvasPanelSlot* GetCanvasDocumentButtonSlot(const std::shared_ptr<EditorSession>& session)
{
    auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(session->GetDocument()->GetRootWidget());
    auto button = GetCanvasDocumentButton(session);
    if (!canvas || !button) {
        return nullptr;
    }

    return dynamic_cast<ImCanvasPanelSlot*>(canvas->GetSlotForChild(button));
}

void BindEditorSessionForTests(
    const std::shared_ptr<EditorSession>& session,
    const std::shared_ptr<ImDesignerSurface>& designerSurface)
{
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);
}

void SyncDesignerCanvasLayout(
    const std::shared_ptr<EditorSession>& session,
    const std::shared_ptr<ImDesignerSurface>& designerSurface)
{
    auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(session->GetDocument()->GetRootWidget());
    if (!canvas || !designerSurface) {
        return;
    }

    canvas->SetGeometry(designerSurface->GetGeometry());
    canvas->Relayout();
}

std::shared_ptr<EditorWorkspaceController> CreateBoundWorkspaceController(
    const std::shared_ptr<EditorShellHost>& shellHost,
    const std::shared_ptr<ImTabView>& documentTabs,
    const std::shared_ptr<ImTextOutlineView>& projectView,
    const std::shared_ptr<ImTextOutlineView>& widgetTreeView,
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextList>& outputText)
{
    auto workspaceController =
        std::make_shared<EditorWorkspaceController>(BuildDocumentRoot);
    shellHost->SetWorkspaceController(workspaceController);
    workspaceController->Bind(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);
    return workspaceController;
}

} // namespace

TEST(EditorSelectionTest, SelectionModelResolvesStableWidgetId)
{
    auto document = std::make_shared<EditorDocument>();
    document->NewDocument(BuildDocumentRoot(), "Main");

    SelectionModel selection;
    auto root = document->GetRootWidget();
    ASSERT_TRUE(root);

    auto child = root->GetChildren().empty() ? nullptr : root->GetChildren().front();
    ASSERT_TRUE(child);

    selection.SetSelectedWidget(child, document);
    EXPECT_FALSE(selection.GetSelectedWidgetId().empty());
    EXPECT_EQ(selection.ResolveSelectedWidget(document), child);
}

TEST(EditorSelectionTest, SelectionModelClearsWhenWidgetOrDocumentMissing)
{
    SelectionModel selection;
    selection.SetSelectedWidgetId("w42");
    selection.SetSelectedWidget(nullptr, nullptr);
    EXPECT_FALSE(selection.HasSelection());
    EXPECT_TRUE(selection.GetSelectedWidgetId().empty());
}

TEST(EditorSelectionTest, DocumentIdRoundTripKeepsSelectionIdStable)
{
    auto document = std::make_shared<EditorDocument>();
    document->NewDocument(BuildDocumentRoot(), "Main");

    auto root = document->GetRootWidget();
    ASSERT_TRUE(root);
    auto child = root->GetChildren().empty() ? nullptr : root->GetChildren().front();
    ASSERT_TRUE(child);

    const std::string childId = document->GetWidgetId(child);
    ASSERT_FALSE(childId.empty());

    const json snapshot = document->ExportDocumentJson();

    auto restored = std::make_shared<EditorDocument>();
    std::string errorMessage;
    ASSERT_TRUE(restored->ImportDocumentJson(snapshot, &errorMessage)) << errorMessage;
    EXPECT_EQ(restored->GetWidgetId(restored->GetRootWidget()->GetChildren().front()), childId);
    EXPECT_EQ(restored->FindWidgetById(childId), restored->GetRootWidget()->GetChildren().front());
}

TEST(EditorSelectionTest, ApplyDocumentSnapshotRestoresDesignerSelection)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto document = session->GetDocument();
    ASSERT_TRUE(document);
    auto root = document->GetRootWidget();
    ASSERT_TRUE(root);
    ASSERT_FALSE(root->GetChildren().empty());
    auto child = root->GetChildren().front();
    ASSERT_TRUE(child);

    const std::string childId = document->GetWidgetId(child);
    ASSERT_FALSE(childId.empty());

    const json documentJson = document->ExportDocumentJson();

    DocumentSnapshotCommand restoreCommand(
        session,
        "Restore Selection",
        documentJson,
        "",
        false,
        documentJson,
        childId,
        false);

    EXPECT_TRUE(restoreCommand.Execute());
    ASSERT_TRUE(designerSurface->GetSelectedWidget());
    EXPECT_EQ(
        session->GetDocument()->GetWidgetId(designerSurface->GetSelectedWidget()),
        childId);

    EXPECT_TRUE(restoreCommand.Undo());
    EXPECT_EQ(designerSurface->GetSelectedWidget(), nullptr);
}

TEST(EditorSelectionTest, BindDocumentWidgetsPopulatesSchemaViewImmediately)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    ASSERT_TRUE(session->GetDocument());
    const std::vector<std::string>& items = schemaText->GetItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items.front(), session->GetDocument()->ExportDocumentJson().dump(2));
}

TEST(EditorSelectionTest, EmptyDesignerSurfaceRightClickOpensRootCreationMenu)
{
    auto session = std::make_shared<EditorSession>([]() {
        return std::shared_ptr<ImWidget>();
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    AdvanceAppWithDraw(*app, {}, FVector2(320.0f, 240.0f));
    const std::size_t beforeWindowCount = app->GetWindowManager().GetOpenWindows().size();

    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, FVector2(24.0f, 24.0f), EMouseButton::Right)},
        FVector2(320.0f, 240.0f));

    const std::size_t afterWindowCount = app->GetWindowManager().GetOpenWindows().size();
    EXPECT_GT(afterWindowCount, beforeWindowCount);
}

TEST(EditorSelectionTest, DesignerDragOverUsesPointerHitTargetInsteadOfCurrentSelection)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto root = std::make_shared<ImHorizontalBox>();

        auto first = std::make_shared<ImVerticalBox>();
        first->SetName("FirstColumn");
        auto firstLabel = std::make_shared<ImTextBlock>();
        firstLabel->SetText("First");
        first->AddChild(firstLabel);
        root->AddChild(first);

        auto second = std::make_shared<ImVerticalBox>();
        second->SetName("SecondColumn");
        auto secondLabel = std::make_shared<ImTextBlock>();
        secondLabel->SetText("Second");
        second->AddChild(secondLabel);
        root->AddChild(second);

        return root;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);
    AdvanceAppWithDraw(*app, {}, FVector2(420.0f, 260.0f));

    auto root = std::dynamic_pointer_cast<ImHorizontalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    auto first = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[0]);
    auto second = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[1]);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    designerSurface->SetSelectedWidget(first);

    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = "ImTextBlock";
    payload->Label = "TextBlock";

    auto operation = std::make_shared<FDragDropOperation>();
    operation->Payload = payload;

    FDragDropEvent dragOverEvent;
    dragOverEvent.Type = EDragDropEventType::DragOver;
    dragOverEvent.Operation = operation;
    dragOverEvent.CurrentPosition = second->GetGeometry().GetCenter();

    const FReply reply = designerSurface->OnDragEvent(dragOverEvent);
    EXPECT_TRUE(reply.IsHandled());
}

TEST(EditorSelectionTest, DesignerDragOverRejectsOccupiedSingleContentTarget)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto button = std::make_shared<ImButton>();
        button->SetName("RootButton");
        auto label = std::make_shared<ImTextBlock>();
        label->SetText("Button Label");
        button->SetContent(label);
        return button;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);
    AdvanceAppWithDraw(*app, {}, FVector2(420.0f, 260.0f));

    auto button = std::dynamic_pointer_cast<ImButton>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(button);
    auto label = button->GetContent();
    ASSERT_TRUE(label);

    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = "ImTextBlock";
    payload->Label = "TextBlock";

    auto operation = std::make_shared<FDragDropOperation>();
    operation->Payload = payload;

    FDragDropEvent dragOverEvent;
    dragOverEvent.Type = EDragDropEventType::DragOver;
    dragOverEvent.Operation = operation;
    dragOverEvent.CurrentPosition = label->GetGeometry().GetCenter();

    const FReply reply = designerSurface->OnDragEvent(dragOverEvent);
    EXPECT_FALSE(reply.IsHandled());
}

TEST(EditorSelectionTest, DesignerDropUsesPointerHitTargetInsteadOfCurrentSelection)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto root = std::make_shared<ImHorizontalBox>();

        auto first = std::make_shared<ImVerticalBox>();
        first->SetName("FirstColumn");
        auto firstLabel = std::make_shared<ImTextBlock>();
        firstLabel->SetText("First");
        first->AddChild(firstLabel);
        root->AddChild(first);

        auto second = std::make_shared<ImVerticalBox>();
        second->SetName("SecondColumn");
        auto secondLabel = std::make_shared<ImTextBlock>();
        secondLabel->SetText("Second");
        second->AddChild(secondLabel);
        root->AddChild(second);

        return root;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);
    AdvanceAppWithDraw(*app, {}, FVector2(420.0f, 260.0f));

    auto root = std::dynamic_pointer_cast<ImHorizontalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    ASSERT_EQ(root->GetChildren().size(), 2u);
    auto first = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[0]);
    auto second = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[1]);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    ASSERT_TRUE(first->GetGeometry().IsValid());
    ASSERT_TRUE(second->GetGeometry().IsValid());
    ASSERT_EQ(first->GetChildren().size(), 1u);
    ASSERT_EQ(second->GetChildren().size(), 1u);

    designerSurface->SetSelectedWidget(first);

    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = "ImTextBlock";
    payload->Label = "TextBlock";

    auto operation = std::make_shared<FDragDropOperation>();
    operation->Payload = payload;

    bool bHandled = false;
    designerSurface->OnDropReceived.Broadcast(
        *designerSurface,
        operation,
        second->GetGeometry().GetCenter(),
        bHandled);

    EXPECT_TRUE(bHandled);
    ASSERT_EQ(first->GetChildren().size(), 1u);
    ASSERT_EQ(second->GetChildren().size(), 2u);
    EXPECT_EQ(second->GetChildren().back()->GetTypeName(), "ImTextBlock");
}

TEST(EditorSelectionTest, DesignerDropMovesExistingWidgetByPointerHitTargetAndSupportsUndoRedo)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto root = std::make_shared<ImHorizontalBox>();

        auto first = std::make_shared<ImVerticalBox>();
        first->SetName("FirstColumn");
        auto firstHeader = std::make_shared<ImTextBlock>();
        firstHeader->SetText("First");
        first->AddChild(firstHeader);
        auto movable = std::make_shared<ImTextBlock>();
        movable->SetName("MovableText");
        movable->SetText("Move Me");
        first->AddChild(movable);
        root->AddChild(first);

        auto second = std::make_shared<ImVerticalBox>();
        second->SetName("SecondColumn");
        auto secondHeader = std::make_shared<ImTextBlock>();
        secondHeader->SetText("Second");
        second->AddChild(secondHeader);
        root->AddChild(second);

        return root;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);
    AdvanceAppWithDraw(*app, {}, FVector2(420.0f, 260.0f));

    auto root = std::dynamic_pointer_cast<ImHorizontalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    ASSERT_EQ(root->GetChildren().size(), 2u);
    auto first = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[0]);
    auto second = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[1]);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    ASSERT_EQ(first->GetChildren().size(), 2u);
    ASSERT_EQ(second->GetChildren().size(), 1u);

    auto movable = first->GetChildren()[1];
    ASSERT_TRUE(movable);
    const std::string movableId = session->GetDocument()->GetWidgetId(movable);
    ASSERT_FALSE(movableId.empty());

    designerSurface->SetSelectedWidget(first);

    auto payload = std::make_shared<WidgetTreeDragDropPayload>();
    payload->WidgetId = movableId;
    payload->Label = "MovableText";

    auto operation = std::make_shared<FDragDropOperation>();
    operation->Payload = payload;

    bool bHandled = false;
    designerSurface->OnDropReceived.Broadcast(
        *designerSurface,
        operation,
        second->GetGeometry().GetCenter(),
        bHandled);

    EXPECT_TRUE(bHandled);
    ASSERT_EQ(first->GetChildren().size(), 1u);
    ASSERT_EQ(second->GetChildren().size(), 2u);
    ASSERT_TRUE(designerSurface->GetSelectedWidget());
    EXPECT_EQ(session->GetDocument()->GetWidgetId(designerSurface->GetSelectedWidget()), movableId);

    ASSERT_TRUE(session->Undo());
    auto rootAfterUndo = std::dynamic_pointer_cast<ImHorizontalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(rootAfterUndo);
    auto firstAfterUndo = std::dynamic_pointer_cast<ImVerticalBox>(rootAfterUndo->GetChildren()[0]);
    auto secondAfterUndo = std::dynamic_pointer_cast<ImVerticalBox>(rootAfterUndo->GetChildren()[1]);
    ASSERT_TRUE(firstAfterUndo);
    ASSERT_TRUE(secondAfterUndo);
    EXPECT_EQ(firstAfterUndo->GetChildren().size(), 2u);
    EXPECT_EQ(secondAfterUndo->GetChildren().size(), 1u);

    ASSERT_TRUE(session->Redo());
    auto rootAfterRedo = std::dynamic_pointer_cast<ImHorizontalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(rootAfterRedo);
    auto firstAfterRedo = std::dynamic_pointer_cast<ImVerticalBox>(rootAfterRedo->GetChildren()[0]);
    auto secondAfterRedo = std::dynamic_pointer_cast<ImVerticalBox>(rootAfterRedo->GetChildren()[1]);
    ASSERT_TRUE(firstAfterRedo);
    ASSERT_TRUE(secondAfterRedo);
    EXPECT_EQ(firstAfterRedo->GetChildren().size(), 1u);
    EXPECT_EQ(secondAfterRedo->GetChildren().size(), 2u);
}

TEST(EditorSelectionTest, TreeDropTestRejectsSiblingInsertAroundSingleContentParentChild)
{
    auto document = std::make_shared<EditorDocument>();
    document->NewDocument(BuildDocumentRoot(), "Main");

    auto treeView = std::make_shared<ImTextOutlineView>();
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    DocumentTreeViewBinder binder;
    binder.Bind(treeView, designerSurface, document);
    binder.RebuildFromRoot(document->GetRootWidget(), nullptr);

    auto root = document->GetRootWidget();
    ASSERT_TRUE(root);
    auto button = std::dynamic_pointer_cast<ImButton>(root->GetChildren().front());
    ASSERT_TRUE(button);
    auto label = button->GetContent();
    ASSERT_TRUE(label);

    ImTextOutlineItem* labelItem = binder.ResolveItem(label);
    ASSERT_TRUE(labelItem != nullptr);

    auto payload = std::make_shared<WidgetPalettePayload>();
    payload->WidgetTypeName = "ImTextBlock";
    payload->Label = "TextBlock";

    auto operation = std::make_shared<FDragDropOperation>();
    operation->Payload = payload;

    bool bAccepted = true;
    treeView->OnItemDropTest.Broadcast(
        *treeView,
        *labelItem,
        ETextOutlineDropZone::BeforeItem,
        operation,
        FVector2(0.0f, 0.0f),
        bAccepted);
    EXPECT_FALSE(bAccepted);

    bAccepted = false;
    treeView->OnItemDropTest.Broadcast(
        *treeView,
        *labelItem,
        ETextOutlineDropZone::OnItem,
        operation,
        FVector2(0.0f, 0.0f),
        bAccepted);
    EXPECT_FALSE(bAccepted);
}

TEST(EditorSelectionTest, AddWidgetCommandRestoresTreeOnUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = session->GetDocument()->GetRootWidget();
    auto inserted = std::make_shared<ImTextBlock>();
    inserted->SetName("Inserted");
    inserted->SetText("Inserted");

    const std::size_t beforeCount = root->GetChildren().size();
    AddWidgetCommand command(
        session,
        "Add Widget",
        inserted,
        root,
        FVector2(0.0f, 0.0f),
        ETextOutlineDropZone::OnItem,
        AddWidgetCommand::EInsertionMode::TreeTarget,
        inserted,
        false,
        true);

    ASSERT_TRUE(command.Execute());
    ASSERT_EQ(root->GetChildren().size(), beforeCount + 1);
    EXPECT_EQ(root->GetChildren().back(), inserted);

    ASSERT_TRUE(command.Undo());
    ASSERT_EQ(root->GetChildren().size(), beforeCount);

    ASSERT_TRUE(command.Execute());
    ASSERT_EQ(root->GetChildren().size(), beforeCount + 1);
    EXPECT_EQ(root->GetChildren().back(), inserted);
}

TEST(EditorSelectionTest, RemoveWidgetCommandRestoresTreeOnUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = session->GetDocument()->GetRootWidget();
    ASSERT_TRUE(root);
    ASSERT_FALSE(root->GetChildren().empty());
    auto child = root->GetChildren().front();
    const std::size_t beforeCount = root->GetChildren().size();

    RemoveWidgetCommand command(
        session,
        "Delete Widget",
        child,
        root,
        root,
        false,
        true);

    ASSERT_TRUE(command.Execute());
    ASSERT_EQ(root->GetChildren().size(), beforeCount - 1);

    ASSERT_TRUE(command.Undo());
    ASSERT_EQ(root->GetChildren().size(), beforeCount);
    EXPECT_EQ(root->GetChildren().back(), child);
}

TEST(EditorSelectionTest, MoveWidgetCommandRestoresOrderOnUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = std::dynamic_pointer_cast<ImVerticalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);

    auto second = std::make_shared<ImTextBlock>();
    second->SetName("Second");
    second->SetText("Second");
    root->AddChild(second);

    ASSERT_EQ(root->GetChildren().size(), 2u);
    auto first = root->GetChildren().front();

    MoveWidgetCommand command(
        session,
        "Move Widget",
        first,
        root,
        0,
        root,
        1,
        first,
        false,
        true);

    ASSERT_TRUE(command.Execute());
    ASSERT_EQ(root->GetChildren().size(), 2u);
    EXPECT_EQ(root->GetChildren().front(), second);
    EXPECT_EQ(root->GetChildren().back(), first);

    ASSERT_TRUE(command.Undo());
    ASSERT_EQ(root->GetChildren().front(), first);
    EXPECT_EQ(root->GetChildren().back(), second);
}

TEST(EditorSelectionTest, MoveWidgetCommandRestoresCrossParentMoveOnUndoRedo)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto root = std::make_shared<ImVerticalBox>();

        auto left = std::make_shared<ImVerticalBox>();
        left->SetName("Left");
        auto right = std::make_shared<ImVerticalBox>();
        right->SetName("Right");

        auto moved = std::make_shared<ImTextBlock>();
        moved->SetName("Moved");
        moved->SetText("Moved");
        left->AddChild(moved);

        root->AddChild(left);
        root->AddChild(right);
        return root;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = std::dynamic_pointer_cast<ImVerticalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    auto left = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[0]);
    auto right = std::dynamic_pointer_cast<ImVerticalBox>(root->GetChildren()[1]);
    ASSERT_TRUE(left);
    ASSERT_TRUE(right);
    ASSERT_EQ(left->GetChildren().size(), 1u);
    auto moved = left->GetChildren().front();

    MoveWidgetCommand command(
        session,
        "Move Widget",
        moved,
        left,
        0,
        right,
        0,
        moved,
        false,
        true);

    ASSERT_TRUE(command.Execute());
    EXPECT_TRUE(left->GetChildren().empty());
    ASSERT_EQ(right->GetChildren().size(), 1u);
    EXPECT_EQ(right->GetChildren().front(), moved);

    ASSERT_TRUE(command.Undo());
    ASSERT_EQ(left->GetChildren().size(), 1u);
    EXPECT_EQ(left->GetChildren().front(), moved);
    EXPECT_TRUE(right->GetChildren().empty());
}

TEST(EditorSelectionTest, AddAndMoveCommandsWorkWithTabViewContent)
{
    auto session = std::make_shared<EditorSession>([]() {
        auto root = std::make_shared<ImVerticalBox>();
        auto tabView = std::make_shared<ImTabView>();

        auto first = std::make_shared<ImTextBlock>();
        first->SetName("FirstTab");
        first->SetText("First");
        tabView->AddTab("First", first);

        auto second = std::make_shared<ImTextBlock>();
        second->SetName("SecondTab");
        second->SetText("Second");
        tabView->AddTab("Second", second);

        root->AddChild(tabView);
        return root;
    });
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = std::dynamic_pointer_cast<ImVerticalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    auto tabView = std::dynamic_pointer_cast<ImTabView>(root->GetChildren().front());
    ASSERT_TRUE(tabView);
    ASSERT_EQ(tabView->GetTabCount(), 2);

    auto added = std::make_shared<ImTextBlock>();
    added->SetName("ThirdTab");
    added->SetText("Third");

    AddWidgetCommand addCommand(
        session,
        "Add Widget",
        added,
        tabView,
        FVector2(0.0f, 0.0f),
        ETextOutlineDropZone::OnItem,
        AddWidgetCommand::EInsertionMode::TreeTarget,
        added,
        false,
        true);

    ASSERT_TRUE(addCommand.Execute());
    ASSERT_EQ(tabView->GetTabCount(), 3);
    EXPECT_EQ(tabView->GetTab(2)->Content, added);

    MoveWidgetCommand moveCommand(
        session,
        "Move Widget",
        added,
        tabView,
        2,
        tabView,
        0,
        added,
        true,
        true);

    ASSERT_TRUE(moveCommand.Execute());
    ASSERT_EQ(tabView->GetTabCount(), 3);
    EXPECT_EQ(tabView->GetTab(0)->Content, added);

    ASSERT_TRUE(moveCommand.Undo());
    EXPECT_EQ(tabView->GetTab(2)->Content, added);

    ASSERT_TRUE(addCommand.Undo());
    ASSERT_EQ(tabView->GetTabCount(), 2);
}

TEST(EditorSelectionTest, ReflectablePropertyCommandRestoresPropertyOnUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = session->GetDocument()->GetRootWidget();
    ASSERT_TRUE(root);
    auto button = std::dynamic_pointer_cast<ImButton>(root->GetChildren().front());
    ASSERT_TRUE(button);

    json beforeJson = button->ToJson();
    json afterJson = beforeJson;
    afterJson["Properties"]["ImButton::Text"] = "Updated";

    ReflectablePropertyCommand command(
        session,
        button,
        "Edit Text",
        beforeJson,
        afterJson,
        button,
        false,
        true);

    ASSERT_TRUE(command.Execute());
    EXPECT_EQ(button->GetText(), "Updated");

    ASSERT_TRUE(command.Undo());
    EXPECT_EQ(button->GetText(), "Child");

    ASSERT_TRUE(command.Execute());
    EXPECT_EQ(button->GetText(), "Updated");
}

TEST(EditorSelectionTest, DetailsRenameKeepsWidgetNamesUnique)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = std::dynamic_pointer_cast<ImVerticalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    auto button = std::dynamic_pointer_cast<ImButton>(root->GetChildren().front());
    ASSERT_TRUE(button);

    designerSurface->SetSelectedWidget(button);
    detailsView->OnPropertyValueCommitted.Broadcast(
        *detailsView,
        button,
        "ImWidget",
        "Name",
        json("Root"));

    EXPECT_EQ(button->GetName(), "Root1");
}

TEST(EditorSelectionTest, SessionPasteSelectedWidgetSupportsUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = session->GetDocument()->GetRootWidget();
    ASSERT_TRUE(root);
    ASSERT_EQ(root->GetChildren().size(), 1u);

    auto child = root->GetChildren().front();
    designerSurface->SetSelectedWidget(child);

    ASSERT_TRUE(session->CopySelectedWidget());
    ASSERT_TRUE(session->PasteCopiedWidget());
    ASSERT_EQ(root->GetChildren().size(), 2u);
    EXPECT_EQ(root->GetChildren()[0]->GetName(), "Child");
    EXPECT_EQ(root->GetChildren()[1]->GetName(), "Button1");

    ASSERT_TRUE(session->Undo());
    ASSERT_EQ(root->GetChildren().size(), 1u);

    ASSERT_TRUE(session->Redo());
    ASSERT_EQ(root->GetChildren().size(), 2u);
}

TEST(EditorSelectionTest, SessionDuplicateSelectedWidgetSupportsUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto root = session->GetDocument()->GetRootWidget();
    ASSERT_TRUE(root);
    ASSERT_EQ(root->GetChildren().size(), 1u);

    auto child = root->GetChildren().front();
    designerSurface->SetSelectedWidget(child);

    ASSERT_TRUE(session->DuplicateSelectedWidget());
    ASSERT_EQ(root->GetChildren().size(), 2u);
    EXPECT_EQ(root->GetChildren()[0]->GetName(), "Child");
    EXPECT_EQ(root->GetChildren()[1]->GetName(), "Button1");

    ASSERT_TRUE(session->Undo());
    ASSERT_EQ(root->GetChildren().size(), 1u);

    ASSERT_TRUE(session->Redo());
    ASSERT_EQ(root->GetChildren().size(), 2u);
}

TEST(EditorSelectionTest, PaletteCreatedWidgetsReceiveUniqueTypeNumberNames)
{
    auto session = std::make_shared<EditorSession>(BuildDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto schemaText = std::make_shared<ImTextList>();
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        schemaText,
        designerSurface,
        nullptr,
        detailsView,
        nullptr);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    const FVector2 kViewportSize(400.0f, 300.0f);
    AdvanceAppWithDraw(*app, {}, kViewportSize);

    auto root = std::dynamic_pointer_cast<ImVerticalBox>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(root);
    ASSERT_TRUE(root->GetGeometry().IsValid());

    auto makePaletteOperation = []() {
        auto payload = std::make_shared<WidgetPalettePayload>();
        payload->WidgetTypeName = "ImButton";
        payload->Label = "Button";

        auto operation = std::make_shared<FDragDropOperation>();
        operation->Payload = payload;
        return operation;
    };

    bool firstHandled = false;
    designerSurface->OnDropReceived.Broadcast(
        *designerSurface,
        makePaletteOperation(),
        root->GetGeometry().GetCenter(),
        firstHandled);
    EXPECT_TRUE(firstHandled);

    bool secondHandled = false;
    designerSurface->OnDropReceived.Broadcast(
        *designerSurface,
        makePaletteOperation(),
        root->GetGeometry().GetCenter(),
        secondHandled);
    EXPECT_TRUE(secondHandled);

    ASSERT_EQ(root->GetChildren().size(), 3u);
    EXPECT_EQ(root->GetChildren()[1]->GetName(), "Button1");
    EXPECT_EQ(root->GetChildren()[2]->GetName(), "Button2");
}

TEST(EditorSelectionTest, DesignerMoveTransformSupportsUndoRedo)
{
    auto session = std::make_shared<EditorSession>(BuildDesignerCanvasDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    BindEditorSessionForTests(session, designerSurface);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    const FVector2 kViewportSize(400.0f, 300.0f);
    AdvanceAppWithDraw(*app, {}, kViewportSize);
    SyncDesignerCanvasLayout(session, designerSurface);

    auto button = GetCanvasDocumentButton(session);
    auto* slot = GetCanvasDocumentButtonSlot(session);
    ASSERT_TRUE(button);
    ASSERT_NE(slot, nullptr);
    ASSERT_TRUE(button->GetGeometry().IsValid());

    const FVector2 selectPoint = button->GetGeometry().GetCenter();
    AdvanceAppWithDraw(
        *app,
        {
            MouseEvent(EInputEventType::MouseButtonDown, selectPoint),
            MouseEvent(EInputEventType::MouseButtonUp, selectPoint)
        },
        kViewportSize);

    ASSERT_EQ(designerSurface->GetSelectedWidget(), button);

    const FVector2 beforePosition = slot->GetRelativePosition();
    const FVector2 beforeSize = slot->GetRelativeSize();
    const bool beforeAutoSize = slot->GetAutoSize();

    const FVector2 dragTarget = selectPoint + FVector2(40.0f, 30.0f);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, selectPoint)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseMove, dragTarget)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonUp, dragTarget)},
        kViewportSize);

    EXPECT_NEAR(slot->GetRelativePosition().X, beforePosition.X + 0.10f, 0.0001f);
    EXPECT_NEAR(slot->GetRelativePosition().Y, beforePosition.Y + 0.10f, 0.0001f);
    EXPECT_EQ(slot->GetRelativeSize(), beforeSize);
    EXPECT_EQ(slot->GetAutoSize(), beforeAutoSize);
    EXPECT_TRUE(session->CanUndo());

    ASSERT_TRUE(session->Undo());
    EXPECT_NEAR(slot->GetRelativePosition().X, beforePosition.X, 0.0001f);
    EXPECT_NEAR(slot->GetRelativePosition().Y, beforePosition.Y, 0.0001f);
    EXPECT_EQ(slot->GetRelativeSize(), beforeSize);
    EXPECT_EQ(slot->GetAutoSize(), beforeAutoSize);

    ASSERT_TRUE(session->Redo());
    EXPECT_NEAR(slot->GetRelativePosition().X, beforePosition.X + 0.10f, 0.0001f);
    EXPECT_NEAR(slot->GetRelativePosition().Y, beforePosition.Y + 0.10f, 0.0001f);
}

TEST(EditorSelectionTest, DesignerMoveHandleTakesPriorityOverButtonTextChild)
{
    auto session = std::make_shared<EditorSession>(BuildDesignerCanvasButtonDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    BindEditorSessionForTests(session, designerSurface);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    const FVector2 kViewportSize(400.0f, 300.0f);
    AdvanceAppWithDraw(*app, {}, kViewportSize);
    SyncDesignerCanvasLayout(session, designerSurface);

    auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(session->GetDocument()->GetRootWidget());
    ASSERT_TRUE(canvas);
    ASSERT_EQ(canvas->GetChildren().size(), 1u);
    auto button = std::dynamic_pointer_cast<ImButton>(canvas->GetChildren().front());
    ASSERT_TRUE(button);
    ASSERT_TRUE(button->GetGeometry().IsValid());

    designerSurface->SetSelectedWidget(button);
    ASSERT_EQ(designerSurface->GetSelectedWidget(), button);

    const FVector2 selectPoint = button->GetGeometry().GetCenter();

    const FVector2 dragTarget = selectPoint + FVector2(20.0f, 10.0f);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, selectPoint)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseMove, dragTarget)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonUp, dragTarget)},
        kViewportSize);

    ASSERT_EQ(designerSurface->GetSelectedWidget(), button);
}

TEST(EditorSelectionTest, WidgetPaletteItemButtonClearsPressedStateWhenGlobalDragStartsAndEnds)
{
    FWidgetPaletteEntry entry {
        "Button",
        "ImButton",
        ECoreIcon::Button
    };

    auto paletteButton = std::make_shared<WidgetPaletteItemButton>(entry);
    paletteButton->SetGeometry(FGeometry(FVector2(0.0f, 0.0f), FVector2(160.0f, 40.0f)));

    const FReply downReply = paletteButton->OnInputEvent(
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)));
    EXPECT_TRUE(downReply.IsHandled());
    EXPECT_TRUE(paletteButton->IsPressed());

    FDragDropEvent dragStartEvent;
    dragStartEvent.Type = EDragDropEventType::DragStart;
    dragStartEvent.SourceWidget = paletteButton;
    paletteButton->OnDragEvent(dragStartEvent);
    EXPECT_FALSE(paletteButton->IsPressed());

    const FReply secondDownReply = paletteButton->OnInputEvent(
        MouseEvent(EInputEventType::MouseButtonDown, FVector2(20.0f, 20.0f)));
    EXPECT_TRUE(secondDownReply.IsHandled());
    EXPECT_TRUE(paletteButton->IsPressed());

    FDragDropEvent dragEndEvent;
    dragEndEvent.Type = EDragDropEventType::DragEnd;
    dragEndEvent.SourceWidget = paletteButton;
    paletteButton->OnDragEvent(dragEndEvent);
    EXPECT_FALSE(paletteButton->IsPressed());
}

TEST(EditorSelectionTest, WidgetPaletteEntriesStayInSyncWithWidgetFactory)
{
    const std::vector<FWidgetPaletteEntry> entries = BuildDefaultWidgetPaletteEntries();
    const WidgetFactory& widgetFactory = WidgetFactory::Get();

    EXPECT_GE(entries.size(), 20u);

    const auto hasEntry = [&](const std::string& typeName) {
        return std::any_of(entries.begin(), entries.end(), [&](const FWidgetPaletteEntry& entry) {
            return entry.TypeName == typeName;
        });
    };

    EXPECT_TRUE(hasEntry("ImHorizontalSplitter"));
    EXPECT_TRUE(hasEntry("ImVerticalSplitter"));
    EXPECT_TRUE(hasEntry("ImTextList"));
    EXPECT_TRUE(hasEntry("ImTextOutlineView"));
    EXPECT_TRUE(hasEntry("ImOutlineView"));
    EXPECT_TRUE(hasEntry("ImListView"));
    EXPECT_TRUE(hasEntry("ImComboBox"));
    EXPECT_TRUE(hasEntry("ImColorPicker"));

    for (const FWidgetPaletteEntry& entry : entries) {
        EXPECT_TRUE(widgetFactory.SupportsWidgetType(entry.TypeName)) << entry.TypeName;
    }
}

TEST(EditorSelectionTest, DesignerResizeTransformSupportsUndoRedoAndRestoresAutoSize)
{
    auto session = std::make_shared<EditorSession>(BuildDesignerCanvasDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    BindEditorSessionForTests(session, designerSurface);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    const FVector2 kViewportSize(400.0f, 300.0f);
    AdvanceAppWithDraw(*app, {}, kViewportSize);
    SyncDesignerCanvasLayout(session, designerSurface);

    auto button = GetCanvasDocumentButton(session);
    auto* slot = GetCanvasDocumentButtonSlot(session);
    ASSERT_TRUE(button);
    ASSERT_NE(slot, nullptr);
    ASSERT_TRUE(slot->GetAutoSize());
    ASSERT_TRUE(button->GetGeometry().IsValid());

    const FVector2 selectPoint = button->GetGeometry().GetCenter();
    AdvanceAppWithDraw(
        *app,
        {
            MouseEvent(EInputEventType::MouseButtonDown, selectPoint),
            MouseEvent(EInputEventType::MouseButtonUp, selectPoint)
        },
        kViewportSize);

    ASSERT_EQ(designerSurface->GetSelectedWidget(), button);

    const FVector2 beforePosition = slot->GetRelativePosition();
    const FVector2 beforeRelativeSize = slot->GetRelativeSize();
    const bool beforeAutoSize = slot->GetAutoSize();
    const FVector2 beforePixelSize = button->GetGeometry().Size;
    const FVector2 beforeEffectiveRelativeSize(
        beforePixelSize.X / kViewportSize.X,
        beforePixelSize.Y / kViewportSize.Y);

    const FVector2 resizeHandlePoint = button->GetGeometry().GetMax() - FVector2(2.0f, 2.0f);
    const FVector2 resizeTarget = resizeHandlePoint + FVector2(40.0f, 30.0f);

    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, resizeHandlePoint)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseMove, resizeTarget)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonUp, resizeTarget)},
        kViewportSize);

    const FVector2 expectedRelativeSize(
        beforeEffectiveRelativeSize.X + (40.0f / kViewportSize.X),
        beforeEffectiveRelativeSize.Y + (30.0f / kViewportSize.Y));

    EXPECT_EQ(slot->GetRelativePosition(), beforePosition);
    EXPECT_FALSE(slot->GetAutoSize());
    EXPECT_NEAR(slot->GetRelativeSize().X, expectedRelativeSize.X, 0.0001f);
    EXPECT_NEAR(slot->GetRelativeSize().Y, expectedRelativeSize.Y, 0.0001f);
    EXPECT_TRUE(session->CanUndo());

    ASSERT_TRUE(session->Undo());
    EXPECT_EQ(slot->GetRelativePosition(), beforePosition);
    EXPECT_EQ(slot->GetRelativeSize(), beforeRelativeSize);
    EXPECT_EQ(slot->GetAutoSize(), beforeAutoSize);

    ASSERT_TRUE(session->Redo());
    EXPECT_FALSE(slot->GetAutoSize());
    EXPECT_NEAR(slot->GetRelativeSize().X, expectedRelativeSize.X, 0.0001f);
    EXPECT_NEAR(slot->GetRelativeSize().Y, expectedRelativeSize.Y, 0.0001f);
}

TEST(EditorSelectionTest, DesignerTopLeftResizeUpdatesPositionAndSize)
{
    auto session = std::make_shared<EditorSession>(BuildDesignerCanvasDocumentRoot);
    auto designerSurface = std::make_shared<ImDesignerSurface>();
    BindEditorSessionForTests(session, designerSurface);

    auto app = std::make_shared<ImApplication>();
    app->SetRootWidget(designerSurface);

    const FVector2 kViewportSize(400.0f, 300.0f);
    AdvanceAppWithDraw(*app, {}, kViewportSize);
    SyncDesignerCanvasLayout(session, designerSurface);

    auto button = GetCanvasDocumentButton(session);
    auto* slot = GetCanvasDocumentButtonSlot(session);
    ASSERT_TRUE(button);
    ASSERT_NE(slot, nullptr);
    ASSERT_TRUE(button->GetGeometry().IsValid());

    const FVector2 selectPoint = button->GetGeometry().GetCenter();
    AdvanceAppWithDraw(
        *app,
        {
            MouseEvent(EInputEventType::MouseButtonDown, selectPoint),
            MouseEvent(EInputEventType::MouseButtonUp, selectPoint)
        },
        kViewportSize);

    ASSERT_EQ(designerSurface->GetSelectedWidget(), button);

    const FVector2 beforePosition = slot->GetRelativePosition();
    const FVector2 beforePixelSize = button->GetGeometry().Size;
    const FVector2 beforeRelativeSize = slot->GetAutoSize()
        ? FVector2(beforePixelSize.X / kViewportSize.X, beforePixelSize.Y / kViewportSize.Y)
        : slot->GetRelativeSize();

    const FVector2 resizeHandlePoint = button->GetGeometry().GetMin() + FVector2(1.0f, 1.0f);
    const FVector2 resizeTarget = resizeHandlePoint - FVector2(20.0f, 10.0f);

    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonDown, resizeHandlePoint)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseMove, resizeTarget)},
        kViewportSize);
    AdvanceAppWithDraw(
        *app,
        {MouseEvent(EInputEventType::MouseButtonUp, resizeTarget)},
        kViewportSize);

    EXPECT_NEAR(slot->GetRelativePosition().X, beforePosition.X - 0.05f, 0.0001f);
    EXPECT_NEAR(slot->GetRelativePosition().Y, beforePosition.Y - (10.0f / kViewportSize.Y), 0.0001f);
    EXPECT_NEAR(slot->GetRelativeSize().X, beforeRelativeSize.X + 0.05f, 0.0001f);
    EXPECT_NEAR(slot->GetRelativeSize().Y, beforeRelativeSize.Y + (10.0f / kViewportSize.Y), 0.0001f);
    EXPECT_FALSE(slot->GetAutoSize());
    EXPECT_TRUE(session->CanUndo());

    ASSERT_TRUE(session->Undo());
    EXPECT_NEAR(slot->GetRelativePosition().X, beforePosition.X, 0.0001f);
    EXPECT_NEAR(slot->GetRelativePosition().Y, beforePosition.Y, 0.0001f);
}

TEST(EditorSelectionTest, WorkspaceControllerOpenDocumentFromPathDeduplicatesAndActivatesExistingTab)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    auto seedSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(seedSession);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path filePath = tempDirectory / "dedupe-open.ui.json";
    std::error_code removeError;
    std::filesystem::remove(filePath, removeError);

    ASSERT_TRUE(seedSession->GetDocument());
    ASSERT_TRUE(seedSession->GetDocument()->SaveAs(filePath));

    ASSERT_EQ(workspaceController->GetDocumentCount(), 1);
    ASSERT_EQ(workspaceController->GetActiveDocumentIndex(), 0);

    ASSERT_TRUE(workspaceController->NewDocument());
    ASSERT_EQ(workspaceController->GetDocumentCount(), 2);
    ASSERT_EQ(workspaceController->GetActiveDocumentIndex(), 1);
    ASSERT_EQ(documentTabs->GetTabCount(), 2);

    ASSERT_TRUE(workspaceController->OpenDocumentFromPath(filePath));
    EXPECT_EQ(workspaceController->GetDocumentCount(), 2);
    EXPECT_EQ(workspaceController->GetActiveDocumentIndex(), 0);
    EXPECT_EQ(documentTabs->GetActiveTabIndex(), 0);
    ASSERT_TRUE(workspaceController->GetActiveSession());
    ASSERT_TRUE(workspaceController->GetActiveSession()->GetDocument());
    EXPECT_EQ(
        workspaceController->GetActiveSession()->GetDocument()->GetFilePath().lexically_normal(),
        filePath.lexically_normal());

    std::filesystem::remove(filePath, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerCloseActiveDocumentPromotesRemainingTab)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    auto app = std::make_shared<ImApplication>();
    auto rootHost = std::make_shared<ImVerticalBox>();
    rootHost->AddChild(shellHost);
    rootHost->AddChild(documentTabs);
    rootHost->AddChild(projectView);
    rootHost->AddChild(widgetTreeView);
    rootHost->AddChild(detailsView);
    rootHost->AddChild(outputText);
    app->SetRootWidget(rootHost);

    ASSERT_EQ(workspaceController->GetDocumentCount(), 1);
    ASSERT_TRUE(workspaceController->NewDocument());
    ASSERT_EQ(workspaceController->GetDocumentCount(), 2);
    ASSERT_EQ(workspaceController->GetActiveDocumentIndex(), 1);
    ASSERT_EQ(documentTabs->GetActiveTabIndex(), 1);

    ASSERT_TRUE(workspaceController->CloseActiveDocument(*app));
    ASSERT_EQ(workspaceController->GetDocumentCount(), 1);
    ASSERT_EQ(workspaceController->GetActiveDocumentIndex(), 0);
    ASSERT_EQ(documentTabs->GetTabCount(), 1);
    ASSERT_EQ(documentTabs->GetActiveTabIndex(), 0);
    ASSERT_TRUE(workspaceController->GetActiveSession());
}

TEST(EditorSelectionTest, WorkspaceControllerCreateAndOpenDocumentAtPathAddsSavedSession)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path filePath = tempDirectory / "created-from-workspace.ui.json";
    std::error_code removeError;
    std::filesystem::remove(filePath, removeError);

    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(filePath));
    EXPECT_TRUE(std::filesystem::exists(filePath));
    EXPECT_EQ(workspaceController->GetDocumentCount(), 2);
    EXPECT_EQ(workspaceController->GetActiveDocumentIndex(), 1);
    EXPECT_EQ(documentTabs->GetTabCount(), 2);
    EXPECT_EQ(documentTabs->GetActiveTabIndex(), 1);

    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    EXPECT_TRUE(activeSession->GetDocument()->HasFilePath());
    EXPECT_EQ(activeSession->GetDocument()->GetFilePath().lexically_normal(), filePath.lexically_normal());
    EXPECT_FALSE(activeSession->GetDocument()->IsDirty());

    std::filesystem::remove(filePath, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerRequestApplicationCloseBlocksWhenDirtyDocumentsExist)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    auto app = std::make_shared<ImApplication>();
    bool bExitCalled = false;
    workspaceController->SetOnExitRequested([&bExitCalled]() {
        bExitCalled = true;
    });

    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(true);

    EXPECT_FALSE(workspaceController->RequestApplicationClose(*app));
    EXPECT_FALSE(bExitCalled);
}

TEST(EditorSelectionTest, WorkspaceControllerRequestProjectRootChangeBlocksWhenDirtyDocumentsExist)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    auto app = std::make_shared<ImApplication>();
    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(true);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path projectRoot = tempDirectory / "DirtyProjectRoot";
    std::filesystem::create_directories(projectRoot);

    EXPECT_FALSE(workspaceController->RequestProjectRootChange(*app, projectRoot));
    EXPECT_NE(workspaceController->GetProjectRoot().lexically_normal(), projectRoot.lexically_normal());

    std::error_code removeError;
    std::filesystem::remove(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerWorkspaceStateRoundTripRestoresProjectAndDocuments)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    std::filesystem::create_directories(tempDirectory);
    const std::filesystem::path projectRoot = tempDirectory / "WorkspaceRoot";
    std::filesystem::create_directories(projectRoot);
    const std::filesystem::path firstFile = projectRoot / "first.ui.json";
    const std::filesystem::path secondFile = projectRoot / "second.ui.json";
    const std::filesystem::path workspaceStateFile = tempDirectory / "workspace-state.json";
    std::error_code removeError;
    std::filesystem::remove(firstFile, removeError);
    std::filesystem::remove(secondFile, removeError);
    std::filesystem::remove(workspaceStateFile, removeError);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(firstFile));
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(secondFile));
    ASSERT_TRUE(workspaceController->ActivateDocumentAt(2));
    ASSERT_TRUE(workspaceController->SaveWorkspaceState(workspaceStateFile));

    auto restoredShellHost = std::make_shared<EditorShellHost>();
    auto restoredDocumentTabs = std::make_shared<ImTabView>();
    auto restoredProjectView = std::make_shared<ImTextOutlineView>();
    auto restoredWidgetTreeView = std::make_shared<ImTextOutlineView>();
    auto restoredDetailsView = std::make_shared<ReflectionDetailsView>();
    auto restoredOutputText = std::make_shared<ImTextList>();
    auto restoredWorkspaceController = CreateBoundWorkspaceController(
        restoredShellHost,
        restoredDocumentTabs,
        restoredProjectView,
        restoredWidgetTreeView,
        restoredDetailsView,
        restoredOutputText);

    ASSERT_TRUE(restoredWorkspaceController->LoadWorkspaceState(workspaceStateFile));
    std::error_code canonicalError;
    const std::filesystem::path expectedProjectRoot =
        std::filesystem::weakly_canonical(projectRoot, canonicalError);
    canonicalError.clear();
    const std::filesystem::path restoredProjectRoot =
        std::filesystem::weakly_canonical(restoredWorkspaceController->GetProjectRoot(), canonicalError);
    EXPECT_EQ(restoredProjectRoot, expectedProjectRoot);
    EXPECT_EQ(restoredWorkspaceController->GetDocumentCount(), 2);
    EXPECT_EQ(restoredWorkspaceController->GetActiveDocumentIndex(), 1);
    ASSERT_TRUE(restoredWorkspaceController->GetActiveSession());
    ASSERT_TRUE(restoredWorkspaceController->GetActiveSession()->GetDocument());
    EXPECT_EQ(
        restoredWorkspaceController->GetActiveSession()->GetDocument()->GetFilePath().lexically_normal(),
        secondFile.lexically_normal());

    std::filesystem::remove(firstFile, removeError);
    std::filesystem::remove(secondFile, removeError);
    std::filesystem::remove(workspaceStateFile, removeError);
    std::filesystem::remove(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerCreateFolderAtPathCreatesDirectory)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    const std::filesystem::path projectRoot = tempDirectory / "FolderCreateRoot";
    const std::filesystem::path folderPath = projectRoot / "NewFolder";
    std::error_code removeError;
    std::filesystem::remove_all(projectRoot, removeError);
    std::filesystem::create_directories(projectRoot);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateFolderAtPath(folderPath));
    EXPECT_TRUE(std::filesystem::is_directory(folderPath));

    std::filesystem::remove_all(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerDeleteProjectItemBlocksDirtyOpenDocument)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    const std::filesystem::path projectRoot = tempDirectory / "DeleteBlockedRoot";
    const std::filesystem::path filePath = projectRoot / "blocked.ui.json";
    std::error_code removeError;
    std::filesystem::remove_all(projectRoot, removeError);
    std::filesystem::create_directories(projectRoot);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(filePath));
    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(true);

    EXPECT_FALSE(workspaceController->DeleteProjectItem(filePath));
    EXPECT_TRUE(std::filesystem::exists(filePath));
    EXPECT_EQ(workspaceController->GetDocumentCount(), 2);

    std::filesystem::remove_all(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerDeleteProjectItemClosesOpenDocumentAndDeletesFile)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    const std::filesystem::path projectRoot = tempDirectory / "DeleteSuccessRoot";
    const std::filesystem::path filePath = projectRoot / "delete-me.ui.json";
    std::error_code removeError;
    std::filesystem::remove_all(projectRoot, removeError);
    std::filesystem::create_directories(projectRoot);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(filePath));
    ASSERT_EQ(workspaceController->GetDocumentCount(), 2);

    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(false);

    EXPECT_TRUE(workspaceController->DeleteProjectItem(filePath));
    EXPECT_FALSE(std::filesystem::exists(filePath));
    EXPECT_EQ(workspaceController->GetDocumentCount(), 1);

    std::filesystem::remove_all(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerRenameProjectItemRenamesFileAndUpdatesOpenDocumentPath)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    const std::filesystem::path projectRoot = tempDirectory / "RenameFileRoot";
    const std::filesystem::path oldPath = projectRoot / "before.ui.json";
    const std::filesystem::path newPath = projectRoot / "after.ui.json";
    std::error_code removeError;
    std::filesystem::remove_all(projectRoot, removeError);
    std::filesystem::create_directories(projectRoot);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(oldPath));
    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(false);

    ASSERT_TRUE(workspaceController->RenameProjectItem(oldPath, "after.ui.json"));
    EXPECT_FALSE(std::filesystem::exists(oldPath));
    EXPECT_TRUE(std::filesystem::exists(newPath));
    EXPECT_EQ(activeSession->GetDocument()->GetFilePath().lexically_normal(), newPath.lexically_normal());

    std::filesystem::remove_all(projectRoot, removeError);
}

TEST(EditorSelectionTest, WorkspaceControllerRenameProjectItemRenamesFolderAndUpdatesContainedOpenDocumentPath)
{
    auto shellHost = std::make_shared<EditorShellHost>();
    auto documentTabs = std::make_shared<ImTabView>();
    auto projectView = std::make_shared<ImTextOutlineView>();
    auto widgetTreeView = std::make_shared<ImTextOutlineView>();
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    auto outputText = std::make_shared<ImTextList>();
    auto workspaceController = CreateBoundWorkspaceController(
        shellHost,
        documentTabs,
        projectView,
        widgetTreeView,
        detailsView,
        outputText);

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "ImWidgetV4EditorTests";
    const std::filesystem::path projectRoot = tempDirectory / "RenameFolderRoot";
    const std::filesystem::path oldFolder = projectRoot / "FolderA";
    const std::filesystem::path newFolder = projectRoot / "FolderB";
    const std::filesystem::path oldFile = oldFolder / "nested.ui.json";
    const std::filesystem::path newFile = newFolder / "nested.ui.json";
    std::error_code removeError;
    std::filesystem::remove_all(projectRoot, removeError);
    std::filesystem::create_directories(oldFolder);

    workspaceController->SetProjectRoot(projectRoot);
    ASSERT_TRUE(workspaceController->CreateAndOpenDocumentAtPath(oldFile));
    auto activeSession = workspaceController->GetActiveSession();
    ASSERT_TRUE(activeSession);
    ASSERT_TRUE(activeSession->GetDocument());
    activeSession->GetDocument()->SetDirty(false);

    ASSERT_TRUE(workspaceController->RenameProjectItem(oldFolder, "FolderB"));
    EXPECT_FALSE(std::filesystem::exists(oldFolder));
    EXPECT_TRUE(std::filesystem::exists(newFolder));
    EXPECT_TRUE(std::filesystem::exists(newFile));
    EXPECT_EQ(activeSession->GetDocument()->GetFilePath().lexically_normal(), newFile.lexically_normal());

    std::filesystem::remove_all(projectRoot, removeError);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
