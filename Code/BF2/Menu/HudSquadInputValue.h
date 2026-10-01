// [CHOSEN, not recovered] Plain value storage for squad input.
// Preserves the original encoded value when passing it back. Do not add a vptr or virtual destructor.
#ifndef MENU_HUDSQUADINPUTVALUE_H
#define MENU_HUDSQUADINPUTVALUE_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <cstdint>
namespace dice { namespace hfe { namespace hud {
class HudSquadInputValue {
public:
    // 4E8150 (28 B). The first word is never written, as shipped.
    explicit HudSquadInputValue(const int& value) : m_value(value) {}
    int get() const { return m_value; }  // the inlined +4 read through 4CBE60
private:
    std::uint32_t m_untouched;           // +0, never written
    int m_value;                         // +4, the retail word holds 4E30C0(K, value)
};
static_assert(sizeof(HudSquadInputValue) == 8, "native input-event value");
} } }
#endif
#endif
