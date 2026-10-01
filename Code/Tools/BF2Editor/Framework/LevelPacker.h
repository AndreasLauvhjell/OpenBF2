// Packs loose level files into client/server archives in another mod folder and copies Info/ loose.
// Scripts and server data go to server.zip; visual/audio assets go to client.zip without overlap.
// Excludes Editor/, archive.md5 and backup files. Edited content requires content checking to be disabled.
// Archives take precedence over loose files when the game loads the result.
#ifndef BF2EDITOR_FRAMEWORK_LEVELPACKER_H
#define BF2EDITOR_FRAMEWORK_LEVELPACKER_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

enum class LevelArchive
{
    None,       // not packed (see above)
    Client,
    Server
};

// For a path relative to the level folder ('/' or '\' separators, any case).
LevelArchive levelArchiveFor(const std::string& relativePath);
const char* levelArchiveName(LevelArchive archive);    // "client.zip", "server.zip", ""

struct LevelPackReport
{
    int                      clientFiles;
    int                      serverFiles;
    int                      infoFiles;         // copied loose
    unsigned long long       clientBytes;       // archive sizes
    unsigned long long       serverBytes;
    std::vector<std::string> skipped;           // not packed, as relative paths
    LevelPackReport() : clientFiles(0), serverFiles(0), infoFiles(0), clientBytes(0), serverBytes(0) {}
};

// Packs the loose level folder `source` into the level folder `destination` (both absolute, each the
// Levels/<name> folder): client.zip and server.zip, each written beside as .tmp and moved into place
// once whole, and Info/ copied. False with `error` when the source has no files, when the destination is
// the source, or when a file cannot be read or written; nothing is moved into place then.
bool packLevel(const std::string& source, const std::string& destination, LevelPackReport& report, std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LEVELPACKER_H
