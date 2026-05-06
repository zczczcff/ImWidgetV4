#pragma once

#include "EditorCommand.h"

#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

class CommandStack {
public:
    void Clear();
    void PushExecuted(std::unique_ptr<EditorCommand> command);

    bool CanUndo() const;
    bool CanRedo() const;
    bool Undo();
    bool Redo();

    std::string GetUndoLabel() const;
    std::string GetRedoLabel() const;

private:
    std::vector<std::unique_ptr<EditorCommand>> m_Commands;
    std::size_t m_NextCommandIndex = 0;
};

} // namespace ImWidgetV4Editor
