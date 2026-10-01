// Editable level object file that preserves untouched records and surrounding text byte for byte.
// Changed values are written in the file's existing precision, indentation and newline style;
// angles compare modulo 360, so equivalent placements do not trigger rewrites.
// Properties retain retail order and omission rules. New templates and records join their class's
// section, preserving host-only blocks and paired sound triggers; deleted records lose their separator.
// The parser is engine-independent and editor-only.
#ifndef BF2EDITOR_FRAMEWORK_LEVELOBJECTFILE_H
#define BF2EDITOR_FRAMEWORK_LEVELOBJECTFILE_H

#include "Framework/EditorConfig.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

struct ObjectPlacement
{
    std::string templateName;
    float       position[3];
    float       rotation[3];    // yaw / pitch / roll in degrees, as Object.rotation
    int         layer;
    bool        hasLayer;
    int         group;          // Object.group; 0 is not written
    bool        disableChildren;
    bool        notInAI;
    bool        isOvergrowth;
    int         lightSourceMask;        // Object.setLightSourceMask
    bool        hasLightSourceMask;
    int         controlPointId;         // Object.setControlPointId (the gameplay layers' spawners)
    bool        hasControlPointId;
    // For a new record only: whether it goes inside an `if v_arg1 == host` block where the file has no
    // records of its class to follow (spawners and control points), and whether a zero rotation is
    // left out even where the file writes zeros (control points: the stock layers give them none).
    bool        hostOnly;
    bool        omitZeroRotation;
    ObjectPlacement();
};

class LevelObjectFile
{
public:
    LevelObjectFile();

    void parse(const std::string& text);
    // The file as it would be written; `order`, when given, receives the records in the order they are
    // written (deleted ones left out) - the order a parse of the result finds them in.
    std::string render(std::vector<size_t>* order = 0) const;

    size_t recordCount() const { return m_records.size(); }
    const ObjectPlacement& placement(size_t record) const { return m_records[record].current; }
    const ObjectPlacement& loadedPlacement(size_t record) const { return m_records[record].loaded; }
    bool isNew(size_t record) const { return m_records[record].isNew; }
    bool isDeleted(size_t record) const { return m_records[record].deleted; }
    bool isEdited(size_t record) const;     // its printed placement differs from the file's
    bool hasChanges() const;                // anything render() would write differently

    // The template name is fixed by the record; only position, rotation and layer change.
    void setPlacement(size_t record, const ObjectPlacement& placement);
    size_t appendRecord(const ObjectPlacement& placement);
    void setDeleted(size_t record, bool deleted);
    // The layer most of the file's records name (a gameplay layer gives all of its objects one); -1 when
    // no record names one.
    int dominantLayer() const;

    bool usesCrlf() const { return m_crlf; }

    // The number style this file writes (see above); the retail StaticObjects.con style when the file
    // has no position or rotation line to learn from. Never fewer digits than the retail writer's
    // (positions 3, rotations 1), so a hand-written "0/0/0" does not round later saves.
    struct Style
    {
        int  positionDecimals;
        int  rotationDecimals;
        bool keepZeroRotation;
        Style() : positionDecimals(3), rotationDecimals(1), keepZeroRotation(false) {}
    };
    const Style& style() const { return m_style; }

    // The editor-only preload block at the top of a level's StaticObjects.con: `if v_arg1 == BF2Editor`,
    // `run /objects/.../<template>.con` lines, `endIf`. The game skips it (GameLogic::load runs the file
    // with no argument); the retail editor's load uses it to load the level's templates first.
    bool hasPreloadBlock() const { return m_preloadEnd != static_cast<size_t>(-1); }
    // Whether the block already runs this line (case and surrounding spaces ignored).
    bool preloads(const std::string& runLine) const;
    // Lines written at the end of the block, before its endIf (templates of objects made in the
    // editor); replaces any set before.
    void setExtraPreload(const std::vector<std::string>& runLines) { m_extraPreload = runLines; }

    // Template blocks
    // The level files define a template per gameplay or ambient object beside it:
    //     ObjectTemplate.create <class> <name>
    //     ObjectTemplate.activeSafe <class> <name>
    //     ObjectTemplate.<property> <value>      ...
    // A property line's key is its property, lower-cased, and for the indexed properties
    // (setObjectTemplate <team> <template>, mapMaterial <index> ...) the index too: "setobjecttemplate 1".
    // The value is the line's text after the property, as the file writes it (quotes kept).
    struct TemplateLine
    {
        std::string key;
        std::string property;       // as written ("setObjectTemplate")
        std::string value;
    };
    bool findTemplate(const std::string& name, std::string& className) const;
    // The block's property lines in file order, edits applied (new ones last).
    std::vector<TemplateLine> templateLines(const std::string& name) const;
    // Sets a property line's value (as the file writes it); a missing line is added at the end of the
    // block. With an empty value, `property` is a line's key and that line is removed. False when the
    // file defines no such template.
    bool setTemplateValue(const std::string& name, const std::string& property, const std::string& value);
    static std::string templateKey(const std::string& property, const std::string& value);
    int editedTemplates() const;        // templates whose lines would be written differently
    // A new template block ("rem [<class>Template: <name>]", create, activeSafe, then `lines` as
    // property/value pairs - a pair with no property is a line written as it is, like the "rem -----"
    // separators of a control point); false when the file defines one of that name already. Where it
    // goes: see "New blocks" below.
    // `addTrigger`: a Sound template, written with its `Sound.addTrigger <name>` line after it, as
    // AmbientObjects.con has for each ambient sound.
    bool appendTemplate(const std::string& className, const std::string& name,
                        const std::vector<std::pair<std::string, std::string> >& lines, bool addTrigger = false);
    // The names of the templates of a class the file defines (deleted ones left out), in file order.
    std::vector<std::string> templateNamesOf(const std::string& className) const;
    bool templateHasTrigger(const std::string& name) const;
    // A template block left out (a gameplay object deleted with its template), or back in.
    bool setTemplateDeleted(const std::string& name, bool deleted);
    bool isTemplateDeleted(const std::string& name) const;
    // Renames a template the file defines (a deleted one too), in place: the block's banner, create and
    // activeSafe lines and its Sound.addTrigger line, and the banner and create line of each of its
    // records. Every other line keeps its text, and a rename back to the name the file has writes the
    // file as it was. False when the file defines no template `from`, or one named `to` already (names
    // compare without case, as the engine's do).
    bool renameTemplate(const std::string& from, const std::string& to);
    // The records of objects of a template defined elsewhere (an object template) renamed: each one's create
    // line and banner name the new template, in place. Returns how many records it renamed.
    size_t renameObjectRecords(const std::string& from, const std::string& to);

    // Editable combat-area block and manager settings. Untouched text is preserved; changed blocks
    // retain command order and numeric formatting. Blocks with unsupported commands are not rewritten.
    // [CHOSEN] The inverted command follows usedByPathFinding when set.
    struct CombatAreaData
    {
        std::string name;
        float       minimum[2];         // CombatArea.min x/z
        float       maximum[2];         // CombatArea.max x/z
        std::vector<std::pair<float, float> > points;    // CombatArea.addAreaPoint x/z, in order
        int         team;               // 0 = any team
        int         vehicles;           // the vehicle-category filter; 4 = all
        int         layer;
        bool        usedByPathFinding;
        bool        inverted;           // an exclusion
        CombatAreaData();
    };
    size_t combatAreaCount() const { return m_areas.size(); }
    const CombatAreaData& combatArea(size_t area) const { return m_areas[area].current; }
    bool isCombatAreaNew(size_t area) const { return m_areas[area].isNew; }
    bool isCombatAreaDeleted(size_t area) const { return m_areas[area].deleted; }
    bool isCombatAreaWritable(size_t area) const { return m_areas[area].writable; }
    // By name, case ignored (the engine's own lookup); deleted ones too.
    bool findCombatArea(const std::string& name, size_t& area) const;
    // False for a block the editor cannot write back (see above). The block keeps its name.
    bool setCombatArea(size_t area, const CombatAreaData& data);
    size_t appendCombatArea(const CombatAreaData& data);
    void setCombatAreaDeleted(size_t area, bool deleted);
    int editedCombatAreas() const;      // blocks written differently: changed, new or deleted
    // A block's lines as the stock files write them, each ended by `newline`.
    static std::string formatCombatArea(const CombatAreaData& data, const std::string& newline);

    struct CombatAreaSettings
    {
        bool  hasUse;                   // CombatAreaManager.use: the master switch
        bool  use;
        bool  hasDamage;                // CombatAreaManager.damage
        float damage;
        bool  hasTimeAllowedOutside;    // CombatAreaManager.timeAllowedOutside
        float timeAllowedOutside;
        CombatAreaSettings();
    };
    const CombatAreaSettings& combatAreaSettings() const { return m_areaSettings; }
    const CombatAreaSettings& loadedCombatAreaSettings() const { return m_loadedAreaSettings; }
    void setCombatAreaSettings(const CombatAreaSettings& settings) { m_areaSettings = settings; }
    bool combatAreaSettingsChanged() const;

    // The retail formats (positions %.3f, rotations %.1f unless a decimals count is given), and the
    // console's "a/b/c" form.
    static std::string formatPosition(const float position[3], int decimals = 3);
    static std::string formatRotation(const float rotation[3], int decimals = 1);
    static bool rotationPrintsAsZero(const float rotation[3], int decimals = 1);
    static bool parseTriple(const std::string& text, float out[3]);

private:
    struct Line
    {
        std::string text;
        bool        crlf;
    };
    // The lines of a record the editor writes, in the retail writer's order.
    enum Slot
    {
        SlotPosition,
        SlotRotation,
        SlotControlPointId,
        SlotLayer,
        SlotGroup,
        SlotDisableChildren,
        SlotNotInAI,
        SlotIsOvergrowth,
        SlotLightSourceMask,
        SlotCount
    };
    struct Record
    {
        size_t          firstLine;      // the rem line when there is one; npos for a new record
        size_t          createLine;
        size_t          endLine;        // one past the record's last line
        size_t          slotLine[SlotCount];    // npos where the record has none
        ObjectPlacement loaded;
        ObjectPlacement current;
        bool            isNew;
        bool            deleted;
    };

    bool placementDiffers(const Record& record) const;
    // Whether the slot's value, as printed, differs between `a` and `b`; and the text it prints as
    // for `p` ("" when this file would not write it).
    bool slotDiffers(int slot, const ObjectPlacement& a, const ObjectPlacement& b) const;
    std::string slotText(int slot, const ObjectPlacement& p, bool lineExists) const;
    void renderRecord(const Record& record, std::string& out) const;
    void appendLine(std::string& out, const std::string& text, bool crlf) const;

    std::vector<Line>        m_lines;
    bool                     m_finalNewline;    // the file's last line ended with a newline
    bool                     m_crlf;            // the newline for lines the editor writes
    std::vector<Record>      m_records;
    size_t                   m_preloadBegin;    // the block's `if` line; npos when there is none
    size_t                   m_preloadEnd;      // its `endIf` line
    std::vector<std::string> m_extraPreload;
    Style                    m_style;

    struct TemplateBlock
    {
        std::string         name;
        std::string         className;
        size_t              createLine;
        size_t              lastLine;       // the block's last ObjectTemplate line
        std::vector<size_t> propertyLines;  // file order
        bool                isNew;
        bool                deleted;
        std::vector<std::pair<std::string, std::string> > newLines;    // a new block's property lines
        size_t              triggerLine;    // its `Sound.addTrigger <name>` line; npos when none
        bool                addTrigger;     // a new block writes one
        std::string         fileName;       // the name as the file writes it ("" for a new block)
        TemplateBlock()
            : createLine(0), lastLine(0), isNew(false), deleted(false), triggerLine(static_cast<size_t>(-1)),
              addTrigger(false)
        {
        }
    };
    // The block's last line: its trigger line when that is the next statement after it.
    size_t blockEnd(const TemplateBlock& block) const;
    // `bannerPrefix`: "rem [<class>Template" unless the class's blocks spell it otherwise.
    void renderNewTemplate(const TemplateBlock& block, std::string& out, const std::string& bannerPrefix = std::string()) const;
    void renderNewRecord(const Record& record, const std::string& indent, std::string& out, bool banner = true) const;

    struct AreaBlock
    {
        size_t         createLine;      // npos for a new block
        size_t         endLine;         // one past its last CombatArea line
        CombatAreaData loaded;
        CombatAreaData current;
        bool           isNew;
        bool           deleted;
        bool           writable;        // every line of it is one the editor writes
    };
    bool areaChanged(const AreaBlock& block) const;
    // The manager line of a setting as it is now ("" when it is not written): 0 use, 1 damage,
    // 2 timeAllowedOutside.
    std::string settingLine(int setting, const CombatAreaSettings& settings) const;
    std::vector<AreaBlock> m_areas;
    CombatAreaSettings     m_areaSettings;
    CombatAreaSettings     m_loadedAreaSettings;
    size_t                 m_settingLine[3];    // where the file has each; npos when it has none
    // New blocks, rendered: each goes before line `at` (npos: the end of the file).
    struct Insertion
    {
        size_t              at;
        std::string         text;
        std::vector<size_t> records;    // the new records it writes, in order
        Insertion() : at(static_cast<size_t>(-1)) {}
    };
    static bool insertionBefore(const Insertion& a, const Insertion& b);
    void planNewBlocks(std::vector<Insertion>& inserts, std::vector<Insertion>& tail) const;
    void writeInsertion(const Insertion& insertion, std::string& out, std::vector<size_t>* order) const;
    size_t lineAfter(size_t line) const;
    std::vector<TemplateBlock>                                 m_templates;
    std::map<std::string, size_t>                              m_templateByName;   // lower-case name
    // Edits: lower-case template name -> key -> the new line (property as written, value); an empty
    // value removes the line.
    std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > > m_templateEdits;
    const TemplateBlock* templateBlock(const std::string& name) const;
    bool lineOfTemplate(size_t line, const TemplateBlock*& block, TemplateLine& parsed) const;
    // Renamed templates (renameTemplate): the lines that name them, as they are written now. Rebuilt
    // after each rename; empty when every name is the file's own.
    std::map<size_t, std::string> m_lineOverride;
    void updateRenamedLines();
    const std::string& lineText(size_t line) const;
};

// Binding the records of a loaded file to the engine's objects: for each record in file order, the
// id of the not yet used object with the same template name (case ignored) nearest the record's
// loaded position, within `tolerance` metres; 0 when there is none (the load did not keep it - for
// example a dynamic object the client load removes). Deleted and new records get 0.
struct SceneObject
{
    unsigned    id;
    std::string templateName;
    float       position[3];
};
std::vector<unsigned> bindRecords(const LevelObjectFile& file, const std::vector<SceneObject>& objects,
                                  float tolerance);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LEVELOBJECTFILE_H
