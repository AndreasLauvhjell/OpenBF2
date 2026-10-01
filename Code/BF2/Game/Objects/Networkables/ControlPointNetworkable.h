// Replicates control-point team, capture state, takeover progress and flag state with four baselines.
// The mask is 7 bits despite getMinimumBitSize returning 6; timestamps remain zero in this component.

#ifndef GAME_OBJECTS_NETWORKABLES_CONTROLPOINTNETWORKABLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_NETWORKABLES_CONTROLPOINTNETWORKABLE_H

#include "../../../IO/Network/NetworkableBase.h"
#include "../../../IO/Network/INetworkableObject.h"
#include "../../../IO/Network/NetworkStats.h"

namespace dice {
namespace hfe {

namespace world { class ControlPoint; class IObject; }

class ControlPointNetworkable : public io::NetworkableBase, public io::NetworkableObjectBase
{
public:
    // The registered factory (0x877A060, 145 B) - the whole constructor is inlined into it, so the
    // ctor here is [INFERRED] from those stores. Refuses aggregation (outer != 0 -> 0).
    static IBase* create(unsigned int iid, IBase* outer);
    ControlPointNetworkable();
    virtual ~ControlPointNetworkable();

    // the quartet, refcount at +0x50
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                         // the "count WAS 1" shape
    virtual void* queryInterface(unsigned int) const;// IBase/INetworkable -> this,
                                                     // INetworkableObject -> the +0x3C base

    // INetworkable
    virtual bool  init(void* object);                // bind + baseline ring
    virtual void  updateStateMask(unsigned int* stateMask);
    virtual bool  getNetUpdate(io::BitStream& stream, io::BaseLineData* baseline,
                               unsigned int stateMask, int precision);
    virtual void  setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                               io::BaseLineData* baseline, int precision);
    virtual unsigned int getMinimumBitSize() const;  // 6
    virtual float getAverageUpdateDeltaTime() const; // (rec[head].time - rec[tail].time) / count

    // INetworkableObject
    virtual void  predict(float);                    // empty: control points are not predicted

    // One 36-byte baseline record; the ring is four of them (see the header note for the wire).
    struct BaseLine
    {
        /* +0x00 */ float time;                      // never written by this class - always 0
        /* +0x04 */ bool  byte04;                    // init zeroes; nothing reads
        /* +0x08 */ int   team;
        /* +0x0C */ bool  bool158;
        /* +0x10 */ float takeOverCounter;
        /* +0x14 */ float takeOverChangePerSecond;
        /* +0x18 */ bool  allowCaptureByTeam1;
        /* +0x19 */ bool  allowCaptureByTeam2;
        /* +0x1C */ int   flagTeam;
        /* +0x20 */ int   capturingState;
    };

    static io::RegistredNetworkStats m_stats;        // "ControlPointNetworkable"

protected:
    // layout +0x50..+0x5B (NetworkableBase 0x00..0x3B, NetworkableObjectBase 0x3C..0x4F)
    /* +0x50 */ int                  m_refCount;     // ctor 1
    /* +0x54 */ BaseLine*            m_baselines;    // ctor 0; init news [4]; the dtor delete[]s
    /* +0x58 */ world::ControlPoint* m_controlPoint; // init's object, typed. the ELF ctor leaves
                                                     // it unwritten until init; nulled here
                                                     // [CHOSEN] - nothing reads it before init
};

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_NETWORKABLES_CONTROLPOINTNETWORKABLE_H
