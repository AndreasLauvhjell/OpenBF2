// Registry and active-template selection behind ObjectTemplate console commands. Header path [INFERRED].
// Preserve overload order and reference-returning string accessors. Binary cache read/write are no-ops.
// IEventListener is a secondary base, so event registration requires its adjusted pointer.
// [UNRESOLVED] exact IObjectTemplateManager interface boundary; the current IBase plus listener
// layout avoids inventing which observed virtual methods belonged to that pure interface.

#ifndef GAME_OBJECTS_OBJECTTEMPLATEMANAGER_H     // [INFERRED] guard style
#define GAME_OBJECTS_OBJECTTEMPLATEMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../../../Core/Hash.h"
#include "../../../../Core/IBase.h"
#include "../../../../Game/IEventListener.h"     // EventCategory, EventNode, IEventListener

namespace dice {
namespace hfe {
namespace world {

class IObjectTemplate;
class IObject;

// Template-name lookup uses a complete Hash object with a case-insensitive string hash.
// Its original x86 fields are bucket pointer, capacity and hash-function pointer; keep the layout intact.

// Keep the primary 37-slot order and secondary IEventListener base unchanged.
class ObjectTemplateManager : public IBase, public IEventListener
{
public:
    ObjectTemplateManager();

    // IBase quartet, slots +0x00..+0x0C
    virtual void  addRef();                                          // +0x00
    virtual int   getRef();                                          // +0x04
    virtual int   release();                                         // +0x08
    virtual void* queryInterface(unsigned int iid) const;            // +0x0C

    // Registration
    virtual void setEngineIsInitialized();                           // +0x10  0x83256d0  (no param)
    virtual void registerTemplateClass(unsigned int classId,
                                       const std::string& name);     // +0x14
    virtual void getTemplateClasses(
        std::list<std::pair<unsigned int, std::string> >& out) const;            // +0x18
    // both return A reference - see the correction block at the top of this file.
    virtual const std::string& getClassNameFromTemplateClassID(unsigned int) const; // +0x1C
    virtual unsigned int getClassIDFromTemplateClassName(const std::string&) const; // +0x20

    // Lookup: four overloads here, a fifth out of sequence at +0x60, all const
    virtual IObjectTemplate* getTemplate(const std::string& name) const;          // +0x24
    virtual IObjectTemplate* getTemplate(const std::string& name,
                                         unsigned int classId) const;             // +0x28
    virtual IObjectTemplate* getTemplate(IObject* obj,
                                         unsigned int classId) const;             // +0x2C
    virtual IObjectTemplate* getTemplate(const IObjectTemplate* t,
                                         unsigned int classId) const;             // +0x30

    virtual bool renameTemplate(const std::string& from,
                                const std::string& to);              // +0x34
    virtual void deleteAll(bool);                                    // +0x38
    virtual void deleteAllNotUsed();                                 // +0x3C
    virtual bool deleteTemplate(const std::string& name);            // +0x40
    virtual bool deleteTemplate(IObjectTemplate*);                   // +0x44

    virtual IObject* getObjectOfCIDAndInputId(IObject* obj, unsigned int cid, int id) const; // +0x48
    virtual IObject* getObjectOfIIDAndInputId(IObject* obj, unsigned int iid, int id) const; // +0x4C

    // The active-template cursor
    // This is the whole reason `.con` files work the way they do: `ObjectTemplate.create` and
    // `ObjectTemplate.active` set the cursor, and every subsequent property line applies to it.
    virtual IObjectTemplate* getActiveObjectTemplate() const;        // +0x50
    virtual void setActiveObjectTemplate(const IObjectTemplate*);    // +0x54

    // Creates a template by numeric class id or class name; preserve the overloaded method name.
    virtual IObjectTemplate* createTemplate(unsigned int classId,
                                            const std::string& name);       // +0x58
    virtual IObjectTemplate* createTemplate(const std::string& className,
                                            const std::string& name);       // +0x5C

    // The fifth getTemplate overload, out of sequence: by numeric template id.
    virtual IObjectTemplate* getTemplate(unsigned int templateId) const;    // +0x60

    virtual std::list<IObjectTemplate*>* getGlobalTemplateList();    // +0x64

    // Which .con file defined which template
    virtual bool setWorkingFolderAndFile(const std::string& templateName,
                                         const std::string& file);   // +0x68
    virtual const std::string& getWorkingFolderAndFile(
        const std::string& templateName) const;                      // +0x6C

    virtual void getAllUsedTemplates(
        std::vector<const IObjectTemplate*>& out) const;             // +0x70
    virtual void preCacheObjects() const;                            // +0x74

    // both empty in this build - the dedicated server can neither write nor read a binary
    // template cache. 0x8324c60 is 16 bytes of prologue/epilogue and 0x8324c70 is a 5-byte stub.
    virtual void writeBinary(const std::string& file) const;         // +0x78  0x8324c60 (empty)
    virtual void readBinary(const std::string& file);                // +0x7C  0x8324c70 (empty)
    virtual void setDisplayError(bool);                              // +0x80
    virtual unsigned int getClassID() const;                         // +0x84

    virtual ~ObjectTemplateManager();                                // +0x88 / +0x8C  D1/D0

    // The IEventListener override. Present twice in the binary: as the real body in the primary
    // vtable at +0x90 (0x8323790, a 5-byte empty stub) and as a `this-4` thunk in the secondary
    // vtable (0x8324d80). The manager subscribes to ECCore in its constructor and then ignores
    // every event it receives - the registration is what matters, not the handler.
    virtual void handleEvent(EventCategory cat, unsigned int id,
                             EventNode* node, void* data);           // +0x90

private:
    // Offsets describe the original 84-byte ELF object. Two interface bases supply its
    // vptrs; native map/list layouts determine this build's size.
    /* 0x08 */ int   m_refCount;                              // ctor sets 1 - the one-reference rule

    /* 0x0C */ std::map<unsigned int, const IObjectTemplate*> m_objTemplateTree;  // by template id
    /* 0x18 */ std::map<unsigned int, std::string>            m_workingFolders;   // id -> .con file

    /* 0x24 */ unsigned int m_objectTemplateIdCounter;        // ctor: 0
    // ctor: 0x1000000, not zero. The sound-object id space starts at 16,777,216 so that a sound
    // template id can never collide with an ordinary one - flag 0x2000 on the template selects it.
    /* 0x28 */ unsigned int m_soundObjectTemplateIdCounter;
    /* 0x2C */ bool         m_displayError;                   // ctor: false

    /* 0x30 */ std::list<IObjectTemplate*> m_globalTemplateList;

    // The template-name index: a real Hash object, 12 bytes, 3011 buckets. See the note above.
    /* 0x34 */ Hash<std::string, const IObjectTemplate*, StringHashNoCaseHashFunc>
                   m_objTemplateNameHashes;

    /* 0x40 */ std::map<unsigned int, std::string> m_tempClasses;   // CID -> template class name

    /* 0x4C */ const IObjectTemplate* m_activeTemplate;       // ctor: 0
    /* 0x50 */ bool                   m_engineInitialized;    // ctor: false
};

extern ObjectTemplateManager* objectTemplateManager;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_OBJECTTEMPLATEMANAGER_H
