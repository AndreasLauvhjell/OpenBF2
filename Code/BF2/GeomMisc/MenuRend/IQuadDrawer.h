// Quad drawing interface with scalar arguments. No virtual destructor or STL values cross this boundary.
// Method names are [INFERRED].
#ifndef GEOM_MENUREND_IQUADDRAWER_H
#define GEOM_MENUREND_IQUADDRAWER_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include "../../Core/IBase.h"
namespace dice { namespace hfe { namespace geom {
class IQuadDrawer : public IBase
{
public:
    virtual void flush() = 0;
    virtual void beginFrame() = 0;
    virtual void resetState() = 0;
    virtual void endFrame() = 0;    // +1C 1008AF10
};
} } }
#endif
#endif
