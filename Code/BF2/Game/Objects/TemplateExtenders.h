// Object-template subclasses that extend their base interface. [INFERRED] Source placement.
// The shared IItemTemplate interface covers item index and type; rank and team settings belong to ItemTemplate.

#ifndef GAME_OBJECTS_TEMPLATEEXTENDERS_H     // [INFERRED] guard style
#define GAME_OBJECTS_TEMPLATEEXTENDERS_H

// Aggregate template includes.
#include "Game/SoldierTemplate.h"
#include "GenericFireArmTemplate.h"
#include "ItemContainerTemplate.h"
#include "ItemTemplate.h"
#include "Game/KitPart.h"

#include <string>

#include "Base/BundleTemplate.h"
#include "Base/SimpleObjectTemplate.h"
#include "ObjectTemplate.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class AnimatedBundleTemplate;
class IObjectTemplate;


} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_TEMPLATEEXTENDERS_H
