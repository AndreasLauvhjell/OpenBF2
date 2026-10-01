// Composite template containing child templates and their relative placement.
// ObjectTemplate.addTemplate builds this hierarchy. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_BUNDLETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_BUNDLETEMPLATE_H

#include "SimpleObjectTemplate.h"
#include "../../../Math/Mat4.h"        // getTransformation / setTransformation take a Mat4
#include <string>
#include <vector>

namespace dice {
namespace hfe {
class HudInformationLayer;
namespace world {

struct BaseTemplateConsoleAccess;
class KitTemplate;

// RTTI and the secondary vtable in both ports prove a direct IBase-derived interface. It has no
// virtual destructor; its first four slots are the inherited IBase quartet.
class IBundleTemplate : public IBase
{
public:
    virtual void addTemplate(const std::string&) = 0;                         // +0x10
    virtual void removeTemplate(int) = 0;                                     // +0x14
    virtual int getNoTemplates() const = 0;                                   // +0x18
    virtual void setTransformation(const Mat4&, int) = 0;                     // +0x1C
    virtual std::string getTemplate(int) const = 0;                           // +0x20
    virtual const Mat4& getTransformation(int) const = 0;                     // +0x24
    virtual void renameChildTemplate(const std::string&, const std::string&) = 0; // +0x28

protected:
    ~IBundleTemplate() {}
};

class BundleTemplate : public SimpleObjectTemplate, public IBundleTemplate
{
    friend struct BaseTemplateConsoleAccess;       // recovered setPosition/setRotation executors
    friend class ::dice::hfe::HudInformationLayer;
    friend class KitTemplate;
public:
    // One child entry. attested as a nested class by its own RTTI record,
    // _ZTIN4dice3hfe5world14BundleTemplate12TemplateInfoE.
    // EffectBundleTemplate::EffectInfo derives from it - the only recovered subclass of a nested
    // template type anywhere in the hierarchy - which is how effect bundles attach extra per-child
    // data.
    // Both ports prove its name, weak template cache, input id, transform, trailing bool, and all
    // constructor defaults. Only the semantic name of the final bool remains unresolved.
    class TemplateInfo
    {
    public:
        explicit TemplateInfo(const std::string& templateName);
        virtual ~TemplateInfo();

        std::string              m_templateName;
        WeakPtr<IObjectTemplate> m_template;
        int                      m_inputId;
        Mat4                     m_transformation;
        bool                     m_unknown50;       // [UNRESOLVED] ELF +0x50 / PE +0x68
    };

    // Preserve the inherited virtual prefix and appended bundle operations.
    // Return-type qualifications follow the base declarations and individual annotations.
public:
    BundleTemplate();

    virtual void  addRef();                                            // +0x00
    virtual int   getRef();                                            // +0x04
    virtual int   release();                                           // +0x08
    virtual void* queryInterface(unsigned int iid) const;              // +0x0C
    // 10 bytes: `mov eax, ds:CID_...; retn` — returns a CID, not an IID.
    virtual unsigned int getClassID() const;                           // +0x10
    virtual ~BundleTemplate();                                         // +0x14 +0x18

    virtual bool writeBinary(io::IStream*) const;                      // +0x20
    virtual bool readBinary(io::IStream*);                             // +0x24
    virtual void* queryImplementation(unsigned int cid);               // +0x50
    virtual IObject* createObject() const;                             // +0x54
    virtual bool makeScript(io::IStream*);                             // +0xD4
    // Creates the sound template through the base, attaches it as a child, and returns it.
    virtual ObjectTemplate* addSoundTemplate(const std::string&);      // +0x110

    // the eleven BundleTemplate adds
    // These two are what actually assemble a bundle at runtime: a bundle template creates its
    // object, then attaches one child object per TemplateInfo. They are const, and they mutate the
    // object rather than the template — the same asymmetry ObjectTemplate.h records for the whole
    // set*Component/remove*Component family.
    virtual void addBundleChilds(IObject*) const;                      // +0x114
    virtual void addDriverDecals(IObject*) const;                      // +0x118

    // Writes one child's .con block. The TemplateInfo* is the child being serialised.
    virtual bool makeMemberScript(const TemplateInfo*, io::IStream*);  // +0x11C ELF81D8EC0 / PE5EE710

    // The console registration metadata confirms a void return. Selection is communicated through
    // the static mActiveMemberId instead.
    virtual void addTemplate(const std::string& templateName);         // +0x120
    virtual void removeTemplate(int index);                            // +0x124
    // Console metadata and both ABIs confirm std::string by value.
    virtual std::string getTemplate(int index) const;                  // +0x128 , by value
    // , and it subtracts rather than reading a stored count — see the m_templateInfos note below.
    virtual int   getNoTemplates() const;                              // +0x12C

    virtual void  setTransformation(const Mat4&, int index);           // +0x130
    // — too small to build a Mat4 by value, so it returns a reference into the TemplateInfo.
    virtual const Mat4& getTransformation(int index) const;            // +0x134

    virtual void  renameChildTemplate(const std::string& from,
                                      const std::string& to);          // +0x138
    // Allocates and constructs one TemplateInfo; both ports expose the exact body.
    virtual TemplateInfo* createTemplateInfo(const std::string&);      // +0x13C

    // Non-virtual console/helper surface; symbols and constness are attested in the ELF.
    void setInputId(int inputId, int index);
    void setPosition(const Vec3&, int index);
    void setRotation(const Vec3&, int index);
    std::string listTemplates() const;
    void clearTemplateList();
    Vec3 getRotation(int index) const;
    Vec3 getPosition(int index) const;
    void setActiveTemplate(int index);
    // [CHOSEN source wrapper, recovered effect] DefaultFollowComp directly tests this shipped
    // byte (ELF +0x179 / PE +0x229); the wrapper keeps that leaf out of BundleTemplate's layout.
    bool getKillHeatSeekingMissile() const { return m_killHeatSeekingMissile; }

protected:
    friend class Bundle;
    // layout, offsets 356..379. SimpleObjectTemplate occupies 0..355.
    // The IBundleTemplate vptr is supplied by the real secondary base above.
    /* 360 */ int m_refCount2;

    // Child-template records are held by pointer in a native vector; the third vector word is capacity, not count.
    /* 364 */ std::vector<TemplateInfo*> m_templateInfos;

    // Console-authored destruction flag, serialized here; its destruction-time consumer is unresolved.
    /* 376 */ bool m_explodePartAtDestroy;
    /* 377 */ bool m_killHeatSeekingMissile;   // read by DefaultFollowComp; console +377 (198)
    // +378..379 are compiler padding and are not written by either constructor.

    static int mActiveMemberId;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_BUNDLETEMPLATE_H
