// [INFERRED] path; interface identity is ELF RTTI-attested.
// Complete ELF88B0AE8 and PE90B830 tables,
#ifndef GAME_OBJECTS_IPOSTPROCESSCOMP_H
#define GAME_OBJECTS_IPOSTPROCESSCOMP_H
#include "IObjectComponent.h"
namespace dice { namespace hfe { namespace world {
// [INFERRED] record name and pointer spelling (a reference has the same ABI).
// getInfo ELF8227CB0/PE68ABD0 returns template+18/+48; the four float
// setter/getter pairs ELF8228750..82287C0 prove these independent fields.
struct PostProcessInfo {
    float tvInterference, tvDistortionRoll, tvDistortionFreq, tvDistortionScale;
};
class IPostProcessComp : public IObjectComponent {
public:
    virtual PostProcessInfo* getInfo() = 0; // +38, borrowed
    virtual ~IPostProcessComp() {}         // +3C PE / +3C,+40 ELF
};
} } }
#endif
