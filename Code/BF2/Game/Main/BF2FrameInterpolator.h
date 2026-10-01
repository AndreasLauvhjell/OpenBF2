// Client frame interpolation owned and driven by BF2Engine. Stores simulation
// transforms, temporarily interpolates them for rendering, then restores them.
// [INFERRED] Class name; implementation is client-only.

#ifndef GAME_MAIN_BF2FRAMEINTERPOLATOR_H       // [INFERRED] guard style
#define GAME_MAIN_BF2FRAMEINTERPOLATOR_H

#include <vector>

#include "../../Math/Mat4.h"      // Entry's 64-byte transform - see postRender

namespace dice {
namespace hfe {

namespace world { class IObject; }

// sizeof == 0x2C (44) in BF2.exe: one bool plus padding, an int, a float, and two 16-byte VC7.1 vectors.
// The constructor (0x45BF10) writes only the enabled byte and zeroes everything except the two
// allocator words. This source keeps the STL operations rather than forcing VC7.1 vector layout
// onto the native compiler; only the retail offsets below are evidence about the shipped image.
class BF2FrameInterpolator
{
public:
    BF2FrameInterpolator();                          // BF2.exe 0x45BF10, 32 B          recovered
    ~BF2FrameInterpolator() {}                       // recovered: inlined at 0x408AB0, it frees the two vectors
                                                     // Stores the tick count and snapshots eligible visible objects before simulation.
                                                     // Allocates snapshot state as needed for later render interpolation.
    void update(int ticks);                          // BF2.exe 0x45BBB0                recovered

    // Both timing arguments are narrowed to float before interpolation.
    void interpolate(float exactTime, float prevTickTime);    // BF2.exe 0x45C190     recovered, [INFERRED] name

    // mainLoop, after BF2Render's frame: walks the 68-byte entries (an object pointer followed by a
    // Mat4) calling setRelativeTransformation or setAbsoluteTransformation on each object depending
    // on whether it has a parent, then empties the vector (0x45BDF0 = resize).
    void postRender();                               // BF2.exe 0x45C4B0                recovered, [INFERRED] name

    // Read directly by mainLoop (`*(*(this+0x1C)+8)`) and handed to Demo::visualUpdate. IT IS A
    // float, not a tick count (): interpolate stores the clamped [-0.1, 1.1]
    // tick fraction there with `fstp dword ptr [esi+8]`, and mainLoop moves the dword straight into
    // visualUpdate's float argument with no conversion instruction.
    float getInterpolationFraction() const { return m_fraction; }   // +0x08, [INFERRED] accessor

private:
    // Walks sibling chains and recurses into children. Roots qualify; parented objects
    // require rotational bundles, with an extra soldier case for interpolation.
    // snapshotObject writes transform snapshots; interpolateObject records restoration entries.
    void snapshotObject(world::IObject* object);                     // BF2.exe 0x45B1F0  recovered
    void interpolateObject(world::IObject* object, float fraction);  // BF2.exe 0x45BFC0  recovered

    // 68 bytes. Proven from both ends: 0x45BDF0's `retn 48h` and its 0x78787879 (divide-by-68)
    // element-count magic, and the push_back site at 0x45C0A4 which builds the value as an object
    // pointer at +0x00 followed by a Mat4 copy-constructed at +0x04.
    struct Entry
    {
        world::IObject* object;                      // +0x00
        Mat4            transform;                   // +0x04
    };

    /* +0x00 */ bool                           m_enabled;      // ctor byte-store = 1; 0x45BBB0 and
                                                               //    both test it first
    /* +0x04 */ int                            m_ticks;        //  stores its argument here
    /* +0x08 */ float                          m_fraction;     // the clamped tick fraction; read by mainLoop
    /* +0x0C */ std::vector<world::IObject*>   m_objects;      // the visible set; refilled every update and
                                                               //   every interpolate (4-byte elements)
    /* +0x1C */ std::vector<Entry>             m_entries;      // walked and emptied by 0x45C4B0
};

} // namespace hfe
} // namespace dice

#endif // GAME_MAIN_BF2FRAMEINTERPOLATOR_H
