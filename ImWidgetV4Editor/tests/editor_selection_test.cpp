#include <gtest/gtest.h>

#include "../src/commands/DocumentSnapshotCommand.h"
#include "../src/editor/EditorDocument.h"
#include "../src/editor/EditorSession.h"
#include "../src/editor/SelectionModel.h"

#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/Button.h>
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
    session->BindDocumentWidgets(
        nullptr,
        -1,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
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

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
