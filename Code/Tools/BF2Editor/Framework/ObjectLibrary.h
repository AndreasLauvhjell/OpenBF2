// Reads and writes object bundles and level brushes as CRLF console scripts.
// Bundle offsets are relative to the first/root object; rotations are absolute yaw/pitch/roll
// ([INFERRED] from the retail Euler conversion). Bundle vectors use two decimals, brush vectors three.
// Preserves item order and scale data; scale is retained for round trips but not applied.
// The editor writes actual rotation ranges instead of copying the retail position-range defect.
// Names accept only letters, digits, underscore and hyphen. Engine-independent and editor-only.
#ifndef BF2EDITOR_FRAMEWORK_OBJECTLIBRARY_H
#define BF2EDITOR_FRAMEWORK_OBJECTLIBRARY_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

// One record of a bundle: the root (records[0]) or a member.
struct BundleRecord
{
    std::string templateName;
    float       offset[3];      // in the root's frame; the root's is (0, 0, 0) and not written
    float       rotation[3];    // absolute yaw / pitch / roll, degrees
    int         layer;
    BundleRecord();
};

class ObjectBundleFile
{
public:
    std::vector<std::string>  preload;      // "run /objects/.../x.con" lines, each once
    std::vector<BundleRecord> records;      // records[0] is the root

    // False with `error` (the line and why) for a line it does not understand, a member before the
    // root, a second root, or no root at all.
    bool parse(const std::string& text, std::string& error);
    std::string render() const;
};

struct LevelBrushItem
{
    std::string templateName;
    float       position[3];    // from the stamp point, in world axes
    float       rotation[3];    // yaw / pitch / roll, degrees
    float       scale[3];       // kept, never applied
    LevelBrushItem();
};

class LevelBrushFile
{
public:
    std::string                 name;
    std::vector<std::string>    preload;
    float                       randomPositionMin[3];
    float                       randomPositionMax[3];
    float                       randomRotationMin[3];
    float                       randomRotationMax[3];
    float                       randomScaleMin[3];
    float                       randomScaleMax[3];
    float                       spacing;            // metres between stamps, measured across the ground (10)
    bool                        snapToGround;       // (true)
    std::vector<LevelBrushItem> items;
    // Set by parse: the rotation ranges repeat the position ranges and are not all zero - the mark of
    // the retail writer.
    bool                        rotationRangesCopied;

    LevelBrushFile();
    // False with `error` for a line it does not understand, a setting before CreateBrush, an item
    // setting before any AddItem, a second brush of another name, or no brush at all.
    bool parse(const std::string& text, std::string& error);
    std::string render() const;
};

// Park-Miller's "minimal standard" generator (multiplier 48271, modulus 2^31 - 1) with its own mapping to
// floats, so a seed gives the same stamps whatever compiled the editor. One generator serves every item
// of every stamp (retail reseeds from the clock for each item, so a stamp's items share their numbers).
class LibraryRandom
{
public:
    explicit LibraryRandom(unsigned seed = 1);
    void  seed(unsigned seed);              // 0, and a multiple of the modulus, become 1
    float next();                           // [0, 1)
    float between(float a, float b);        // a + next() * (b - a)
    unsigned state() const { return m_state; }

private:
    unsigned m_state;
};

bool validLibraryName(const std::string& name);

// The folder that holds ObjectBundle/ and LevelBrushes/. By default the retail editor's content folder
// under the game (the editor runs in the game's folder), so the two editors share their bundles and
// brushes.
class ObjectLibrary
{
public:
    static const char* defaultFolder();     // "bf2editor/Content/LevelEditor"

    ObjectLibrary();
    // The brushes are read again, from the new folder, when next used.
    void setFolder(const std::string& folder);
    const std::string& folder() const { return m_folder; }
    std::string bundlePath(const std::string& name) const;

    // Bundles: read from their files each time they are used
    std::vector<std::string> bundleNames() const;               // sorted, without ".con"
    bool bundleExists(const std::string& name) const;
    bool loadBundle(const std::string& name, ObjectBundleFile& out, std::string& error) const;
    // Refused for an existing name unless `overwrite`.
    bool saveBundle(const std::string& name, const ObjectBundleFile& bundle, bool overwrite, std::string& error) const;
    bool deleteBundle(const std::string& name, std::string& error) const;

    // brushes: every .con of LevelBrushes/ read once, as the retail plugin does at start; each
    // change is written back at once (writeBrush), so there is nothing to save or lose later ----------
    const std::vector<LevelBrushFile>& brushes();
    LevelBrushFile* brush(const std::string& name);             // any case; 0 when there is none
    // The files that could not be read, and why (one line each).
    const std::vector<std::string>& brushProblems();
    bool addBrush(const LevelBrushFile& brush, std::string& error);     // a new name only; written at once
    bool writeBrush(const std::string& name, std::string& error);       // as it stands now
    bool deleteBrush(const std::string& name, std::string& error);      // with its file
    void reloadBrushes() { m_brushesLoaded = false; }

    // The whole file written at once: to <path>.tmp, then moved over <path>; the folders are made first.
    static bool writeTextFile(const std::string& path, const std::string& text, std::string& error);
    static bool readTextFile(const std::string& path, std::string& text, std::string& error);

private:
    std::string brushFolder() const;
    std::string bundleFolder() const;
    void loadBrushes();

    std::string                 m_folder;
    bool                        m_brushesLoaded;
    std::vector<LevelBrushFile> m_brushes;
    std::vector<std::string>    m_brushFiles;       // the file each brush came from (or will go to)
    std::vector<std::string>    m_brushProblems;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_OBJECTLIBRARY_H
