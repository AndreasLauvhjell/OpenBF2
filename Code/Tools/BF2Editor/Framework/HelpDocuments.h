// Finds help documents and CommandDescriptions.dat in the user's retail-editor install
// for shell opening and console help. No help assets are bundled.
#ifndef BF2EDITOR_FRAMEWORK_HELPDOCUMENTS_H
#define BF2EDITOR_FRAMEWORK_HELPDOCUMENTS_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

struct HelpDocument
{
    std::string label;      // "User guide", "Workshop: LevelWorkshop"
    std::string path;       // as found, under the help folder
};

// The documents the help folder holds, in the menu's order: the user guide, the command list, the object
// editor's sheet, then each workshop and each tutorial (*.doc in Help/Workshop and Help/Tutorial, by name).
// Only files that are there.
std::vector<HelpDocument> helpDocuments(const std::string& helpFolder);

// A document by its label or its file name (case-insensitive); null when there is none.
const HelpDocument* findHelpDocument(const std::vector<HelpDocument>& documents, const std::string& name);

// Opens a document with the program the system has for it (the shell's "open"), as retail did.
bool openHelpDocument(const HelpDocument& document, std::string& result);

// The retail editor's help folder, relative to the game folder the editor runs in.
const char* const kHelpFolder = "bf2editor/Help";

// CommandDescriptions.dat: two little-endian 32-bit counts (rows, columns), then rows x columns
// zero-terminated UTF-16LE strings, row by row; the first row is the column headings. An 8-byte trailer
// follows the table [UNRESOLVED: what it holds; it is not needed to read the table].
class CommandCatalogue
{
public:
    bool parse(const std::string& bytes, std::string& error);
    bool load(const std::string& path, std::string& error);

    size_t size() const { return m_rows.size(); }
    const std::string& name(size_t i) const { return m_rows[i].first; }
    const std::string& description(size_t i) const { return m_rows[i].second; }
    // A command's description (UTF-8), the name compared without case; null when it is not listed.
    const std::string* describe(const std::string& command) const;

private:
    std::vector<std::pair<std::string, std::string> > m_rows;     // (name, description), headings dropped
    std::map<std::string, size_t>                     m_byName;   // lower-case name -> row
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_HELPDOCUMENTS_H
