// Owns physics materials and material-pair damage/effect responses. Header path [INFERRED].
// Preserve virtual declaration order: the concrete destructor follows all interface methods,
// unlike manager classes that place it immediately after IBase.

#ifndef PHYSICS_MATERIALMANAGER_H     // [INFERRED] guard style
#define PHYSICS_MATERIALMANAGER_H

#include <map>
#include <string>

#include "../Core/IBase.h"
#include "../Core/Ids.h"
#include "../Core/Hash.h"     // Hash<unsigned int, IMMCell*, UIntHashFunc> - the cell table

namespace dice {
namespace hfe {

namespace io { class IStream; }

namespace world {

class IMMCell;
class Material;
class IObjectTemplate;

// [UNRESOLVED] its own interface; _ZTIN4dice3hfe5world16IMaterialManagerE (0x8890CF0) is the
// recorded base and MaterialManager overrides everything, so its slots are not separately readable.
class IMaterialManager : public IBase
{
protected:
    ~IMaterialManager() {}
};

class MaterialManager : public IMaterialManager
{
public:
    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    MaterialManager();

    // Material operations. Parameter types are established; unproven return types remain [CHOSEN].
    virtual void  setAttMaterial(unsigned int id);              // + 16  0x810E810  192 B  (PE 0x5ECB10): creates + selects
    // (read for the water compile): news a Material (0x34), takes the first
    // free id in 0..254 (returns 0 when all are taken), inserts, setName(n), returns the material.
    virtual Material* createMaterial(const std::string& n);
    virtual unsigned int getAttMaterial() const;                // + 24  0x810EA50   11 B  recovered -> +0x14
    virtual void  removeMaterial(unsigned int id);
    virtual unsigned int getMaterialId(const std::string& n);
    // map lookup; a missing id falls back to material 0 (through the vtable,
    // so an override sees the retry); a missing 0 is null.
    virtual Material* getMaterialPtr(unsigned int id);

    // recovered: 28 bytes, and it is a convenience overload, not a separate lookup - it loads
    // this->m_attMaterial (+0x14) and tail-calls slot +36 with it.
    virtual Material* getMaterialPtr();                         // + 40  0x8110E30   28 B  recovered

    // recovered: 11 bytes, `add eax, 8` - it returns &m_materials.
    virtual const std::map<unsigned int, Material*>& getMaterialMap();  // + 44 0x8110E50 11 B recovered

    virtual void  setSpeedDamage(float v);                      // + 48  0x8110E60   14 B  recovered -> +0x1C
    virtual float getSpeedDamage() const;                       // + 52  0x8110E70   11 B  recovered -> +0x1C
    virtual void  setDefaultDamage(float v);                    // + 56  0x8110E80   14 B  recovered -> +0x20
    virtual float getDefaultDamage() const;                     // + 60  0x8110E90   11 B  recovered -> +0x20
    virtual void  setDefMaterial(unsigned int id);
    virtual unsigned int getDefMaterial() const;                // + 68  0x8110EA0   11 B  recovered -> +0x18

    // Cells: the (material, material) pair table
    // Two-argument forms take a material pair; the no-argument forms use the current att/def pair.
    // world::getMaterialHashKey(unsigned, unsigned) (0x810E460) is what folds the pair into the
    // Hash key below.
    virtual IMMCell* getCreateCell(unsigned int a, unsigned int b);  // + 72  0x810EB60  244 B  (PE 0x5ED2B0): selects the pair, creates
    virtual IMMCell* getCreateCell();
    virtual IMMCell* getCell(unsigned int a, unsigned int b);
    virtual IMMCell* getCell();

    virtual float getDamageMod(unsigned int a, unsigned int b);
    virtual void* getEffectTemplate(unsigned int a, unsigned int b, int i);
    virtual void* getDecalTemplate(unsigned int a, unsigned int b, int i);
    virtual IObjectTemplate* getSoundTemplate(unsigned int a, unsigned int b, int i);
    virtual void* getSound(unsigned int a, unsigned int b, int i, bool f);

    virtual float getFrictionForMaterial(unsigned int id);
    virtual float getElasticityForMaterial(unsigned int id);
    virtual float getResistanceForMaterial(unsigned int id);

    // deletes every material through its D0 slot, clears the map, then
    // killMMCell()s every cell in the hash before clearing that. See the .cpp.
    virtual void  removeAll();                                       // +120  0x810EA80  written
    virtual bool  saveMaterials(io::IStream* out);
    virtual bool  saveCells(io::IStream* out);
    virtual Material* getMaterialByName(const std::string& n);
    virtual Material* getMaterialById(unsigned int id);

    // last, not first - see the warning in the header block.
    virtual ~MaterialManager();

    static IBase* create(unsigned int iid, IBase* outer);

private:
    // layout, sizeof == 48 (0x30). Scalar offsets are read from the accessors, not inferred
    // from declaration order - the ObjectManager cull-distance incident is exactly what happens
    // when that shortcut is taken (three floats mapped to the wrong fields, compiles perfectly,
    // culls at wildly wrong distances). Each is annotated with the instruction that proves it.
    // /* +0x00 */ vptr
    /* +0x04 */ int m_refCount;                       // ctor: 1

    // Original map values are IMaterial pointers; this source uses Material pointers.
    // Any interface retyping must update the lookup return types and consumers atomically.
    /* +0x08 */ std::map<unsigned int, Material*> m_materials;   // header +0x08, node count +0x0C.
                                                                 // getMaterialMap: `add eax, 8`.

    /* +0x10 */ int m_unknown10;    // the constructor never writes it. [UNRESOLVED].

    /* +0x14 */ unsigned int m_attMaterial;    // ctor 10001. getAttMaterial: mov eax,[eax+14h]
    /* +0x18 */ unsigned int m_defMaterial;    // ctor 10001. getDefMaterial: mov eax,[eax+18h]
    /* +0x1C */ float m_speedDamage;           // ctor 0.1f  (0x3DCCCCCD). get: fld [eax+1Ch]
    /* +0x20 */ float m_defaultDamage;         // ctor 1.0f  (0x3F800000). get: fld [eax+20h]

    // Material-pair cells use 10000 hash buckets; preserve that explicit size rather than
    // the engine hash default of 3011.
    Hash<unsigned int, IMMCell*, UIntHashFunc> m_cells;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_MATERIALMANAGER_H
