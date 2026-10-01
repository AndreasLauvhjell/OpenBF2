// Path:     [INFERRED] - see ActionBuffer.h.
// Contents: recovered from bf2.i64 (ELF, symbolised). All ActionBuffer bodies are written.

#include "ActionBuffer.h"
#include "../../IO/Input/PlayerInput.h"

namespace dice {
namespace hfe {

// Starts with an empty queue and a neutral current action built through PlayerAction::set.
// The temporary input's trailing padding is intentionally left uninitialized.
ActionBuffer::ActionBuffer()
    : m_lastInputIndex(0)
    , m_currentInputIndex(-1)
    , m_replayCount(0)
{
    // clear resets both initial values; preserve the original initialization order.
    clear(0);

    // The -1 goes in before set() runs, not after. The recovered 782-byte set() body writes the
    // axes, button masks and flag but leaves m_inputIndex untouched; the shipped order is preserved.
    m_current.m_inputIndex = -1;

    io::PlayerInput input;
    for (int i = 0; i < 64; ++i)
        input.inputs[i] = 0.0f;
    input.mappingFlags = 0;
    input.dword0 = 0;
    input.dword1 = 0;
    input.flag     = 0;
    // input.byte1/byte112/byte113 deliberately NOT touched - see the header block.

    m_current.set(input);
}

// Drops history and reseeds one neutral action at index. Clears the input fields
// but retains PlayerInput's final three bytes, matching constructor initialization.
void ActionBuffer::clear(int index)
{
    m_entries.clear();

    m_lastInputIndex       = index;
    m_currentInputIndex    = index;
    m_current.m_inputIndex = index;

    io::PlayerInput input;
    input.mappingFlags = 0;
    input.dword0 = 0;
    input.dword1 = 0;
    input.flag     = 0;
    for (int i = 0; i < 64; ++i)
        input.inputs[i] = 0.0f;

    m_current.set(input);
}

// Copies an action into the queue and adopts its input index. Does not trim entries;
// callers must remove old actions or clear the buffer.
bool ActionBuffer::pushBack(PlayerAction& action)
{
    ActionBufferEntry entry;
    entry.m_action = action;                    // by value - see above

    m_entries.insert(m_entries.end(), entry);

    m_lastInputIndex = action.m_inputIndex;     // the stamp, not a count
    return true;
}

// libstdc++ 3.x's std::list caches no size, so the ELF walks the node chain to a count - 46 bytes of
// pointer-chasing rather than a load (see the header block). The source operation is size();
// reproduced as such - the current STL's O(1) size() is the same source call, just cheaper.
int ActionBuffer::size() const
{
    return static_cast<int>(m_entries.size());
}

// Copies an action, assigns the buffer's next input index, and returns the added
// entry. Unlike pushBack, it replaces the supplied index and advances the counter.
ActionBufferEntry* ActionBuffer::add(const PlayerAction& action)
{
    ActionBufferEntry entry;
    entry.m_action = action;                          // copy all 28 bytes...
    entry.m_action.m_inputIndex = m_lastInputIndex;   // ...then overwrite the index with our counter
    ++m_lastInputIndex;

    m_entries.insert(m_entries.end(), entry);
    // Returns the stored, stamped action rather than the caller's input.
    return &m_entries.back();
}

// Drops entries from the front while their m_inputIndex is not `index`, stopping AT the entry stamped
// `index` (or when the list empties). Guarded so it only runs when there is something older to drop:
// the list is non-empty and `index` is at least the front entry's stamp. If no entry carries
// exactly `index`, it empties the list - the loop bound is the stamp match, not a position.
void ActionBuffer::removeOlderThanIndex(int index)
{
    if (m_entries.empty() || index >= m_entries.front().m_action.m_inputIndex)
    {
        while (!m_entries.empty() && m_entries.front().m_action.m_inputIndex != index)
            m_entries.pop_front();
    }
}

// Calls clear(0) before member destruction, including its neutral-action reseeding.
ActionBuffer::~ActionBuffer()
{
    clear(0);
    // m_entries destructs here.
}


// [INFERRED] Latches the front action as current and removes it. Keep the separate
// nonempty checks for latching and unlinking, matching the two original guards.
void ActionBuffer::popFront()
{
    if (!m_entries.empty())                       // +0x08, the list size
    {
        m_currentInputIndex = m_entries.front().m_action.m_inputIndex;   // +0x10
        m_current.set(m_entries.front().m_action);                       // +0x14, the 28-byte copy
    }
    if (!m_entries.empty())                       // tested again - see the note
        m_entries.pop_front();
}
} // namespace hfe
} // namespace dice
