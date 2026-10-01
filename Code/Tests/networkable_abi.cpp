// SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
//
// Verifies the INetworkable/IBase hierarchy, the recovered 32-bit layouts and the virtual order
// emitted by MSVC from the recovered declaration. The ELF has separate D1/D0 destructor entries;
// MSVC uses one deleting-destructor entry, so its post-destructor slots are one position earlier.

#include "../BF2/IO/Network/NetworkableBase.h"
#include "../BF2/Game/Objects/Networkables/Networkables.h"

#include <cstdio>
#include <type_traits>

namespace dice {
namespace hfe {
namespace io {

class BitStream
{
};

struct BaseLineData
{
    int value;
};

} // namespace io
} // namespace hfe
} // namespace dice

using dice::hfe::IBase;
using dice::hfe::io::BaseLineData;
using dice::hfe::io::BitStream;
using dice::hfe::io::INetworkable;
using dice::hfe::io::NetworkableBase;

static_assert(std::is_base_of<IBase, INetworkable>::value,
              "INetworkable must inherit the IBase quartet");
static_assert(std::is_base_of<INetworkable, NetworkableBase>::value,
              "NetworkableBase must preserve the INetworkable primary base");

#if defined(_M_IX86)
static_assert(sizeof(INetworkable) == 0x1C, "INetworkable x86 layout drifted");
static_assert(sizeof(NetworkableBase) == 0x3C, "NetworkableBase x86 layout drifted");
#endif

namespace {

struct Checks
{
    Checks() : count(0), failures(0) {}

    void expect(bool condition, const char* message)
    {
        ++count;
        if (!condition)
        {
            ++failures;
            std::printf("FAIL: %s\n", message);
        }
    }

    int count;
    int failures;
};

class OrderProbe : public NetworkableBase
{
public:
    OrderProbe() : mark(0) {}
    virtual ~OrderProbe() {}

    virtual void addRef() { mark = 1; }
    virtual int getRef() { mark = 2; return 102; }
    virtual int release() { mark = 3; return 103; }
    virtual void* queryInterface(unsigned int iid) const
    {
        mark = 4;
        return iid == 104 ? const_cast<OrderProbe*>(this) : 0;
    }
    virtual bool init(void* value) { mark = 6; return value == this; }
    virtual void reset() { mark = 7; }
    virtual void addedToManager() { mark = 8; }
    virtual void removedFromManager() { mark = 9; }
    virtual unsigned int getGhostStateMask() const { mark = 10; return 110; }
    virtual void updateStateMask(unsigned int* value) { mark = 11; *value = 111; }
    virtual bool getNetUpdate(BitStream&, BaseLineData* baseLine,
                              unsigned int stateMask, int precision)
    {
        mark = 12;
        return baseLine && stateMask == 112 && precision == 212;
    }
    virtual void setNetUpdate(BitStream&, float updateTime, bool ghostActive,
                              BaseLineData* baseLine, int precision)
    {
        mark = (updateTime == 13.0f && ghostActive && baseLine && precision == 213) ? 13 : -13;
    }
    virtual bool getBaseLine(BaseLineData& baseLine)
    {
        mark = 14;
        baseLine.value = 114;
        return true;
    }
    virtual void setGhostActive(bool active) { mark = active ? 15 : -15; }
    virtual unsigned int getMinimumBitSize() const { mark = 16; return 116; }
    virtual float getAverageUpdateDeltaTime() const { mark = 17; return 117.0f; }

    mutable int mark;
};

class DefaultProbe : public NetworkableBase
{
public:
    DefaultProbe() : refs(1) {}

    virtual void addRef() { ++refs; }
    virtual int getRef() { return refs; }
    virtual int release() { return --refs; }
    virtual void* queryInterface(unsigned int) const { return 0; }
    virtual bool init(void*) { return true; }
    virtual void updateStateMask(unsigned int*) {}
    virtual bool getNetUpdate(BitStream&, BaseLineData*, unsigned int, int) { return true; }
    virtual void setNetUpdate(BitStream&, float, bool, BaseLineData*, int) {}

    void seedNetworkableId(unsigned short value) { m_networkableId = value; }
    unsigned short networkableId() const { return m_networkableId; }
    bool ghostActive() const { return m_ghostActive; }
    bool forceFullUpdate() const { return m_forceFullUpdate; }
    int baselineCount() const { return m_count; }

private:
    int refs;
};

void testRawVirtualOrder(Checks& checks)
{
#if defined(_M_IX86)
    OrderProbe probe;
    void** vtable = *reinterpret_cast<void***>(&probe);
    BitStream stream;
    BaseLineData baseLine = {0};
    unsigned int stateMask = 0;

    typedef void (__thiscall *VoidFn)(OrderProbe*);
    typedef int (__thiscall *IntFn)(OrderProbe*);
    typedef void* (__thiscall *QueryFn)(OrderProbe*, unsigned int);
    typedef bool (__thiscall *InitFn)(OrderProbe*, void*);
    typedef unsigned int (__thiscall *UIntFn)(OrderProbe*);
    typedef void (__thiscall *UpdateMaskFn)(OrderProbe*, unsigned int*);
    typedef bool (__thiscall *GetNetFn)(OrderProbe*, BitStream&, BaseLineData*, unsigned int, int);
    typedef void (__thiscall *SetNetFn)(OrderProbe*, BitStream&, float, bool, BaseLineData*, int);
    typedef bool (__thiscall *BaseLineFn)(OrderProbe*, BaseLineData&);
    typedef void (__thiscall *GhostFn)(OrderProbe*, bool);
    typedef float (__thiscall *FloatFn)(OrderProbe*);

    reinterpret_cast<VoidFn>(vtable[0])(&probe);
    checks.expect(probe.mark == 1, "slot 0 is IBase::addRef");
    checks.expect(reinterpret_cast<IntFn>(vtable[1])(&probe) == 102 && probe.mark == 2,
                  "slot 1 is IBase::getRef");
    checks.expect(reinterpret_cast<IntFn>(vtable[2])(&probe) == 103 && probe.mark == 3,
                  "slot 2 is IBase::release");
    checks.expect(reinterpret_cast<QueryFn>(vtable[3])(&probe, 104) == &probe && probe.mark == 4,
                  "slot 3 is IBase::queryInterface");

    // slot 4 is MSVC's deleting destructor. The ELF address point has D1 and D0 at +0x10/+0x14.
    checks.expect(reinterpret_cast<InitFn>(vtable[5])(&probe, &probe) && probe.mark == 6,
                  "slot 5 is init after the MSVC deleting destructor");
    reinterpret_cast<VoidFn>(vtable[6])(&probe);
    checks.expect(probe.mark == 7, "slot 6 is reset");
    reinterpret_cast<VoidFn>(vtable[7])(&probe);
    checks.expect(probe.mark == 8, "slot 7 is addedToManager");
    reinterpret_cast<VoidFn>(vtable[8])(&probe);
    checks.expect(probe.mark == 9, "slot 8 is removedFromManager");
    checks.expect(reinterpret_cast<UIntFn>(vtable[9])(&probe) == 110 && probe.mark == 10,
                  "slot 9 is getGhostStateMask");
    reinterpret_cast<UpdateMaskFn>(vtable[10])(&probe, &stateMask);
    checks.expect(probe.mark == 11 && stateMask == 111, "slot 10 is updateStateMask");
    checks.expect(reinterpret_cast<GetNetFn>(vtable[11])(&probe, stream, &baseLine, 112, 212) &&
                      probe.mark == 12,
                  "slot 11 is getNetUpdate");
    reinterpret_cast<SetNetFn>(vtable[12])(&probe, stream, 13.0f, true, &baseLine, 213);
    checks.expect(probe.mark == 13, "slot 12 is setNetUpdate");
    checks.expect(reinterpret_cast<BaseLineFn>(vtable[13])(&probe, baseLine) &&
                      probe.mark == 14 && baseLine.value == 114,
                  "slot 13 is getBaseLine");
    reinterpret_cast<GhostFn>(vtable[14])(&probe, true);
    checks.expect(probe.mark == 15, "slot 14 is setGhostActive");
    checks.expect(reinterpret_cast<UIntFn>(vtable[15])(&probe) == 116 && probe.mark == 16,
                  "slot 15 is getMinimumBitSize");
    checks.expect(reinterpret_cast<FloatFn>(vtable[16])(&probe) == 117.0f && probe.mark == 17,
                  "slot 16 is getAverageUpdateDeltaTime");
#else
    checks.expect(false, "networkable ABI test must run as x86");
#endif
}

void testInheritedDefaults(Checks& checks)
{
    DefaultProbe probe;
    IBase* base = &probe;
    INetworkable* networkable = &probe;
    BaseLineData baseLine = {0};

    checks.expect(reinterpret_cast<void*>(base) == reinterpret_cast<void*>(networkable),
                  "IBase is the zero-offset primary base");
    checks.expect(probe.ghostActive() && probe.forceFullUpdate() && probe.baselineCount() == 0,
                  "constructors preserve the recovered initial state");
    checks.expect(networkable->getGhostStateMask() == 0xFFFFFFFFu,
                  "NetworkableBase supplies getGhostStateMask at the inherited slot");
    checks.expect(networkable->getBaseLine(baseLine),
                  "NetworkableBase supplies getBaseLine at the inherited slot");
    checks.expect(networkable->getMinimumBitSize() == 0 &&
                      networkable->getAverageUpdateDeltaTime() == 0.0f,
                  "INetworkable tail defaults dispatch through the corrected table");

    probe.seedNetworkableId(42);
    networkable->removedFromManager();
    checks.expect(probe.networkableId() == 0,
                  "INetworkable::removedFromManager remains reachable at its inherited slot");

    networkable->setGhostActive(false);
    probe.makeRoomForNextUpdate(false);
    networkable->reset();
    checks.expect(probe.ghostActive() && probe.forceFullUpdate() && probe.baselineCount() == 0,
                  "INetworkable pointer dispatches reset to NetworkableBase");
}

} // namespace

int main()
{
    Checks checks;
    testRawVirtualOrder(checks);
    testInheritedDefaults(checks);
    std::printf("networkable ABI: %d checks, %d failures\n", checks.count, checks.failures);
    return checks.failures == 0 ? 0 : 1;
}
