// Path: header [INFERRED] beside attested Core/ClassManager.cpp; declarations recovered from the ELF.
// Engine class registry and singleton factory. Preserve virtual declaration order:
// the IBase quartet precedes the destructor, followed by the registry surface.
// The 0x48 allocation is retail ELF evidence, not a portable STL-containing object size.

#ifndef CORE_CLASSMANAGER_H         // [INFERRED] guard style
#define CORE_CLASSMANAGER_H

#include <map>
#include <string>
#include <vector>

#include "IBase.h"

namespace dice {
namespace hfe {

// IBase is the root of the whole interface hierarchy; its definition moved to Core/IBase.h once a
// real call site (ObjectTemplateManager::createTemplate) needed the base relation to be visible
// rather than merely declared.

class IClassServer;

// Class factory registration. The x86 ELF record is 12 bytes; native STL layouts may differ.
struct ClassRegInfo
{
    /*  0 */ unsigned int  classId;
    /*  4 */ std::string   name;        // libstdc++ std::string is one pointer
    // Factory receives the requested IID and an optional outer IBase pointer.
    // Class ID selects the factory; IID selects the returned interface. [UNRESOLVED] original name of
    // the outer parameter; the current name reflects its aggregation role.
    /*  8 */ IBase*      (*create)(unsigned int iid, IBase* outer);
};

// Named singleton registration; x86 ELF size is 20 bytes.
// Flags: 0x1 creates during initSingletons, 0x2 creates immediately, 0x4 registers an existing object.
// Bits above 2 are [UNRESOLVED]. Existing-object entries need no IID or CID.
struct SingletonRegInfo
{
    /*  0 */ std::string   name;
    /*  4 */ IBase**       instance;    // address of the global that receives the pointer
    /*  8 */ unsigned int  iid;
    /* 12 */ unsigned int  classId;
    /* 16 */ int           flags;
};

// Provides a module's class and singleton registrations. [INFERRED] Interface extent beyond the observed callers.
class IClassServer
{
public:
    virtual void addRef() = 0;                                          // +0x00
    virtual int  getRef() = 0;                                          // +0x04
    virtual int  release() = 0;                                         // +0x08
    virtual void* queryInterface(unsigned int iid) const = 0;           // +0x0C
    virtual void getClassList(std::vector<ClassRegInfo>&) = 0;          // +0x10
    virtual void getSingletonList(std::vector<SingletonRegInfo>&) = 0;  // +0x14
};

class ClassManager
{
public:
    ClassManager();

    // IBase, vtable slots 0x00-0x0C
    virtual void  addRef();                                             // +0x00
    virtual int   getRef();                                             // +0x04
    virtual int   release();                                            // +0x08
    virtual void* queryInterface(unsigned int iid) const;               // +0x0C

    // Keep the destructor after the IBase methods. ELF uses two destructor slots; MSVC uses one.
    virtual ~ClassManager();                                            // +0x10

    // Class servers
    virtual void registerServer(IClassServer* server);                  // +0x18
    virtual void unregisterServer(IClassServer* server);                // +0x1C

    // Class registry
    virtual void setDefaultClass(unsigned int iid, unsigned int classId);       // +0x20
    virtual bool getDefaultClass(unsigned int iid, unsigned int& classId);      // +0x24
    virtual bool getClassName(unsigned int classId, std::string& name);         // +0x28
    virtual bool getClassID(const std::string& name, unsigned int& classId);    // +0x2C

    // Three overloads. createInstance2 (+0x34) is the one the engine uses most - BF2Engine's
    // setGameLogic and ClassManager::createSingleton both go through it.
    virtual IBase* createInstance(unsigned int classId, IBase* outer);                      // +0x30
    virtual IBase* createInstance(unsigned int classId, unsigned int iid, IBase* outer);     // +0x34
    virtual IBase* createInstance(std::string name, unsigned int iid, IBase* outer);         // +0x38

    // Changes a named singleton's class. Returns false when the name is unknown.
    virtual bool  setSingletonClass(const std::string& name, unsigned int classId);   // +0x3C
    virtual bool  getSingletonClass(const std::string& name, unsigned int& classId);  // +0x40  0x83bbc20 (7 bytes - a stub)
    virtual void  registerSingleton(const SingletonRegInfo& info);                    // +0x44
    virtual void  unregisterSingleton(const std::string& name);                       // +0x48
    virtual IBase* getSingleton(const std::string& name);                             // +0x4C
    virtual void*  getSingletonAs(const std::string& name, unsigned int iid);          // +0x50

    // "simple" singletons: raw void*, no refcounting, no class id
    virtual void  registerSimpleSingleton(const std::string& name, void* p);          // +0x54
    virtual void  unregisterSimpleSingleton(const std::string& name);                 // +0x58
    virtual void* getSimpleSingleton(const std::string& name) const;                  // +0x5C

    // Diagnostics
    virtual std::string getPrintableSingletonList() const;                            // +0x60
    // const - see note 3 in the file header.
    virtual void getAllClassIncludingSubstring(const std::string& sub,
                                               std::vector<std::string>& out) const;   // +0x64

    // Instantiates every singleton whose flags have bit 0 set, then latches m_singletonsInit.
    virtual void initSingletons();                                                     // +0x68

    // Non-virtual helpers
    // Forward-declared here because the three getClassInfo/createSingleton declarations below
    // take them by reference; their definitions follow under "nested records". A member function
    // parameter type is not in a complete-class context, so the plain definitions-after-use order
    // does not compile.
    struct ClassInfo;
    struct SingletonInfo;

    void addServer(IClassServer* server);
    bool findServer(IClassServer* server);
    void removeServer(IClassServer* server);
    bool getClassInfo(unsigned int classId, ClassInfo& out);
    bool getClassInfo(const std::string& name, ClassInfo& out);
    void createSingleton(const std::string& name, SingletonInfo& info);

    // Nested records
    // sizeof(ClassInfo) == 16: registerServer stores create/classId/name/server in that order and
    // advances the vector cursor by 4 ints (`*this->classInfos = v5 + 4`).
    struct ClassInfo
    {
        // Same correction as ClassRegInfo::create above - registerServer copies the factory pointer
        // straight across (`info.create = c->create;`), so the two types must agree.
        /*  0 */ IBase*      (*create)(unsigned int iid, IBase* outer);
        /*  4 */ unsigned int  classId;
        /*  8 */ std::string   name;
        /* 12 */ IClassServer* server;

        // Unregistration matches names case-sensitively; lookup uses case-insensitive comparison.
        // The member operator spelling is [CHOSEN, not recovered]; comparison semantics are observed.
        bool operator==(const std::string& n) const { return name == n; }
    };

    // sizeof(SingletonInfo) == 16. Offsets proven from createSingleton, which is handed
    // (node+16, node+20) out of a _Rb_tree node, and from initSingletons testing node+32 == +12.
    struct SingletonInfo
    {
        /*  0 */ IBase**       instance;   // note the double indirection: *instance is the global
        /*  4 */ unsigned int  iid;
        /*  8 */ unsigned int  classId;
        /* 12 */ int           flags;
    };

private:
    // Offsets describe the 72-byte x86 ELF layout. Its legacy maps are 12 bytes with a heap header;
    // do not apply these offsets to modern STL layouts. Vectors contain three pointers.
    /*  0 */ void*         pVMT;
    /*  4 */ int           m_refCount;
    // from the `int m_classServerTree; int m_dwordC; char m_gap10[4]` triple.
    // it is A MAP, not A SET, and the survey's `std::set<IClassServer*>` was wrong. addServer,
    // findServer and removeServer all instantiate
    //     _Rb_tree<IClassServer*, pair<IClassServer* const, std::string>, ...>
    // - a std::map keyed by the server pointer with a std::string value. addServer inserts an empty
    // string for every server and nothing in the binary ever reads the value back.
    // [UNRESOLVED] what the string was for. A module name is the obvious guess and is not claimed.
    /*  8 */ std::map<IClassServer*, std::string> m_classServerTree;    // 8..19
    // Maps interface IDs to their default class IDs.
    /* 20 */ std::map<unsigned int, unsigned int> m_defaultClasses;   // 20..31, iid -> classId
    // Container declarations replace their original padding; do not retain both when changing layouts.
    /* 32 */ std::vector<ClassInfo> m_classInfos;               // 32..43 (3 pointers)
    /* 44 */ std::map<std::string, SingletonInfo> m_normalSingletons;  // 44..55
    /* 56 */ bool          m_singletonsInit;     // latched true by initSingletons()
    // Named borrowed pointers; this registry does not own or release its entries.
    /* 60 */ std::map<std::string, void*> m_simpleSingletons;   // 60..71
};

} // namespace hfe
} // namespace dice

#endif // CORE_CLASSMANAGER_H
