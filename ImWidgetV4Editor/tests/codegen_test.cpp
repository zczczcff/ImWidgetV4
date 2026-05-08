#include "../src/codegen/WidgetTreeToCppGenerator.h"
#include "../src/editor/EditorSession.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>

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
    EXPECT_NE(
        result.Files.SourceText.find("ActionButton->SetContent(ButtonLabel);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("RootPanel->AddChild(ActionButton);"),
        std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("return RootPanel;"), std::string::npos);
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
