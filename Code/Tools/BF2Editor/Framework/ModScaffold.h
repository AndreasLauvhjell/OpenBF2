// Creates a mod's archive lists, game initialization and menu files with its own archives before base-mod archives.
// Missing archives are not mounted; text files use CRLF.
// [CHOSEN, not recovered] Also writes a 1.50 Mod.desc with the mod name as its title.
#ifndef BF2EDITOR_FRAMEWORK_MODSCAFFOLD_H
#define BF2EDITOR_FRAMEWORK_MODSCAFFOLD_H

#include "Framework/EditorConfig.h"

#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

// The files of a new mod named `name` layered over `baseMods` (in mount order), as (file name, bytes).
std::vector<std::pair<std::string, std::string> > modScaffoldFiles(const std::string& name,
                                                                     const std::vector<std::string>& baseMods);

// A mod name: letters, digits, '_' and '-' (a folder name and a +loadMod argument).
bool validModName(const std::string& name);

// mods/<name>/ made with the scaffold; refused when the folder exists.
bool createMod(const std::string& modsFolder, const std::string& name, const std::vector<std::string>& baseMods,
               std::string& error);

// The mods under `modsFolder`: the folders that hold a Mod.desc, sorted.
std::vector<std::string> listMods(const std::string& modsFolder);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_MODSCAFFOLD_H
