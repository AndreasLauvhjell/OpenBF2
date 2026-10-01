// Code/Tools/BF2Editor/Framework/Selection.h

// The selection service (WP5). It holds engine object IDS (IObject +0x10, handed out by
// ObjectManager::getNewID from 1 upwards; 0 is never an object), never pointers, so it survives
// anything that deletes an object: a stale id simply finds nothing. Selecting is not an edit and is
// not recorded in the undo history. Engine-free. Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_SELECTION_H
#define BF2EDITOR_FRAMEWORK_SELECTION_H

#include "Framework/EditorConfig.h"

#include <cstddef>
#include <vector>

namespace bf2editor
{

class Selection
{
public:
    Selection();

    void clear();
    void set(unsigned id);          // exactly this object (0 clears)
    void add(unsigned id);          // also this one; it becomes the primary
    void remove(unsigned id);
    void toggle(unsigned id);

    bool contains(unsigned id) const;
    bool empty() const { return m_ids.empty(); }
    size_t size() const { return m_ids.size(); }
    // The object the gizmo and the Selection panel work on: the last one added. 0 when empty.
    unsigned primary() const { return m_ids.empty() ? 0u : m_ids.back(); }
    const std::vector<unsigned>& ids() const { return m_ids; }

    unsigned revision() const { return m_revision; }

private:
    std::vector<unsigned> m_ids;    // in the order they were added
    unsigned m_revision;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_SELECTION_H
