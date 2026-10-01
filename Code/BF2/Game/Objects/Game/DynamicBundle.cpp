// Path: [INFERRED]. ELF 0x81E0260/300; PE 0x627420/480, read in full.
#include "DynamicBundle.h"
#include "../TemplateLeaves.h"

namespace dice { namespace hfe { namespace world {
DynamicBundle::DynamicBundle(const DynamicBundleTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    updateFlags(8, 0);
}
DynamicBundle::~DynamicBundle() {}
} } }
