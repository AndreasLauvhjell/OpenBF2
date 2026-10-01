// Shared template interface with a lazily allocated weak-reference control block.
// Preserve virtual order: IPersistant's destructor and getWeakPtr are nonvirtual.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_IOBJECTTEMPLATE_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../Core/ClassManager.h"     // for IBase
#include "../../Core/IPersistant.h"
#include "../../Core/SmartPtr.h"
#include "../../Core/WeakPtr.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

// readBinary takes an io::IStream*; a pointer parameter needs only a forward declaration, and
// pulling in the real IStream header here would drag the whole IO layer into every template header.
namespace io { class IStream; }

namespace world {

class IObject;
class IObjectTemplateComponent;

class IObjectTemplate : public IPersistant
{
public:
    // Component ownership keyed by interface ID.
    typedef std::map<unsigned int, SmartPtr<IObjectTemplateComponent> > ComponentMap;

    // the 53 slots, in slot order. Signatures identical to ObjectTemplate.h's overriders
    // (which carry the per-slot evidence notes and ELF addresses; they are not repeated here).
    // +0x00..+0x0C IBase quartet, inherited pure through IPersistant.
    virtual unsigned int getClassID() const = 0;                          // +0x10
    virtual ~IObjectTemplate();                                           // +0x14 +0x18  D1 / D0
    virtual void templateInitialize() = 0;                                // +0x1C
    virtual bool writeBinary(io::IStream*) const;                         // +0x20 (false)
    virtual bool readBinary(io::IStream*);                                // +0x24
    virtual void        setName(const std::string&) = 0;                  // +0x28
    virtual const std::string& getName() const = 0;                       // +0x2C
    virtual const std::string& getCreator() const = 0;                    // +0x30
    virtual void        setCreator(const std::string&) = 0;               // +0x34
    virtual const std::string& getModifiedByUser() const = 0;             // +0x38
    virtual void        setModifiedByUser(const std::string&) = 0;        // +0x3C
    virtual void        setId(unsigned int) = 0;                          // +0x40
    virtual unsigned int getId() const = 0;                               // +0x44
    virtual unsigned int getFlags() const = 0;                            // +0x48
    virtual unsigned int updateFlags(unsigned int set, unsigned int clear) = 0; // +0x4C
    virtual void* queryImplementation(unsigned int classId) = 0;          // +0x50
    virtual IObject* createObject() const = 0;                            // +0x54
    virtual void  setComponent(unsigned int classId, IBase* component) = 0; // +0x58
    virtual IObjectTemplateComponent* queryComponent(unsigned int) = 0;   // +0x5C (NOT const)
    virtual IObjectTemplateComponent* getComponentFromClassID(unsigned int) const = 0; // +0x60
    virtual void objectCreated(IObject*) = 0;                             // +0x64
    virtual void objectDestroyed(IObject*) = 0;                           // +0x68
    virtual void preCache() const = 0;                                    // +0x6C
    virtual void  setCullRadiusScale(float) = 0;                          // +0x70
    virtual float getCullRadiusScale() const = 0;                         // +0x74
    virtual void setPhysicsComponent(IObject*) const = 0;                 // +0x78
    virtual void removePhysicsComponent(IObject*) const = 0;              // +0x7C
    virtual void setPhysicsNodeComponent(IObject*) const = 0;             // +0x80
    virtual void removePhysicsNodeComponent(IObject*) const = 0;          // +0x84
    virtual void setResponsePhysicsComponent(IObject*) const = 0;         // +0x88
    virtual void removeResponsePhysicsComponent(IObject*) const = 0;      // +0x8C
    virtual void setNetworkableInfo(const std::string&) = 0;              // +0x90
    virtual const std::string& getNetworkableInfo() const = 0;            // +0x94
    virtual void setCollisionMesh(const std::string&);                    // +0x98 (empty)
    virtual const std::string& getCollisionMesh() const;                  // +0x9C
    virtual const Vec3& getAnchor() const = 0;                            // +0xA0
    virtual void setAnchor(const Vec3&) = 0;                              // +0xA4
    virtual const Vec3& getAnchorOffset() const = 0;                      // +0xA8
    virtual void setAnchorOffset(const Vec3&) = 0;                        // +0xAC
    virtual Vec3 getAnchorPos() const = 0;                                // +0xB0
    virtual void setIsOvergrowth(bool) = 0;                               // +0xB4
    virtual bool getIsOvergrowth() const = 0;                             // +0xB8
    // SimpleObjectTemplate::mapMaterial names the element type through the instantiated
    // std::pair/std::vector helpers in both ports: (material-name, material-id).
    virtual std::vector<std::pair<std::string, unsigned int> >* getMaterialVector() = 0; // +0xBC
    virtual ComponentMap* getComponentMap();                              // +0xC0
    virtual void setMaterial(int) = 0;                                    // +0xC4
    virtual int  getMaterial() const = 0;                                 // +0xC8
    virtual void setSpecialMaterials(unsigned int) = 0;                   // +0xCC
    virtual unsigned int getSpecialMaterials() const = 0;                 // +0xD0

    // non-virtual members
    // NOT a vtable slot - see the header block. Lazily allocates the weak-ref control block.
    WeakPtr<IObjectTemplate> getWeakPtr();                                // by value

    IObjectTemplate();                                                    // zeroes m_weakRef

protected:
    // Lazily allocated {reference count, owner} control block; destruction clears its owner.
    /* +0x04 */ void* m_weakRef;
};

// The IID this interface is registered under lives in Core/Ids.h.  Do not redeclare it `extern`
// here: the recovered ID catalog intentionally uses per-translation-unit const definitions, and an
// earlier extern declaration would change their linkage and create duplicate definitions.

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IOBJECTTEMPLATE_H
