// Extracts object archives for loose-mode editing and packs the Objects folder into client/server archives.
// Extraction never overwrites existing loose files. Editing requires archives to be unmounted because they take priority.
// Packing applies the editor's file-kind exclusions, compresses only when smaller and atomically replaces each zip.
#ifndef BF2EDITOR_FRAMEWORK_OBJECTARCHIVES_H
#define BF2EDITOR_FRAMEWORK_OBJECTARCHIVES_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

// Whether the retail packager puts a file (a path inside the content folder) into the client or the
// server zip of its folder.
bool objectArchiveTakes(const std::string& path, bool server);

// The `fileManager.mountArchive <zip> <mount point>` lines of an archive script: each zip mounted at
// `mountPoint` (compared without regard to case).
std::vector<std::string> archivesMountedAt(const std::string& scriptText, const std::string& mountPoint);

struct ExtractReport
{
    int                files;       // written
    int                kept;        // already there, left alone
    unsigned long long bytes;       // written
    ExtractReport() : files(0), kept(0), bytes(0) {}
};

// The total size of a zip's files uncompressed.
bool archiveContentSize(const std::string& zipPath, unsigned long long& bytes, std::string& error);
// Every file of the zip written under `folder` (its paths kept), none overwritten.
bool extractArchive(const std::string& zipPath, const std::string& folder, ExtractReport& report, std::string& error);

struct ObjectPackReport
{
    int                clientFiles;
    int                serverFiles;
    unsigned long long clientBytes;     // the zips' sizes
    unsigned long long serverBytes;
    int                left;            // files neither zip takes
    ObjectPackReport() : clientFiles(0), serverFiles(0), clientBytes(0), serverBytes(0), left(0) {}
};

// <destination>/Objects_client.zip and Objects_server.zip made from the loose folder `objects`.
bool packObjectArchives(const std::string& objects, const std::string& destination, ObjectPackReport& report,
                        std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_OBJECTARCHIVES_H
