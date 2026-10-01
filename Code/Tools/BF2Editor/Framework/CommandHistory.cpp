// Code/Tools/BF2Editor/Framework/CommandHistory.cpp - see CommandHistory.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/CommandHistory.h"

namespace bf2editor
{

CommandHistory::CommandHistory(size_t limit)
    : m_position(0)
    , m_limit(limit > 0 ? limit : 1)
    , m_savedPosition(0)
    , m_revision(0)
    , m_runner(0)
    , m_context(0)
{
}

void CommandHistory::setRunner(ScriptRunner runner, void* context)
{
    m_runner = runner;
    m_context = context;
}

bool CommandHistory::run(const std::string& script, std::string& error)
{
    if (!m_runner)
    {
        error = "no console to run the command";
        return false;
    }
    return m_runner(m_context, script, error);
}

void CommandHistory::push(const HistoryEntry& entry)
{
    // A new change drops the redo tail; a save inside that tail can no longer be reached.
    if (m_position < m_entries.size())
    {
        if (m_savedPosition > static_cast<long>(m_position))
            m_savedPosition = -1;
        m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(m_position), m_entries.end());
    }
    m_entries.push_back(entry);
    ++m_position;
    // Over the limit, the oldest entry goes; a save before it can no longer be reached either.
    if (m_entries.size() > m_limit)
    {
        m_entries.erase(m_entries.begin());
        --m_position;
        if (m_savedPosition >= 0)
            m_savedPosition = m_savedPosition == 0 ? -1 : m_savedPosition - 1;
    }
    ++m_revision;
}

bool CommandHistory::execute(const HistoryEntry& entry, std::string& error)
{
    if (!run(entry.doScript, error))
        return false;
    push(entry);
    return true;
}

void CommandHistory::record(const HistoryEntry& entry)
{
    push(entry);
}

bool CommandHistory::undo(std::string& error)
{
    if (!canUndo())
    {
        error = "nothing to undo";
        return false;
    }
    // A failed undo leaves the position where it was: what the document holds is then unknown, and
    // the caller reports it rather than pretending the step was taken back.
    if (!run(m_entries[m_position - 1].undoScript, error))
        return false;
    --m_position;
    ++m_revision;
    return true;
}

bool CommandHistory::redo(std::string& error)
{
    if (!canRedo())
    {
        error = "nothing to redo";
        return false;
    }
    if (!run(m_entries[m_position].doScript, error))
        return false;
    ++m_position;
    ++m_revision;
    return true;
}

const HistoryEntry* CommandHistory::nextUndo() const
{
    return canUndo() ? &m_entries[m_position - 1] : 0;
}

const HistoryEntry* CommandHistory::nextRedo() const
{
    return canRedo() ? &m_entries[m_position] : 0;
}

void CommandHistory::clear()
{
    const bool dirty = isDirty();
    m_entries.clear();
    m_position = 0;
    m_savedPosition = dirty ? -1 : 0;
    ++m_revision;
}

bool CommandHistory::isDirty() const
{
    return m_savedPosition != static_cast<long>(m_position);
}

void CommandHistory::markSaved()
{
    m_savedPosition = static_cast<long>(m_position);
    ++m_revision;
}

} // namespace bf2editor
