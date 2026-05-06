#pragma once

#include <imwidgetv4/core/Widget.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace ImWidgetV4Editor {

class WidgetFactory {
public:
    using WidgetCreator = std::function<std::shared_ptr<ImWidgetV4::ImWidget>()>;

    static WidgetFactory& Get();

    std::shared_ptr<ImWidgetV4::ImWidget> CreateWidget(const std::string& typeName) const;
    bool SupportsWidgetType(const std::string& typeName) const;

private:
    WidgetFactory();

    void RegisterDefaults();
    void Register(const std::string& typeName, WidgetCreator creator);

    std::unordered_map<std::string, WidgetCreator> m_WidgetCreators;
};

} // namespace ImWidgetV4Editor
