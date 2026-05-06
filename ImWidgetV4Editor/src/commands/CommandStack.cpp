#include "CommandStack.h"

namespace ImWidgetV4Editor {

void CommandStack::Clear()
{
    m_Commands.clear();
    m_NextCommandIndex = 0;
}

void CommandStack::PushExecuted(std::unique_ptr<EditorCommand> command)
{
    if (!command) {
        return;
    }

    if (m_NextCommandIndex < m_Commands.size()) {
        m_Commands.erase(m_Commands.begin() + static_cast<std::ptrdiff_t>(m_NextCommandIndex), m_Commands.end());
    }

    m_Commands.push_back(std::move(command));
    m_NextCommandIndex = m_Commands.size();
}

bool CommandStack::CanUndo() const
{
    return m_NextCommandIndex > 0;
}

bool CommandStack::CanRedo() const
{
    return m_NextCommandIndex < m_Commands.size();
}

bool CommandStack::Undo()
{
    if (!CanUndo()) {
        return false;
    }

    std::size_t commandIndex = m_NextCommandIndex - 1;
    if (!m_Commands[commandIndex] || !m_Commands[commandIndex]->Undo()) {
        return false;
    }

    m_NextCommandIndex = commandIndex;
    return true;
}

bool CommandStack::Redo()
{
    if (!CanRedo()) {
        return false;
    }

    if (!m_Commands[m_NextCommandIndex] || !m_Commands[m_NextCommandIndex]->Execute()) {
        return false;
    }

    ++m_NextCommandIndex;
    return true;
}

std::string CommandStack::GetUndoLabel() const
{
    if (!CanUndo() || !m_Commands[m_NextCommandIndex - 1]) {
        return "";
    }

    return m_Commands[m_NextCommandIndex - 1]->GetLabel();
}

std::string CommandStack::GetRedoLabel() const
{
    if (!CanRedo() || !m_Commands[m_NextCommandIndex]) {
        return "";
    }

    return m_Commands[m_NextCommandIndex]->GetLabel();
}

} // namespace ImWidgetV4Editor
