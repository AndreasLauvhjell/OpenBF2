// Reads/writes the twelve ordered lightmap settings in editor-only ObjectLight.con and TerrainLight.con blocks.
// Object and terrain tracing use separate GI and sun settings.
// [CHOSEN] Defaults come from currentLight.con because the game exposes no lightmap-generation bindings.
#ifndef BF2EDITOR_FRAMEWORK_LIGHTSETUPFILE_H
#define BF2EDITOR_FRAMEWORK_LIGHTSETUPFILE_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct LightSetup
{
    std::vector<std::string> values;        // in settingNames() order
    LightSetup();                           // the defaults
    static const std::vector<std::string>& settingNames();
    // 0 a float, 1 an int, 2 a bool (0/1), 3 x/y/z
    static int settingKind(size_t index);
    // A value checked for its setting; false with `error` when it is not one.
    bool set(const std::string& name, const std::string& value, std::string& error);
    bool get(const std::string& name, std::string& value) const;
    // The file's text, and a file read back (values it does not set keep their defaults).
    std::string render() const;
    bool parse(const std::string& text);
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LIGHTSETUPFILE_H
