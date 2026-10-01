// One-byte empty debug object. Type name and header placement are [INFERRED].
// Engine shutdown deletes the allocation without clearing its global pointer; repeated shutdown
// therefore double-frees. Preserve the lifecycle until an intentional change is approved.

#ifndef CORE_DEBUGDRAW_H        // [INFERRED] guard style
#define CORE_DEBUGDRAW_H

namespace dice {
namespace hfe {

// [INFERRED] type name - see above. Empty by measurement.
class DebugDraw
{
};

extern DebugDraw* debugDraw;    // ds:0x8ABBC78, variable name attested

} // namespace hfe
} // namespace dice

#endif // CORE_DEBUGDRAW_H
