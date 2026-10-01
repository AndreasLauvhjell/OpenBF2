// Code/Tools/BF2Editor/Framework/CommandHistory.h

// The undo/redo history. An entry is a pair of console scripts: the lines
// that make a change and the lines that take it back. Both run through the engine console, so every
// edit is the same .con text a level file or a script would carry.

// Engine-free: the owner supplies the runner that executes a script (Framework/EditorConsole in the
// editor, a recording fake in bf2_editor_checks).

// Dirty state comes from the history, not from UI handlers (§4): the document is clean exactly when
// the history stands where it stood at the last save. Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_COMMANDHISTORY_H
#define BF2EDITOR_FRAMEWORK_COMMANDHISTORY_H

#include "Framework/EditorConfig.h"

#include <cstddef>
#include <string>
#include <vector>

namespace bf2editor
{

struct HistoryEntry
{
    std::string label;          // for menus and the History panel, e.g. "Move dock_crane (#1234)"
    std::string doScript;       // console lines, '\n'-separated
    std::string undoScript;
};

// Runs a console script. Returns false, with a message, when a line failed; the lines after a failing
// one are not run.
typedef bool (*ScriptRunner)(void* context, const std::string& script, std::string& error);

class CommandHistory
{
public:
    explicit CommandHistory(size_t limit = 500);

    void setRunner(ScriptRunner runner, void* context);

    // Runs the entry's do script and records it. A failed run records nothing; the redo tail is only
    // dropped when the run succeeded.
    bool execute(const HistoryEntry& entry, std::string& error);
    // Records an entry whose effect has already been made - an object creation, whose undo script
    // can only be written once the engine has given the object its id.
    void record(const HistoryEntry& entry);

    bool undo(std::string& error);
    bool redo(std::string& error);
    bool canUndo() const { return m_position > 0; }
    bool canRedo() const { return m_position < m_entries.size(); }
    const HistoryEntry* nextUndo() const;   // 0 when there is nothing to undo
    const HistoryEntry* nextRedo() const;

    // Forgets every entry: something changed the document that the history cannot undo (a typed
    // console command, a reload). A dirty document stays dirty.
    void clear();

    const std::vector<HistoryEntry>& entries() const { return m_entries; }
    size_t position() const { return m_position; }      // entries [0, position) are applied
    size_t limit() const { return m_limit; }

    bool isDirty() const;
    void markSaved();

    // Changes whenever anything above does; the UI compares it to redraw.
    unsigned revision() const { return m_revision; }

private:
    bool run(const std::string& script, std::string& error);
    void push(const HistoryEntry& entry);

    std::vector<HistoryEntry> m_entries;
    size_t       m_position;
    size_t       m_limit;
    long         m_savedPosition;   // -1: the saved state can no longer be reached by undo or redo
    unsigned     m_revision;
    ScriptRunner m_runner;
    void*        m_context;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_COMMANDHISTORY_H
