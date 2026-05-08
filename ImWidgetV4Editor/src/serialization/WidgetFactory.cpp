#include "WidgetFactory.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

WidgetFactory& WidgetFactory::Get()
{
    static WidgetFactory instance;
    return instance;
}

WidgetFactory::WidgetFactory()
{
    RegisterDefaults();
}

void WidgetFactory::RegisterDefaults()
{
    Register("ImButton", []() { return std::make_shared<ImButton>(); });
    Register("ImCanvasPanel", []() { return std::make_shared<ImCanvasPanel>(); });
    Register("ImCheckBox", []() { return std::make_shared<ImCheckBox>(); });
    Register("ImColorPicker", []() { return std::make_shared<ImColorPicker>(); });
    Register("ImComboBox", []() { return std::make_shared<ImComboBox>(); });
    Register("ImEditableText", []() { return std::make_shared<ImEditableText>(); });
    Register("ImExpandableBox", []() { return std::make_shared<ImExpandableBox>(); });
    Register("ImHorizontalBox", []() { return std::make_shared<ImHorizontalBox>(); });
    Register("ImHorizontalSplitter", []() { return std::make_shared<ImHorizontalSplitter>(); });
    Register("ImImage", []() { return std::make_shared<ImImage>(); });
    Register("ImListView", []() { return std::make_shared<ImListView>(); });
    Register("ImOutlineView", []() { return std::make_shared<ImOutlineView>(); });
    Register("ImScrollBox", []() { return std::make_shared<ImScrollBox>(); });
    Register("ImSlider", []() { return std::make_shared<ImSlider>(); });
    Register("ImSwitch", []() { return std::make_shared<ImSwitch>(); });
    Register("ImTabView", []() { return std::make_shared<ImTabView>(); });
    Register("ImTextBlock", []() { return std::make_shared<ImTextBlock>(); });
    Register("ImTextList", []() { return std::make_shared<ImTextList>(); });
    Register("ImTextOutlineView", []() { return std::make_shared<ImTextOutlineView>(); });
    Register("ImVerticalBox", []() { return std::make_shared<ImVerticalBox>(); });
    Register("ImVerticalSplitter", []() { return std::make_shared<ImVerticalSplitter>(); });
}

void WidgetFactory::Register(const std::string& typeName, WidgetCreator creator)
{
    m_WidgetCreators[typeName] = std::move(creator);
}

std::shared_ptr<ImWidget> WidgetFactory::CreateWidget(const std::string& typeName) const
{
    const auto it = m_WidgetCreators.find(typeName);
    if (it == m_WidgetCreators.end()) {
        return nullptr;
    }

    return it->second ? it->second() : nullptr;
}

bool WidgetFactory::SupportsWidgetType(const std::string& typeName) const
{
    return m_WidgetCreators.find(typeName) != m_WidgetCreators.end();
}

} // namespace ImWidgetV4Editor
