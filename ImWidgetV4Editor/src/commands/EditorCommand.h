#pragma once

#include <string>

namespace ImWidgetV4Editor {

class EditorCommand {
public:
    explicit EditorCommand(std::string label)
        : m_Label(std::move(label))
    {
    }

    virtual ~EditorCommand() = default;

    const std::string& GetLabel() const { return m_Label; }

    virtual bool Execute() = 0;
    virtual bool Undo() = 0;

private:
    std::string m_Label;
};

} // namespace ImWidgetV4Editor
