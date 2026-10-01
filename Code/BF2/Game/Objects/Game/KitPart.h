// Path: [INFERRED] beside Kit.cpp; symbolized ELF KitPart translation-unit group.
#ifndef GAME_OBJECTS_GAME_KITPART_H
#define GAME_OBJECTS_GAME_KITPART_H
#include "../Base/Bundle.h"
#include "../Base/BundleTemplate.h"
#include "../IKitPart.h"
#include <map>

namespace dice { namespace hfe { namespace world {

class KitPartTemplate : public BundleTemplate
{
public:
    KitPartTemplate();
    virtual ~KitPartTemplate();
    virtual unsigned int getClassID() const;
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual void setBoneName(std::string); // ELF ESs and PE retn 0x1C: by value
    virtual const std::string& getBoneName();
    virtual short getBoneNameIndex();      // PE materializes AX, ELF sign extends the short
    virtual void setCopyLinksCount(int);
    virtual int getCopyLinksCount();

    // ELF +0x17A uses base tail padding; PE begins the short at +0x22C.
    // Native fields follow the compiler's own base layout; no raw retail STL offsets.
    short m_boneNameIndex;                // retail leaves this uninitialized
    int m_copyLinksCount;                // retail leaves this uninitialized
    std::map<int, std::string> m_teamGeometry; // ELF +0x180 / PE +0x234
    int m_kitPartIndex;                   // ELF +0x18C / PE +0x240: -1
    int m_rank;                           // ELF +0x190 / PE +0x244: 1
};

class KitPart : public Bundle, public IKitPart
{
public:
    explicit KitPart(const KitPartTemplate*);
    virtual ~KitPart();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual int getRank() const;
    virtual int getKitPartIndex();
    void setTeam(int);
private:
    int m_refCount;                     // ELF +0x140 / PE +0x158
    // Shipped defect: neither constructor initializes this extra template pointer.
    // Rank/index read it; setTeam instead uses Object's initialized template pointer.
    const KitPartTemplate* m_kitPartTemplate; // ELF +0x144 / PE +0x15C
    int m_team;                         // ELF +0x148 / PE +0x160; uninitialized until setTeam
};

} } }
#endif
