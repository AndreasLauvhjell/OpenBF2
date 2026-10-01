// Player interface defaults and weak-reference control block.

#include "IPlayer.h"

namespace dice {
namespace hfe {
namespace world {

// vptr + null block.
IPlayer::IPlayer()
    : m_weakControl(0)
{
}

// (D2/D1, 58 B) / 0x83075E0 (D0, 74 B): the block's target is nulled - every live
// WeakPtr<IPlayer> now reads 0 - and the block released (deleted when this was its last holder).
IPlayer::~IPlayer()
{
    WeakPtr<IPlayer>::Ctrl* control = m_weakControl;
    if (control)
    {
        control->target = 0;
        if (control->refCount-- == 1)
            ::operator delete(control);
    }
}

// allocate {1, this} on first use, then hand out one more reference.
WeakPtr<IPlayer> IPlayer::getWeakPtr()
{
    if (!m_weakControl)
    {
        m_weakControl = static_cast<WeakPtr<IPlayer>::Ctrl*>(
            ::operator new(sizeof(WeakPtr<IPlayer>::Ctrl)));
        m_weakControl->refCount = 1;
        m_weakControl->target = this;
    }
    WeakPtr<IPlayer> result;
    result.m_ctrl = m_weakControl;
    ++m_weakControl->refCount;
    return result;
}

// The four non-pure slots (0x8307770 / 0x8307780 / 0x8307790 / 0x83077A0 - 5 / 7 / 5 / 7 B):
// the interface's own answers, overridden by Player.
void IPlayer::setIsRemote(bool)          {}
bool IPlayer::getIsRemote() const        { return false; }
void IPlayer::setIsAIPlayer(bool)        {}
bool IPlayer::getIsAIPlayer() const      { return false; }

} // namespace world

// Player flag registration order assigns bits 0 through 3. Bits 0 and 1 represent AI and network players.
// Bit 3 brackets remote item switches and affects sound feedback; bit 2 remains [UNRESOLVED].
Flag32Bits<world::IPlayer> g_playerFlagBit0(0);          // ds:0x8AB0A28  makeScript: Player.isAi
Flag32Bits<world::IPlayer> g_playerFlagBit1(1);          // ds:0x8AB0A30  makeScript: Player.isNetwork
Flag32Bits<world::IPlayer> g_playerFlagBit2(2);          // ds:0x8AB0A38  [UNRESOLVED meaning]
Flag32Bits<world::IPlayer> g_playerFlagBit3(3);          // ds:0x8AB0A40  the item-switch latch

} // namespace hfe
} // namespace dice
