// Code/Tools/BF2Editor/Framework/LevelCopier.h

// File > Save level as (the retail TerrainEditor's Save As, plugin census §12: "copies the level under a
// new name ... rewrites path references inside the level's *.con, *.emi and *.tai files"): the saved level
// folder copied to Levels/<new name>/, with every `Levels/<old name>/` path reference (either slash, any
// case) pointing at the copy in the level's text files (.con .emi .tai .desc .txt .ai .cfg), in the four
// texture base-name lines of terraindata.raw's header, and in file names that begin with the old name
// (Info/<old>.desc, the menu pictures). The map description's display name (<name>) becomes the new name
// with its underscores as spaces. Everything else is copied byte for byte. Engine-free (Win32 files).
// Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_LEVELCOPIER_H
#define BF2EDITOR_FRAMEWORK_LEVELCOPIER_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

// A level folder name the game takes: letters, digits and underscores, 1..64 of them.
bool validLevelName(const std::string& name);

// The text with every `Levels/<from>/` (or `Levels\<from>\`, any case) naming `to` instead; the slash and
// the word "Levels" keep their spelling. `count` gets the number of references changed.
std::string retargetLevelPaths(const std::string& text, const std::string& from, const std::string& to, int& count);

// terraindata.raw with its four base-name lines retargeted (the header: version, two Vec3, three floats,
// patch size, subdivide byte, three sizes = 57 bytes, then colormap, detailmap, low-detail map and
// lightmap base names, each ended by '\n'); false when the file is too short for that.
bool retargetTerrainData(std::vector<unsigned char>& data, const std::string& from, const std::string& to, int& count);

struct LevelCopyReport
{
    int files;
    int rewritten;          // files whose contents changed
    int references;         // path references changed
    int renamed;            // files whose names changed
    LevelCopyReport() : files(0), rewritten(0), references(0), renamed(0) {}
};

// Copies `source` (the Levels/<from> folder) to `target` (the Levels/<to> folder, which must not exist):
// written into `target`.tmp first and moved into place when whole.
bool copyLevel(const std::string& source, const std::string& target, const std::string& from, const std::string& to,
               LevelCopyReport& report, std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LEVELCOPIER_H
