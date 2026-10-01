// Base interface for script-defined AI plug-in templates.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGIN_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGIN_H

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplate;
class IAIObjectPlugIn;

class AITemplatePlugIn
{
public:
    virtual ~AITemplatePlugIn();

    // [INFERRED] Returns the runtime plug-in interface consumed by addPlugIn.
    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate) = 0;

    // The plug-in's slot in AITemplate::m_plugIns - AITemplatePhysical answers 4.
    virtual int getId() = 0;

    virtual int getTemplateKind();                  // `xor eax, eax` - always 0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATEPLUGIN_H
