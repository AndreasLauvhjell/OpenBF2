// Reference-property pickers for mounted animation/sound files, engine templates and materials.
// Selections return the value written to the file; texture thumbnails use TexturePalette.
#ifndef BF2EDITOR_APP_PROPERTYPICKERS_H
#define BF2EDITOR_APP_PROPERTYPICKERS_H

#include "Framework/EditorConfig.h"

#include <string>

namespace bf2editor
{

class EditSession;

namespace PropertyPickers
{

// Whether a picker serves this property (its name without a set/get prefix, any case).
bool serves(const std::string& property);
// The button beside the property's text and its popup; true when something was picked (`out` is the value).
bool button(const char* id, const std::string& property, const std::string& value, EditSession& session, std::string& out);

} // namespace PropertyPickers

} // namespace bf2editor

#endif // BF2EDITOR_APP_PROPERTYPICKERS_H
