// Code/Tools/BF2Editor/Framework/MapDescription.h

// A level's map description (Levels/<level>/Info/<level>.desc, the XML the game's map list and menus read)
// as the editor edits it: which gameplay layers it lists - `<mode type="gpm_cq">`
// with a `<maptype players="64" ...>` line per size - and a layer added to it, as text. A new size is a
// copy of its mode's last maptype line with the players changed and without `ai="1"` (a copied layer has
// no AI data); a new mode is a mode element of its own, its line a copy of the last mode's first. Every
// other byte is kept. The level folder's root map.desc is older data the game does not read. Engine-free
// (checked in bf2_editor_checks). Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_MAPDESCRIPTION_H
#define BF2EDITOR_FRAMEWORK_MAPDESCRIPTION_H

#include "Framework/EditorConfig.h"

#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

// The layers listed: (mode, players), in file order.
std::vector<std::pair<std::string, int> > mapDescriptionLayers(const std::string& xml);
// `xml` with the layer listed; false (and `xml` unchanged) when it is listed already or the text has no
// modes element to add it to.
bool addMapDescriptionLayer(std::string& xml, const std::string& mode, int players);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_MAPDESCRIPTION_H
