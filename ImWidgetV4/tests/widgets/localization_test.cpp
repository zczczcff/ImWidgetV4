#include <gtest/gtest.h>

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>

using namespace ImWidgetV4;

namespace {

void ResetLocalization()
{
    auto& localization = FLocalizationManager::Get();
    localization.ClearStringTables();
    localization.SetDefaultCulture("en-US");
    localization.SetCulture("en-US");
}

} // namespace

TEST(LocalizationTest, TextResolvesThroughCurrentCultureAndFallback)
{
    ResetLocalization();

    FStringTable english;
    english.Culture = "en-US";
    english.Entries["Editor.File"] = "File";
    FLocalizationManager::Get().RegisterStringTable(std::move(english));

    FStringTable chinese;
    chinese.Culture = "zh-CN";
    chinese.Entries["Editor.File"] = "文件";
    FLocalizationManager::Get().RegisterStringTable(std::move(chinese));

    FText text = FText::FromKey("Editor.File", "Default File");
    EXPECT_EQ(text.Resolve(), "File");

    ASSERT_TRUE(FLocalizationManager::Get().SetCulture("zh-CN"));
    EXPECT_EQ(text.Resolve(), "文件");

    FText fallbackText = FText::FromKey("Editor.Edit", "Edit");
    EXPECT_EQ(fallbackText.Resolve(), "Edit");
}

TEST(LocalizationTest, TextBlockKeepsPlainTextSerializableAndLocalizedTextRuntimeOnly)
{
    ResetLocalization();

    FStringTable chinese;
    chinese.Culture = "zh-CN";
    chinese.Entries["Widget.Button"] = "按钮";
    FLocalizationManager::Get().RegisterStringTable(std::move(chinese));
    FLocalizationManager::Get().SetCulture("zh-CN");

    ImTextBlock textBlock;
    textBlock.SetText(FText::FromKey("Widget.Button", "Button"));

    EXPECT_EQ(textBlock.GetText(), "按钮");
    EXPECT_EQ(textBlock.ToJson()["Properties"]["ImTextBlock::Text"], "Button");

    ImTextBlock restored;
    restored.FromJson(textBlock.ToJson());
    EXPECT_EQ(restored.GetText(), "Button");
}

TEST(LocalizationTest, ButtonTextUsesLocalizedTextBlockContent)
{
    ResetLocalization();

    FStringTable chinese;
    chinese.Culture = "zh-CN";
    chinese.Entries["Command.Apply"] = "应用";
    FLocalizationManager::Get().RegisterStringTable(std::move(chinese));
    FLocalizationManager::Get().SetCulture("zh-CN");

    ImButton button;
    button.SetText(FText::FromKey("Command.Apply", "Apply"));

    EXPECT_EQ(button.GetText(), "应用");
    auto content = std::dynamic_pointer_cast<ImTextBlock>(button.GetContent());
    ASSERT_TRUE(content);
    EXPECT_EQ(content->GetText(), "应用");
}

TEST(LocalizationTest, ApplicationFacadeControlsCultureAndTables)
{
    ResetLocalization();

    ImApplication app;
    FStringTable table;
    table.Culture = "zh-CN";
    table.Entries["App.Title"] = "应用标题";
    app.RegisterStringTable(std::move(table));

    ASSERT_TRUE(app.SetCulture("zh-CN"));
    EXPECT_EQ(app.GetCulture(), "zh-CN");
    EXPECT_EQ(app.ResolveText(FText::FromKey("App.Title", "Title")), "应用标题");
}
