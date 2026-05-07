#include <gtest/gtest.h>

#include "../src/commands/DocumentSnapshotCommand.h"
#include "../src/commands/AddWidgetCommand.h"
#include "../src/commands/MoveWidgetCommand.h"
#include "../src/commands/ReflectablePropertyCommand.h"
#include "../src/commands/RemoveWidgetCommand.h"
#include "../src/editor/EditorDocument.h"
#include "../src/editor/EditorSession.h"
#include "../src/editor/SelectionModel.h"

#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include "../src/inspector/ReflectionDetailsView.h"

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

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

    ASSERT_TRUE(session->Undo());
    ASSERT_EQ(root->GetChildren().size(), 1u);

    ASSERT_TRUE(session->Redo());
    ASSERT_EQ(root->GetChildren().size(), 2u);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
