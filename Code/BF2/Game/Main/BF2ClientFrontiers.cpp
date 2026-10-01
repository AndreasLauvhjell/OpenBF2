// [INFERRED] diagnostic companions to BF2ClientSeams.h, not recovered bodies.
#include "BF2ClientSeams.h"
#if !DEDICATED
#include <cstdio>
#include <cstdlib>

namespace dice { namespace hfe {
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: local-profile-client-callers]
// The native manager publishes through +C8, with two approved abort-only positions.
// The remaining activateMod caller chain first stops at untyped +D0. These signatures
// describe the engine callers, not a recovered LocalProfileManager vtable.
#define BF2_LOCAL_PROFILE_FRONTIER(method) \
    std::fprintf(stderr, "[FRONTIER] dice::hfe::" #method ": LocalProfileManager suffix is unrecovered\n"); \
    std::fflush(stderr); \
    std::abort()

void localProfileManager_vslot0x70(IBase*, const std::string&) { BF2_LOCAL_PROFILE_FRONTIER(localProfileManager_vslot0x70); }
const std::string& localProfileManager_vslot0xD0(IBase*, std::string&, int) { BF2_LOCAL_PROFILE_FRONTIER(localProfileManager_vslot0xD0); }
#undef BF2_LOCAL_PROFILE_FRONTIER

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: swiff-host-server-browser]
// Haptic targets remain null until their implementations exist; menu initialization
// checks for null before dispatching to them.
IBase* g_hapticForceTargets[6] = { 0, 0, 0, 0, 0, 0 };
#define BF2_MENU_SEAM_FRONTIER(name, text) \
    std::fprintf(stderr, "[FRONTIER] dice::hfe::" #name ": " text "\n"); \
    std::fflush(stderr); \
    std::abort()
void   hapticTarget_vslot0x4C(IBase*, const double*)                     { BF2_MENU_SEAM_FRONTIER(hapticTarget_vslot0x4C, "the haptic force target's +0x4C (a NovintHFX::Vect3 setter) is unrecovered"); }
#undef BF2_MENU_SEAM_FRONTIER
} }
#endif
