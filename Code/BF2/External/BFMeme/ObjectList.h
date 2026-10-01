// Concrete collection at839780/table94EBA0; class/path/member names [INFERRED].
// Complete22-slot ObjectCollection subclass, not a RuntimeObject descriptor.
#ifndef BFMEME_OBJECTLIST_H
#define BFMEME_OBJECTLIST_H
#include "ObjectCollection.h"
#if !DEDICATED
namespace dice { namespace meme {
class ObjectList : public ObjectCollection
{
public:
    explicit ObjectList(TypeDescriptor*);
    ~ObjectList() override;                              // 8397B0, link-only recursive deletion
    bool clear(System*) override;                       // 8397E0, payload deletion first
    ObjectCollectionPosition* first() override;
    ObjectCollectionPosition* last() override;
    ObjectCollectionPosition* next(ObjectCollectionPosition*) override;
    ObjectCollectionPosition* previous(ObjectCollectionPosition*) override;
    int size() override;                                // 839890, counts links
    Object* getObject(ObjectCollectionPosition*) override;
    bool setObject(ObjectCollectionPosition*, Object*) override; // 8398D0, no type check
    TypeDescriptor* getObjectType() override;
    bool append(System*, Object*) override;
    bool insert(System*, ObjectCollectionPosition*, Object*) override;
    void remove(System*, ObjectCollectionPosition*) override; // 839A70, link-only
    bool readObjects(System*, IStream*) override;
protected:
    struct Link;
    Link* m_first;                                       // +04
    Link* m_last;                                        // +08, deliberately stale after clear
    Link* m_cursor;                                      // +0C [INFERRED] role
    TypeDescriptor* m_type;                              // +10, borrowed
};
} }
#endif
#endif
