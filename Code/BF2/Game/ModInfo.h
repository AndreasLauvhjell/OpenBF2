// Mod and map metadata registry, constructed during module singleton registration.
// Owns the mod list and game-mode display-name map. XML parsing entry points are
// defined in ModInfo.cpp; consult their current bodies for supported content.

#ifndef GAME_MODINFO_H     // [INFERRED] guard style
#define GAME_MODINFO_H

#include <map>
#include <string>
#include <vector>

#include "IModInfo.h"

namespace dice {
namespace hfe {

class ModInfo : public IModInfo
{
public:
    ModInfo();

    // the IBase quartet, +0x00..+0x0C
    virtual void  addRef();                                                 // +0x00
    virtual int   getRef();                                                 // +0x04
    virtual int   release();                                                // +0x08
    virtual void* queryInterface(unsigned int iid) const;                   // +0x0C

    // the IModInfo surface, +0x10..+0x4C. order frozen - see Game/IModInfo.h.
    virtual bool              init();                                       // +0x10
    virtual std::vector<Mod>* getModList();                                 // +0x14
    virtual const Map*        getMap(const std::string& map) const;         // +0x18
    virtual const Mod*        getCurrentMod() const;                        // +0x1C
    virtual std::string       getCurrentModPath();                          // +0x20
    virtual const Map*        findMapFromCurrentMod(const std::string& map);// +0x24
    virtual const Map*        findMapFromMod(const Mod* mod,
                                             const std::string& map);       // +0x28
    virtual const Mod*        getModFromName(const std::string& name);      // +0x2C
    virtual const Mod*        getModFromPath(const std::string& path);      // +0x30
    virtual const Map*        getMapFromGameSpyId(int id);                  // +0x34
    virtual std::string       getMapNameFromMap(const std::string& map);    // +0x38
    virtual std::string       getMapPathFromMap(const std::string& map);    // +0x3C
    virtual std::string       getMapNameFromMap(const Mod* mod,
                                                const std::string& map);    // +0x40
    virtual std::string       getMapPathFromMap(const Mod* mod,
                                                const std::string& map);    // +0x44
    virtual void              addNameForGameMode(const std::string& gameMode,
                                                 const std::string& name);  // +0x48
    virtual std::string       getNameFromGameMode(const std::string& gameMode);
                                                                            // +0x4C

    // ModInfo's own two slots. must stay in this order and must stay last.
    virtual bool              parseMapDesc(Mod* mod, const std::string& levelsPath, Map* map);
                                                                            // +0x50  , ~4000 B
    virtual ~ModInfo();                                                     // +0x54 / +0x58

    // non-virtual
    // Called directly from init (0x82F9FA9), not through the vtable. See the vtable note above.
    bool parseModDesc(Mod* mod);

    // Factory entry for the requested interface; aggregation uses the outer parameter.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    int                                m_refCount;        // +0x04
    std::vector<Mod>                   m_mods;            // +0x08
    bool                               m_initialised;     // +0x14
    std::map<std::string, std::string> m_gameModeNames;   // +0x18
};

} // namespace hfe
} // namespace dice

#endif // GAME_MODINFO_H
