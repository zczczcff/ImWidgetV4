#include "WidgetFactory.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>

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
    Register("ImEditableText", []() { return std::make_shared<ImEditableText>(); });
    Register("ImHorizontalBox", []() { return std::make_shared<ImHorizontalBox>(); });
    Register("ImImage", []() { return std::make_shared<ImImage>(); });
    Register("ImScrollBox", []() { return std::make_shared<ImScrollBox>(); });
    Register("ImSlider", []() { return std::make_shared<ImSlider>(); });
    Register("ImSwitch", []() { return std::make_shared<ImSwitch>(); });
    Register("ImTabView", []() { return std::make_shared<ImTabView>(); });
    Register("ImTextBlock", []() { return std::make_shared<ImTextBlock>(); });
    Register("ImVerticalBox", []() { return std::make_shared<ImVerticalBox>(); });
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
