// [INFERRED type name] Runtime descriptors expose concrete factories and instance counters.
#ifndef BFMEME_TYPEDESCRIPTOR_H
#define BFMEME_TYPEDESCRIPTOR_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include <new>
namespace dice { namespace meme {
class RuntimeObject;
class TypeDescriptor
{
public:
    explicit TypeDescriptor(const char* name);         // 0x82BB60; borrowed string
    virtual ~TypeDescriptor();                        // +00, no unlink
    virtual TypeDescriptor* getBaseType();             // +04
    virtual RuntimeObject* create();                   // +08
    virtual const char* getDescription();              // +0C, 0x835D00 proves text
    virtual void addInstance();                       // +10
    virtual void removeInstance();                    // +14
    virtual int getInstanceCount();                   // +18
    virtual unsigned int getInstanceSize();            // +1C
    bool isDerivedFrom(TypeDescriptor*);
    static TypeDescriptor* find(const char* name);

    const char* getName() const { return m_name; }
    const char* getShortName() const { return m_name + m_shortNameOffset; }
    TypeDescriptor* getNext() const { return m_next; }
private:
    TypeDescriptor* m_next;
    const char* m_name;
    unsigned int m_shortNameOffset;
};

// The emitted per-type descriptor pattern has sixteen-byte instances and one
// static count per concrete type. Instantiate only for fully recovered classes.
template<class T, class Base> class ClassTypeDescriptor : public TypeDescriptor
{
public:
    explicit ClassTypeDescriptor(const char* name) : TypeDescriptor(name) {}
    TypeDescriptor* getBaseType() override { return Base::getClassType(); }
    RuntimeObject* create() override
    {
        void* allocation = ::operator new(sizeof(T)); // 737D60, no exception suppression
        return allocation ? new(allocation) T : nullptr;
    }
    void addInstance() override { ++instanceCount; }
    void removeInstance() override { --instanceCount; }
    int getInstanceCount() override { return instanceCount; }
    unsigned int getInstanceSize() override { return sizeof(T); }
private:
    static int instanceCount;
};
template<class T, class Base> int ClassTypeDescriptor<T, Base>::instanceCount = 0;
} }
#endif
#endif
