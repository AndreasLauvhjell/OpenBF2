// Free-camera replication component. A four-slot ring carries position, rotation
// and the followed player id; velocity is derived for prediction. Player creation
// requires this component before registering the camera with NetworkManager.

#ifndef GAME_OBJECTS_NETWORKABLES_FREECAMERANETWORKABLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_NETWORKABLES_FREECAMERANETWORKABLE_H

#include "../../../IO/Network/NetworkableBase.h"
#include "../../../IO/Network/INetworkableObject.h"
#include "../../../IO/Network/NetworkStats.h"
#include "../../../Math/Quat.h"

namespace dice {
namespace hfe {

namespace world { class FreeCamera; class IObject; }

class FreeCameraNetworkable : public io::NetworkableBase, public io::NetworkableObjectBase
{
public:
    // The registered factory (0x8779A60, 145 B) - the constructor is inlined into it, so the ctor
    // here is [INFERRED] from those stores. Refuses aggregation (outer != 0 -> 0), and releases
    // itself when the requested interface is not one of its three.
    static IBase* create(unsigned int iid, IBase* outer);
    FreeCameraNetworkable();
    virtual ~FreeCameraNetworkable();

    // the quartet, refcount at +0x50
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    // INetworkable
    virtual bool  init(void* object);
    virtual void  updateStateMask(unsigned int* stateMask);
    virtual bool  getNetUpdate(io::BitStream& stream, io::BaseLineData* baseline,
                               unsigned int stateMask, int precision);
    virtual void  setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                               io::BaseLineData* baseline, int precision);
    virtual bool  getBaseLine(io::BaseLineData& baseline);

    // Interpolates or extrapolates snapshot transforms using millisecond timestamps.
    virtual void  predict(float time);

    // Non-virtual helper of the prediction path: given a time, the two ring records that bracket it.
    bool findEmbracingIndices(float time, unsigned int* lo, unsigned int* hi) const;

    // One 56-byte ring record - see the header note for what each field carries.
    struct Record
    {
        /* +0x00 */ float time;
        /* +0x04 */ bool  byte04;
        /* +0x08 */ int   dword08;               // init does NOT write this one
        /* +0x0C */ float position[3];
        /* +0x18 */ float velocity[3];           // resolved by predict's floating-point stores
        /* +0x24 */ Quat  rotation;              // +0x24..+0x33, w = 1.0f from init
        /* +0x34 */ int   followedPlayerId;
    };

    static io::RegistredNetworkStats m_stats;        // "FreeCameraNetworkable"

protected:
    // layout +0x50..+0x57 (NetworkableBase 0x00..0x3B, NetworkableObjectBase 0x3C..0x4F)
    /* +0x50 */ int     m_refCount;                  // ctor 1
    /* +0x54 */ Record* m_records;                   // ctor 0; init news [4]; the dtor delete[]s
};

static_assert(sizeof(FreeCameraNetworkable::Record) == 0x38,
              "FreeCameraNetworkable::Record fixed-width layout drift");
#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(FreeCameraNetworkable) == 0x58,
              "FreeCameraNetworkable x86 layout drift");
#endif

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_NETWORKABLES_FREECAMERANETWORKABLE_H
