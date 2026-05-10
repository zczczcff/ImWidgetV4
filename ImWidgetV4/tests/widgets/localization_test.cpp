#include <gtest/gtest.h>

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/TabView.h>
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

TEST(LocalizationTest, CheckBoxLabelUsesLocalizedTextAndKeepsDefaultSerializable)
{
    ResetLocalization();

    FStringTable pseudo;
    pseudo.Culture = "pseudo";
    pseudo.Entries["Option.Enabled"] = "Enabled-Pseudo";
    FLocalizationManager::Get().RegisterStringTable(std::move(pseudo));
    FLocalizationManager::Get().SetCulture("pseudo");

    ImCheckBox checkBox;
    checkBox.SetLabel(FText::FromKey("Option.Enabled", "Enabled"));

    EXPECT_EQ(checkBox.GetLabel(), "Enabled-Pseudo");
    EXPECT_EQ(checkBox.ToJson()["Properties"]["ImCheckBox::Label"], "Enabled");

    ImCheckBox restored;
    restored.FromJson(checkBox.ToJson());
    EXPECT_EQ(restored.GetLabel(), "Enabled");
}

TEST(LocalizationTest, TabViewTitleUsesLocalizedTextAndKeepsDefaultTitle)
{
    ResetLocalization();

    FStringTable pseudo;
    pseudo.Culture = "pseudo";
    pseudo.Entries["Tab.Settings"] = "Settings-Pseudo";
    FLocalizationManager::Get().RegisterStringTable(std::move(pseudo));
    FLocalizationManager::Get().SetCulture("pseudo");

    auto content = std::make_shared<ImTextBlock>();
    ImTabView tabView;
    const int tabIndex = tabView.AddTab(FText::FromKey("Tab.Settings", "Settings"), content);

    ASSERT_EQ(tabIndex, 0);
    ASSERT_NE(tabView.GetTab(0), nullptr);
    EXPECT_EQ(tabView.GetTab(0)->Title, "Settings");
    EXPECT_EQ(tabView.GetTab(0)->TitleText.Resolve(), "Settings-Pseudo");

    ASSERT_TRUE(tabView.SetTabTitle(0, FText::FromKey("Tab.Settings", "Settings")));
    EXPECT_EQ(tabView.GetTab(0)->TitleText.Resolve(), "Settings-Pseudo");
}

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
