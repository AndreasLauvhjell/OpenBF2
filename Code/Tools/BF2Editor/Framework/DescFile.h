// Code/Tools/BF2Editor/Framework/DescFile.h

// The small XML description files the game's menus read: a mod's mod.desc (title, desc,
// url, logo, icon, music, version) and a level's Info/<level>.desc (name, briefing, music; its modes list is
// MapDescription's). The retail mod manager edited them on its settings and level-description pages. Here an
// element's text is changed in place and every other byte is kept: its attributes (a briefing's locid), the
// blanks the stock files put inside the tags ("<title> X </title>"), the file's line ends. A missing element
// is added before the root's closing tag, indented as its first child is. Text is escaped (& < >) on the way
// in and unescaped on the way out. Engine-free (checked in bf2_editor_checks). Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_DESCFILE_H
#define BF2EDITOR_FRAMEWORK_DESCFILE_H

#include "Framework/EditorConfig.h"

#include <string>

namespace bf2editor
{

// The text of the first <element ...>text</element> (trimmed, unescaped); false when there is none.
bool descText(const std::string& xml, const std::string& element, std::string& text);

// Sets it: the text between the tags replaced, keeping a blank the old text had on either side; a missing
// element added inside `root` (before </root>). False, `xml` unchanged, when the root's closing tag is not
// there or the element is written as an empty tag (<element/>).
bool setDescText(std::string& xml, const std::string& root, const std::string& element, const std::string& text);

// Takes the first <element ...>...</element> out, with its line when it stands alone on one (an element
// setDescText added, taken out again by an undo). False when there is none.
bool removeDescElement(std::string& xml, const std::string& element);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_DESCFILE_H
