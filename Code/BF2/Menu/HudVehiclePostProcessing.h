// [INFERRED] Call-shaped post-processing wrappers.
// Use the renderer's secondary table; the full cross-image interface and ownership remain unidentified.
#ifndef MENU_HUDVEHICLEPOSTPROCESSING_H
#define MENU_HUDVEHICLEPOSTPROCESSING_H
#include "../Core/IBase.h"
namespace dice { namespace hfe { namespace hud {
// The pointer retains ClassManager's type; the slot is reached raw, never
// through IBase's quartet.
void dispatchPostProductionInterference(IBase*, float); // PE+04
void dispatchPostProductionRoll(IBase*, float);         // PE+0C
void dispatchPostProductionFrequency(IBase*, float);    // PE+14
void dispatchPostProductionScale(IBase*, float);        // PE+1C
} } }
#endif
