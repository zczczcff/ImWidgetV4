#include "../src/codegen/WidgetTreeToCppGenerator.h"
#include "../src/editor/EditorSession.h"
#include "../src/serialization/WidgetSerializer.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

std::shared_ptr<ImWidget> BuildSimpleGeneratedRoot()
{
    auto root = std::make_shared<ImVerticalBox>();
    root->SetName("RootPanel");

    auto button = std::make_shared<ImButton>();
    button->SetName("ActionButton");

    auto label = std::make_shared<ImTextBlock>();
    label->SetName("ButtonLabel");
    label->SetText("Click Me");
    button->SetContent(label);

    root->AddChild(button);
    return root;
}

std::shared_ptr<ImWidget> BuildTabGeneratedRoot()
{
    auto tabs = std::make_shared<ImTabView>();
    tabs->SetName("DocumentTabs");

    auto first = std::make_shared<ImTextBlock>();
    first->SetName("FirstDocument");
    first->SetText("First");

    auto secondHost = std::make_shared<ImScrollBox>();
    secondHost->SetName("SecondHost");
    auto secondLabel = std::make_shared<ImTextBlock>();
    secondLabel->SetName("SecondLabel");
    secondLabel->SetText("Second");
    secondHost->SetContent(secondLabel);

    const int firstIndex = tabs->AddTab("First", first);
    tabs->SetTabClosable(firstIndex, true);
    tabs->SetTabDirty(firstIndex, true);

    const int secondIndex = tabs->AddTab("Second", secondHost);
    tabs->SetActiveTab(secondIndex);

    auto archive = std::make_shared<ImTextBlock>();
    archive->SetName("ArchiveDocument");
    archive->SetText("Archive");

    const int thirdIndex = tabs->AddTab("Archive", archive);
    tabs->SetTabEnabled(thirdIndex, false);

    return tabs;
}

std::shared_ptr<ImWidget> BuildCanvasGeneratedRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("CanvasRoot");

    auto child = std::make_shared<ImTextBlock>();
    child->SetName("FloatingLabel");
    child->SetText("Floating");

    canvas->AddChildAt(child, FVector2(0.25f, 0.50f));
    return canvas;
}

std::shared_ptr<ImWidget> BuildTitleBarGeneratedRoot()
{
    auto titleBar = std::make_shared<ImTitleBar>();
    titleBar->SetName("RootTitleBar");

    auto title = std::make_shared<ImTextBlock>();
    title->SetName("TitleText");
    title->SetText("Project");
    titleBar->AddLeadingItem(title);

    auto action = std::make_shared<ImButton>();
    action->SetName("ActionButton");
    action->SetText("Run");
    titleBar->AddTrailingItem(action);
    return titleBar;
}

std::shared_ptr<ImWidget> BuildVerticalSplitterGeneratedRoot()
{
    auto splitter = std::make_shared<ImVerticalSplitter>();
    splitter->SetName("RootSplitter");

    auto top = std::make_shared<ImTextBlock>();
    top->SetName("TopPanel");
    top->SetText("Top");
    splitter->AddPart(top, 2.2f, 320.0f, FMargin(18.0f, 18.0f, 16.0f, 12.0f));

    auto bottom = std::make_shared<ImTextBlock>();
    bottom->SetName("BottomPanel");
    bottom->SetText("Bottom");
    splitter->AddPart(bottom, 1.0f, 140.0f, FMargin(18.0f, 18.0f, 12.0f, 18.0f));
    return splitter;
}

void ExpectNoTrailingWhitespace(const std::string& text)
{
    std::size_t lineStart = 0;
    while (lineStart < text.size()) {
        std::size_t lineEnd = text.find('\n', lineStart);
        if (lineEnd == std::string::npos) {
            lineEnd = text.size();
        }

        std::size_t contentEnd = lineEnd;
        if (contentEnd > lineStart && text[contentEnd - 1] == '\r') {
            --contentEnd;
        }

        if (contentEnd > lineStart) {
            const char last = text[contentEnd - 1];
            EXPECT_NE(last, ' ');
            EXPECT_NE(last, '\t');
        }

        if (lineEnd == text.size()) {
            break;
        }
        lineStart = lineEnd + 1;
    }
}

} // namespace

TEST(EditorCodeGenTest, GeneratesHeaderAndSourceForSimpleWidgetTree)
{
    FCodeGenOptions options;
    options.ClassName = "GeneratedToolbar";
    options.Namespace = "ImWidgetV4Editor::Generated";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildSimpleGeneratedRoot(), options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_EQ(result.Files.HeaderFileName, "GeneratedToolbar.h");
    EXPECT_EQ(result.Files.SourceFileName, "GeneratedToolbar.cpp");
    EXPECT_NE(
        result.Files.HeaderText.find(
            "class GeneratedToolbar : public ImWidgetV4::ImUserWidget"),
        std::string::npos);
    EXPECT_NE(
        result.Files.HeaderText.find(
            "std::shared_ptr<ImWidgetV4::ImVerticalBox> RootPanel;"),
        std::string::npos);
    EXPECT_LT(
        result.Files.HeaderText.find("std::shared_ptr<ImWidgetV4::ImVerticalBox> RootPanel;"),
        result.Files.HeaderText.find("protected:"));
    EXPECT_NE(
        result.Files.SourceText.find("ActionButton->SetContent(ButtonLabel);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("RootPanel->AddChild(ActionButton);"),
        std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("return RootPanel;"), std::string::npos);
    ExpectNoTrailingWhitespace(result.Files.HeaderText);
    ExpectNoTrailingWhitespace(result.Files.SourceText);
}

TEST(EditorCodeGenTest, CanGeneratePrivateMembersFromCodegenMetadata)
{
    json rootJson = WidgetSerializer::SerializeWidgetTree(BuildSimpleGeneratedRoot());
    ASSERT_TRUE(rootJson.contains("Children"));
    ASSERT_FALSE(rootJson["Children"].empty());
    rootJson["Children"][0][kEditorCodegenFieldName][kEditorCodegenMemberAccessFieldName] =
        kEditorCodegenMemberAccessPrivate;

    FCodeGenOptions options;
    options.ClassName = "GeneratedToolbar";

    const FCodeGenResult result = WidgetTreeToCppGenerator::Generate(rootJson, options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    const std::size_t publicSection = result.Files.HeaderText.find("public:");
    const std::size_t protectedSection = result.Files.HeaderText.find("protected:");
    const std::size_t privateSection = result.Files.HeaderText.find("private:");
    const std::size_t rootMember =
        result.Files.HeaderText.find("std::shared_ptr<ImWidgetV4::ImVerticalBox> RootPanel;");
    const std::size_t actionMember =
        result.Files.HeaderText.find("std::shared_ptr<ImWidgetV4::ImButton> ActionButton;");
    const std::size_t labelMember =
        result.Files.HeaderText.find("std::shared_ptr<ImWidgetV4::ImTextBlock> ButtonLabel;");

    ASSERT_NE(publicSection, std::string::npos);
    ASSERT_NE(protectedSection, std::string::npos);
    ASSERT_NE(privateSection, std::string::npos);
    ASSERT_NE(rootMember, std::string::npos);
    ASSERT_NE(actionMember, std::string::npos);
    ASSERT_NE(labelMember, std::string::npos);
    EXPECT_LT(publicSection, rootMember);
    EXPECT_LT(rootMember, protectedSection);
    EXPECT_LT(publicSection, labelMember);
    EXPECT_LT(labelMember, protectedSection);
    EXPECT_LT(privateSection, actionMember);
}

TEST(EditorCodeGenTest, GeneratesTabSpecificRebuildLogic)
{
    FCodeGenOptions options;
    options.ClassName = "GeneratedWorkspace";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildTabGeneratedRoot(), options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs->AddTab(\"First\", FirstDocument);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs->SetTabClosable(FirstDocumentTabIndex, true);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs->SetTabDirty(FirstDocumentTabIndex, true);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs->SetTabEnabled(ArchiveDocumentTabIndex, false);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("SecondHost->SetContent(SecondLabel);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs->SetActiveTab(1);"),
        std::string::npos);
}

TEST(EditorCodeGenTest, GeneratesSlotRestorationForPanelChildren)
{
    FCodeGenOptions options;
    options.ClassName = "GeneratedCanvas";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildCanvasGeneratedRoot(), options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_NE(
        result.Files.SourceText.find("CanvasRoot->AddChild(FloatingLabel);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("slot->FromJson(ParseGeneratedJson("),
        std::string::npos);
    EXPECT_EQ(result.Files.SourceText.find("ImSlot::SlotPosition"), std::string::npos);
    EXPECT_EQ(result.Files.SourceText.find("ImSlot::SlotSize"), std::string::npos);
}

TEST(EditorCodeGenTest, FiltersRuntimeSlotPropertiesFromRawJson)
{
    json rootJson;
    rootJson["Type"] = "ImCanvasPanel";
    rootJson["Properties"] = {
        {"ImWidget::Name", "CanvasRoot"}
    };
    rootJson["Children"] = json::array();

    json childJson;
    childJson["Type"] = "ImTextBlock";
    childJson["Properties"] = {
        {"ImWidget::Name", "FloatingLabel"},
        {"ImTextBlock::Text", "Floating"}
    };
    childJson["Children"] = json::array();
    childJson["Slot"] = {
        {"Type", "ImCanvasPanelSlot"},
        {"Properties", {
            {"ImSlot::SlotPosition", {10.0f, 20.0f}},
            {"ImSlot::SlotSize", {200.0f, 80.0f}},
            {"ImCanvasPanelSlot::RelativePosition", {0.25f, 0.50f}},
            {"ImCanvasPanelSlot::RelativeSize", {0.40f, 0.20f}},
            {"ImCanvasPanelSlot::AutoSize", false}
        }}
    };
    rootJson["Children"].push_back(childJson);

    FCodeGenOptions options;
    options.ClassName = "GeneratedCanvas";

    const FCodeGenResult result = WidgetTreeToCppGenerator::Generate(rootJson, options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_EQ(result.Files.SourceText.find("ImSlot::SlotPosition"), std::string::npos);
    EXPECT_EQ(result.Files.SourceText.find("ImSlot::SlotSize"), std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("ImCanvasPanelSlot::RelativePosition"), std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("ImCanvasPanelSlot::RelativeSize"), std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("ImCanvasPanelSlot::AutoSize"), std::string::npos);
}

TEST(EditorCodeGenTest, PreservesAuthoredJsonWithoutExpandingDefaultStyle)
{
    json rootJson;
    rootJson["Type"] = "ImCheckBox";
    rootJson["Properties"] = {
        {"ImCheckBox::Label", "Build SDK"},
        {"ImCheckBox::Checked", true},
        {"ImCheckBox::Disabled", false},
        {"ImWidget::Name", "BuildSdkCheckBox"},
        {"ImWidget::Visible", true},
        {"ImWidget::HitTestVisible", true},
        {"ImWidget::SupportsKeyboardFocus", true}
    };
    rootJson["Children"] = json::array();

    FCodeGenOptions options;
    options.ClassName = "GeneratedOptions";

    const FCodeGenResult result = WidgetTreeToCppGenerator::Generate(rootJson, options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_NE(result.Files.SourceText.find("BuildSdkCheckBox->FromJson"), std::string::npos);
    EXPECT_EQ(result.Files.SourceText.find("ImCheckBox::Style"), std::string::npos);
    ExpectNoTrailingWhitespace(result.Files.SourceText);
}

TEST(EditorCodeGenTest, GeneratesTitleBarLeadingAndTrailingItems)
{
    FCodeGenOptions options;
    options.ClassName = "GeneratedTitleBar";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildTitleBarGeneratedRoot(), options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_NE(result.Files.SourceText.find("RootTitleBar->AddLeadingItem(TitleText);"), std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("RootTitleBar->AddTrailingItem(ActionButton);"), std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("#include <imwidgetv4/widgets/TitleBar.h>"), std::string::npos);
}

TEST(EditorCodeGenTest, SerializesTitleBarLeadingAndTrailingItems)
{
    const auto root = BuildTitleBarGeneratedRoot();
    const json serialized = WidgetSerializer::SerializeWidgetTree(root);
    ASSERT_TRUE(serialized.contains("LeadingItems"));
    ASSERT_TRUE(serialized.contains("TrailingItems"));
    ASSERT_EQ(serialized.at("LeadingItems").size(), 1U);
    ASSERT_EQ(serialized.at("TrailingItems").size(), 1U);

    const FWidgetSerializationResult result = WidgetSerializer::DeserializeWidgetTree(serialized);
    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    auto restoredTitleBar = std::dynamic_pointer_cast<ImTitleBar>(result.Widget);
    ASSERT_TRUE(restoredTitleBar);
    EXPECT_EQ(restoredTitleBar->GetLeadingItemCount(), 1U);
    EXPECT_EQ(restoredTitleBar->GetTrailingItemCount(), 1U);
    ASSERT_TRUE(std::dynamic_pointer_cast<ImTextBlock>(restoredTitleBar->GetLeadingItemAt(0)));
    ASSERT_TRUE(std::dynamic_pointer_cast<ImButton>(restoredTitleBar->GetTrailingItemAt(0)));
}

TEST(EditorCodeGenTest, DeserializesVerticalSplitterSlots)
{
    const auto root = BuildVerticalSplitterGeneratedRoot();
    const json serialized = WidgetSerializer::SerializeWidgetTree(root);

    const FWidgetSerializationResult result = WidgetSerializer::DeserializeWidgetTree(serialized);
    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;

    auto restoredSplitter = std::dynamic_pointer_cast<ImVerticalSplitter>(result.Widget);
    ASSERT_TRUE(restoredSplitter);
    ASSERT_EQ(restoredSplitter->GetSlotCount(), 2);

    auto* topSlot = dynamic_cast<ImVerticalSplitterSlot*>(restoredSplitter->GetSlotAt(0));
    auto* bottomSlot = dynamic_cast<ImVerticalSplitterSlot*>(restoredSplitter->GetSlotAt(1));
    ASSERT_NE(topSlot, nullptr);
    ASSERT_NE(bottomSlot, nullptr);
    EXPECT_FLOAT_EQ(topSlot->GetRatio(), 2.2f);
    EXPECT_FLOAT_EQ(topSlot->GetMinSize(), 320.0f);
    EXPECT_FLOAT_EQ(bottomSlot->GetRatio(), 1.0f);
    EXPECT_FLOAT_EQ(bottomSlot->GetMinSize(), 140.0f);
}

TEST(EditorCodeGenTest, RejectsInvalidClassName)
{
    FCodeGenOptions options;
    options.ClassName = "123Broken";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildSimpleGeneratedRoot(), options);

    EXPECT_FALSE(result.bSuccess);
    EXPECT_FALSE(result.ErrorMessage.empty());
}

TEST(EditorCodeGenTest, SessionCanWriteGeneratedHeaderAndSourceFiles)
{
    EditorSession session([]() { return BuildSimpleGeneratedRoot(); });

    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path() / "imwidgetv4_editor_codegen_test";
    std::error_code errorCode;
    std::filesystem::remove_all(tempDirectory, errorCode);
    std::filesystem::create_directories(tempDirectory, errorCode);
    ASSERT_FALSE(errorCode);

    const std::filesystem::path headerPath = tempDirectory / "GeneratedPreview.h";
    const std::filesystem::path sourcePath = tempDirectory / "GeneratedPreview.cpp";

    ASSERT_TRUE(session.GenerateCppFilesAt(headerPath));
    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::ifstream headerStream(headerPath, std::ios::binary);
    std::ifstream sourceStream(sourcePath, std::ios::binary);
    ASSERT_TRUE(headerStream.is_open());
    ASSERT_TRUE(sourceStream.is_open());

    const std::string headerText(
        (std::istreambuf_iterator<char>(headerStream)),
        std::istreambuf_iterator<char>());
    const std::string sourceText(
        (std::istreambuf_iterator<char>(sourceStream)),
        std::istreambuf_iterator<char>());

    EXPECT_NE(headerText.find("class GeneratedPreview"), std::string::npos);
    EXPECT_NE(headerText.find("std::shared_ptr<ImWidgetV4::ImVerticalBox> RootPanel;"), std::string::npos);
    EXPECT_NE(sourceText.find("ActionButton->SetContent(ButtonLabel);"), std::string::npos);
    EXPECT_NE(sourceText.find("return RootPanel;"), std::string::npos);

    std::filesystem::remove_all(tempDirectory, errorCode);
}

TEST(EditorCodeGenTest, EditorDocumentPersistsCodegenMemberAccessMetadata)
{
    EditorDocument document;
    document.NewDocument(BuildSimpleGeneratedRoot(), "Main");
    const std::shared_ptr<ImWidget> root = document.GetRootWidget();
    ASSERT_TRUE(root);
    ASSERT_FALSE(root->GetChildren().empty());
    const std::shared_ptr<ImWidget> actionButton = root->GetChildren().front();

    json metadata;
    metadata[kEditorCodegenMemberAccessFieldName] = kEditorCodegenMemberAccessPrivate;
    document.SetWidgetCodegenMetadata(actionButton, metadata);

    const json documentJson = document.ExportDocumentJson();
    ASSERT_TRUE(documentJson.contains("RootWidget"));
    ASSERT_TRUE(documentJson["RootWidget"].contains("Children"));
    ASSERT_FALSE(documentJson["RootWidget"]["Children"].empty());
    EXPECT_EQ(
        documentJson["RootWidget"]["Children"][0][kEditorCodegenFieldName][kEditorCodegenMemberAccessFieldName],
        kEditorCodegenMemberAccessPrivate);

    EditorDocument restored;
    std::string error;
    ASSERT_TRUE(restored.ImportDocumentJson(documentJson, &error)) << error;
    const std::shared_ptr<ImWidget> restoredRoot = restored.GetRootWidget();
    ASSERT_TRUE(restoredRoot);
    ASSERT_FALSE(restoredRoot->GetChildren().empty());
    const json restoredMetadata = restored.GetWidgetCodegenMetadata(restoredRoot->GetChildren().front());
    EXPECT_EQ(
        restoredMetadata.value(kEditorCodegenMemberAccessFieldName, std::string()),
        kEditorCodegenMemberAccessPrivate);
}
