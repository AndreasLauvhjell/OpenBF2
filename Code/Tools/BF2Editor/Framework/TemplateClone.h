// Clones template text for Save As, preserving method lines that engine serialization omits.
// Renames locally defined children and owner-derived sound names plus their whole-word references;
// children in separate files remain shared. Geometry/collision resources remain shared and their
// definition commands are omitted. The include path is changed to the new tweak filename.
#ifndef BF2EDITOR_FRAMEWORK_TEMPLATECLONE_H
#define BF2EDITOR_FRAMEWORK_TEMPLATECLONE_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct TemplateCloneRename
{
    std::string from;
    std::string to;
};

struct TemplateClone
{
    std::string                      con;           // the new .con
    std::string                      tweak;         // the new .tweak; empty when the source has none
    std::vector<TemplateCloneRename> renames;       // the source first, then each cloned child, then the sounds
    int                              droppedLines;  // GeometryTemplate / CollisionManager lines left out
    int                              sounds;        // sound templates renamed after their owners (in renames)
    TemplateClone() : droppedLines(0), sounds(0) {}
};

// `conStem` is the source .con's file name without its extension: its include line names <conStem>.tweak.
bool cloneTemplateFiles(const std::string& conText, const std::string& tweakText, const std::string& conStem,
                        const std::string& source, const std::string& target, TemplateClone& out, std::string& error);

// The name a cloned child takes: <target> and what follows <source> in its name, or <target>_<child>.
std::string clonedChildName(const std::string& child, const std::string& source, const std::string& target);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_TEMPLATECLONE_H
