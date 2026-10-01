// Base authored object definition, registered with ObjectTemplateManager and serializable as console script.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_OBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_OBJECTTEMPLATE_H

#include <map>
#include <string>
#include <vector>

#include "../../Math/Vec3.h"
#include "../../Core/IBase.h"
#include "../../Core/SmartPtr.h"
#include "IObject.h"
#include "IObjectTemplate.h"
#include "../../IO/Console/IConsoleSaveable.h"
#include "../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {

// Vec3 is used by setAnchor / setAnchorOffset (demangled signatures take `dice::hfe::Vec3 const&`).
// It is already recovered in Math/Vec3.h - included above. A forward declaration was tried here
// first and conflicts, because Math/Vec3.h defines `dice::hfe::Vec3` as a typedef to
// `Math::Vec3`, not as a class.

namespace world {

class IObjectTemplateComponent;

// Preserve inherited virtual order, including IConsoleSaveable and the component-management hooks.
class ObjectTemplate : public IObjectTemplate, public io::IConsoleSaveable
{
public:
    // Owns a reference to each component, keyed by interface ID.
    typedef std::map<unsigned int, SmartPtr<IObjectTemplateComponent> > ComponentMap;

    // +0x00..+0x0C  IBase quartet (addRef / getRef / release / queryInterface), not overridden
    virtual unsigned int getClassID() const;                             // +0x10
    virtual ~ObjectTemplate();                                           // +0x14 +0x18  D1 / D0

    virtual void templateInitialize();                                   // +0x1C (stub)
    // Writes networkable info and flags, returning success.
    virtual bool writeBinary(io::IStream*) const;                        // +0x20
    virtual bool readBinary(io::IStream*);                               // +0x24

    // identity
    virtual void        setName(const std::string&);                     // +0x28
    virtual const std::string& getName() const;                          // +0x2C
    virtual const std::string& getCreator() const;                       // +0x30 (stub)
    virtual void        setCreator(const std::string&);                  // +0x34 (stub)

    // Editor author identity. The base stores nothing and returns the shared empty string.
    virtual const std::string& getModifiedByUser() const;                // +0x38 (stub)
    virtual void        setModifiedByUser(const std::string&);           // +0x3C (stub)
    virtual void        setId(unsigned int);                             // +0x40
    virtual unsigned int getId() const;                                  // +0x44

    // Flag 0x20 suppresses precaching, 0x2000 selects sound-object ids, and 0x4000 marks creation after engine init.
    // [UNRESOLVED] Original flag names. preCache reads flags through the virtual getter.
    virtual unsigned int getFlags() const;                               // +0x48
    // m_flags = ~clear & (set | m_flags) - SET is applied before clear, so a bit in both is
    // cleared. See the .cpp.
    virtual unsigned int updateFlags(unsigned int set, unsigned int clear); // +0x4C

    virtual void* queryImplementation(unsigned int classId);             // +0x50

    // Creates a live object from the authored template. Concrete templates supply the factory.
    // [INFERRED] Original return type is represented as IObject*.
    virtual IObject* createObject() const = 0;                           // +0x54
    // Sets a component by class ID; a null component removes the entry.
    virtual void  setComponent(unsigned int classId, IBase* component);  // +0x58
    virtual IObjectTemplateComponent* queryComponent(unsigned int);      // +0x5C
    // Scans component class IDs; the map itself is keyed by interface ID.
    virtual IObjectTemplateComponent* getComponentFromClassID(unsigned int) const; // +0x60

    // The template -> instance bridge. See the .cpp and IObjectTemplateComponent.h.
    // takes IObject*, not Object* - from the mangled name.
    virtual void objectCreated(IObject*);                                // +0x64
    virtual void objectDestroyed(IObject*);                              // +0x68
    // IS const, and it is the one that gained the qualifier rather than losing it.
    virtual void preCache() const;                                       // +0x6C

    virtual void  setCullRadiusScale(float);                             // +0x70
    virtual float getCullRadiusScale() const;                            // +0x74

    // Applies or removes physics on the supplied object without modifying this template.
    virtual void setPhysicsComponent(IObject*) const;                    // +0x78
    virtual void removePhysicsComponent(IObject*) const;                 // +0x7C
    virtual void setPhysicsNodeComponent(IObject*) const;                // +0x80
    virtual void removePhysicsNodeComponent(IObject*) const;             // +0x84
    virtual void setResponsePhysicsComponent(IObject*) const;            // +0x88
    virtual void removeResponsePhysicsComponent(IObject*) const;         // +0x8C

    virtual void setNetworkableInfo(const std::string&);                 // +0x90
    // `const std::string&`, not void* - 0x8117560 is `add eax,10h; retn`, i.e. it returns the
    // address of m_networkableInfo, which is how GCC returns a reference to a member. writeBinary
    // then does `[eax]` for the data pointer and `[[eax]-0Ch]` for the length - the std::string _Rep
    // layout - which confirms it independently.
    virtual const std::string& getNetworkableInfo() const;               // +0x94

    // Collision-mesh accessors are inherited directly from IObjectTemplate.

    // Base anchor getters return zero; getAnchorPos returns its value by copy.
    virtual const Vec3& getAnchor() const;                               // +0xA0 (stub)
    virtual void setAnchor(const Vec3&);                                 // +0xA4 (stub)
    virtual const Vec3& getAnchorOffset() const;                         // +0xA8 (stub)
    virtual void setAnchorOffset(const Vec3&);                           // +0xAC (stub)
    virtual Vec3 getAnchorPos() const;                                   // +0xB0 (stub)

    virtual void setIsOvergrowth(bool);                                  // +0xB4 (stub)
    virtual bool getIsOvergrowth() const;                                // +0xB8 (stub)

    // Controls the engine-wide precaching mode; this is shared state, independent of individual template flags.
    static void setIsPreCaching(bool);
    static bool getIsPreCaching();

    // Neither is const - see the note on queryImplementation.
    virtual std::vector<std::pair<std::string, unsigned int> >* getMaterialVector(); // +0xBC (stub)
    virtual ComponentMap* getComponentMap();                             // +0xC0

    virtual void setMaterial(int);                                       // +0xC4 (stub)
    virtual int  getMaterial() const;                                    // +0xC8 (stub)
    virtual void setSpecialMaterials(unsigned int);                      // +0xCC
    // `unsigned int`, not void*. The setter's parameter type IS attested (mangled name) and it
    // writes the same dword this reads back, so the pair is unsigned int. [CHOSEN] over `void*` on
    // that symmetry - the return type itself is not in the mangling.
    virtual unsigned int getSpecialMaterials() const;                    // +0xD0

    // Serializes .con text and may mutate template state while writing.
    virtual bool makeScript(io::IStream*);                               // +0xD4

    // Preserve constness: removal affects the object; setters may also modify template state.
    virtual void setGameLogicComponents(IObject*);                       // +0xD8 (stub)
    virtual void removeGameLogicComponents(IObject*) const;              // +0xDC
    virtual void setHudComponents(IObject*);                             // +0xE0 (stub)
    virtual void removeHudComponents(IObject*) const;                    // +0xE4

    ObjectTemplate();

protected:
    // Uses native STL storage. Original ELF string and map offsets must not be applied to rebuilt objects.
    // The primary interface owns the weak-reference control block.
    /* 0x0C */ std::string  m_templateName;
    /* 0x10 */ std::string  m_networkableInfo;        // a std::string, not a pointer - see the ctor
    /* 0x14 */ unsigned int m_templateId;
    /* 0x18 */ unsigned int m_flags;
    // exact type from setComponent's mangled _Rb_tree instantiation. The SmartPtr value means the
    // map owns a reference to each component. Occupies +0x1C..+0x27 in the original - see above.
    /* 0x1C */ ComponentMap m_componentMap;
    /* 0x28 */ float        m_cullRadiusScale;
    /* 0x2C */ unsigned int m_specialMaterials;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_OBJECTTEMPLATE_H
