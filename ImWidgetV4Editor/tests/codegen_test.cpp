#include "../src/codegen/WidgetTreeToCppGenerator.h"

#include <gtest/gtest.h>

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
            "std::shared_ptr<ImWidgetV4::ImVerticalBox> RootPanel_;"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("ActionButton_->SetContent(ButtonLabel_);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("RootPanel_->AddChild(ActionButton_);"),
        std::string::npos);
    EXPECT_NE(result.Files.SourceText.find("return RootPanel_;"), std::string::npos);
}

TEST(EditorCodeGenTest, GeneratesTabSpecificRebuildLogic)
{
    FCodeGenOptions options;
    options.ClassName = "GeneratedWorkspace";

    const FCodeGenResult result =
        WidgetTreeToCppGenerator::Generate(BuildTabGeneratedRoot(), options);

    ASSERT_TRUE(result.bSuccess) << result.ErrorMessage;
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs_->AddTab(\"First\", FirstDocument_);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs_->SetTabClosable(FirstDocument_TabIndex, true);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs_->SetTabDirty(FirstDocument_TabIndex, true);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs_->SetTabEnabled(ArchiveDocument_TabIndex, false);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("SecondHost_->SetContent(SecondLabel_);"),
        std::string::npos);
    EXPECT_NE(
        result.Files.SourceText.find("DocumentTabs_->SetActiveTab(1);"),
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
        result.Files.SourceText.find("CanvasRoot_->AddChild(FloatingLabel_);"),
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
