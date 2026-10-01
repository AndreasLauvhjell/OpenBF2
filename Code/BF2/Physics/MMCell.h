// Material-pair response: damage modifier plus indexed effect, decal and sound templates.
// Header path [INFERRED]. IMMCell has a pure virtual destructor with a required definition.
// One- and two-argument setters are distinct virtual overloads; preserve declaration order.
// MSVC reverses adjacent overload slots relative to ELF. getCellSound is nonvirtual.
// Return types marked [CHOSEN] remain subject to caller verification.

#ifndef PHYSICS_MMCELL_H              // [INFERRED] guard style
#define PHYSICS_MMCELL_H

#include <map>

namespace dice {
namespace hfe {
namespace world {

class IObjectTemplate;

class IMMCell
{
public:
    // pure, not just virtual - slots 0/1 in IMMCell's own vtable are __cxa_pure_virtual. C++
    // requires a body for a pure destructor anyway, and the ELF has one (0x810D630); an inline
    // empty body reproduces it.
    virtual ~IMMCell() = 0;                                                     // slots 0/1

    virtual void  setDamageMod(float mod)                               = 0;    // slot  2
    virtual float getDamageMod()                                        = 0;    // slot  3

    virtual void  setEffectTemplate(IObjectTemplate* t)                 = 0;    // slot  4
    virtual void  setEffectTemplate(int index, IObjectTemplate* t)      = 0;    // slot  5
    virtual IObjectTemplate* getEffectTemplate(int index, bool b)       = 0;    // slot  6

    virtual void  setDecalTemplate(IObjectTemplate* t)                  = 0;    // slot  7
    virtual void  setDecalTemplate(int index, IObjectTemplate* t)       = 0;    // slot  8
    virtual IObjectTemplate* getDecalTemplate(int index, bool b)        = 0;    // slot  9

    virtual void  setSoundTemplate(IObjectTemplate* t)                  = 0;    // slot 10
    virtual void  setSoundTemplate(int index, IObjectTemplate* t)       = 0;    // slot 11
    virtual IObjectTemplate* getSoundTemplate(int index, bool b)        = 0;    // slot 12

    // [CHOSEN] return. MaterialManager::getSound (+104) forwards this one and the tree already
    // declares that as void*; MMCell::CellSound is the nested type the body actually deals in.
    virtual void* getSound(int index, bool b)                           = 0;    // slot 13
};

inline IMMCell::~IMMCell() {}

// world::MMCell - the concrete cell, 0x2C bytes. See the header block for the evidence.
class MMCell : public IMMCell
{
public:
    // The sound map's value: the template bound by setSoundTemplate and the sound instance
    // getSound manufactures from it on first use (null until then).
    struct CellSound
    {
        IObjectTemplate* soundTemplate;     // +0x00
        void*            sound;             // +0x04 - [UNRESOLVED type] the sound manager's object
    };

    MMCell();
    virtual ~MMCell();

    virtual void  setDamageMod(float mod);
    virtual float getDamageMod();

    virtual void  setEffectTemplate(IObjectTemplate* t);                        // PE 0x5EE020 (ELF 0x8110D20): (0, t)
    virtual void  setEffectTemplate(int index, IObjectTemplate* t);
    virtual IObjectTemplate* getEffectTemplate(int index, bool fallback);

    virtual void  setDecalTemplate(IObjectTemplate* t);                         // PE 0x5EE040 (ELF 0x8110D50): (0, t)
    virtual void  setDecalTemplate(int index, IObjectTemplate* t);
    virtual IObjectTemplate* getDecalTemplate(int index, bool fallback);

    virtual void  setSoundTemplate(IObjectTemplate* t);                         // PE 0x5EE060 (ELF 0x8110D80): (0, t)
    virtual void  setSoundTemplate(int index, IObjectTemplate* t);
    virtual IObjectTemplate* getSoundTemplate(int index, bool fallback);

    virtual void* getSound(int index, bool fallback);

private:
    // [CHOSEN access mechanism] MaterialManager::saveCells reads the concrete maps
    // and damage modifier directly (ELF 0x810FB90 / PE 0x5ECC10), with no virtual accessors.
    friend class MaterialManager;

    // The non-virtual helper both sound getters share: the CellSound for `index`, or for 0 when
    // `fallback` and `index` has no template, or null. PE 0x5EC710 ().
    CellSound* getCellSound(int index, bool fallback);

    std::map<unsigned int, IObjectTemplate*> m_effectTemplates;     // +0x04
    std::map<unsigned int, IObjectTemplate*> m_decalTemplates;      // +0x10
    std::map<unsigned int, CellSound>        m_soundTemplates;      // +0x1C
    float                                    m_damageMod;           // +0x28 - ctor -1.0f
};

// Deletes a const material cell through its virtual destructor; null is harmless.
// IMMCell is not reference-counted. Defined out of line in MaterialManager.cpp.
void killMMCell(const IMMCell* cell);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_MMCELL_H
