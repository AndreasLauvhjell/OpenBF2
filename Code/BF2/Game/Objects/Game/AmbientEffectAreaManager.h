// [INFERRED] header beside the attested Game/Objects/Game/AmbientEffectArea.cpp.
// ELF 8164320..8164922 / dedicated PE 588EF0..58925B, 5895D0.
#ifndef GAME_OBJECTS_GAME_AMBIENTEFFECTAREAMANAGER_H
#define GAME_OBJECTS_GAME_AMBIENTEFFECTAREAMANAGER_H

#include <map>
#include <set>
#include <vector>

namespace dice { namespace hfe { namespace world {
class AmbientEffectArea;
class IObject;

// Non-owning indexes; no base or virtual methods. Retail set/map occupy 12 bytes
// each in both ports, but their internal layouts differ. Use native STL members.
class AmbientEffectAreaManager
{
public:
    AmbientEffectAreaManager();
    ~AmbientEffectAreaManager();
    void removeAll();
    void addAmbientEffectArea(AmbientEffectArea* area);
    void removeAmbientEffectArea(AmbientEffectArea* area);
    bool getAreasWithTriggerId(int triggerId, std::vector<IObject*>& objects);
    AmbientEffectArea* getAreaFromId(int id);

private:
    std::set<AmbientEffectArea*> m_areas;           // ELF/PE +00
    std::map<int, AmbientEffectArea*> m_areasById;  // ELF/PE +0C
};

extern AmbientEffectAreaManager* ambientEffectAreaManager;
} } }
#endif
