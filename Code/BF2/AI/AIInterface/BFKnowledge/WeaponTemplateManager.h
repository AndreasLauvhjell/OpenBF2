// Owns AI weapon templates.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_WEAPONTEMPLATEMANAGER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_WEAPONTEMPLATEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

#include "../../../IO/File/IFileSystem.h"   // hfe::NoCaseStringCompare - m_byName's comparator

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class WeaponTemplate;   // (`operator new(108)` in `create`); held only by pointer here

class WeaponTemplateManager
{
public:
    // Name and template pointer record: 8 bytes with the original ELF STL and 32 with PE STL.
    struct MgrTemplate
    {
        /* +0 */ std::string m_name;        // [INFERRED name]
        /* +4 */ WeaponTemplate* m_template;
    };

    WeaponTemplateManager(int kind);
    // reset, then the instance cleared. not virtual.
    ~WeaponTemplateManager();

    virtual int reset();                       // - `mov eax, 1`

    // The `weaponTemplate` console object's targets (ELF ConsoleClass223/224). Names by value
    // strings returned by value.
    std::string create(std::string name);
    void setActive(std::string name);          // - [INFERRED void]
    std::string getActive() const;
    WeaponTemplate* getActiveTemplate() const; // - getActiveWeaponTemplate's callee

    // not const (`_ZN`, not `_ZNK`) even though it only reads.
    WeaponTemplate* getTemplate(const std::string& name);

    // not declared: getAllUsedTemplates , getWorkingFileAndPath and
    // getAllUsedTemplateFiles - no caller in either image.

    static WeaponTemplateManager* m_instance;      // ds:

private:
    /* +0x00 */ // vptr
    // Template names use NoCaseStringCompare to match script lookup semantics.
    /* +0x04 */ std::map<std::string, MgrTemplate*, NoCaseStringCompare> m_byName;
    /* +0x10 */ MgrTemplate* m_active;           // [INFERRED name] create / setActive / reset
    // The constructor sets kind to six, the strength count passed to template arrays.
    /* +0x14 */ int m_kind;
    // Each template's defining .con file (`_Rb_tree<WeaponTemplate*, pair<WeaponTemplate* const
    // std::string>, ..., std::less<WeaponTemplate*>>`, cleared by reset). [INFERRED name]
    /* +0x18 */ std::map<WeaponTemplate*, std::string> m_workingFiles;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_WEAPONTEMPLATEMANAGER_H
