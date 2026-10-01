// Path/type and method names [INFERRED] beside the attested BFMeme cluster.
// Complete22-slot table94F3E0; concrete list94EBA0 establishes pointer positions.
#ifndef BFMEME_OBJECTCOLLECTION_H
#define BFMEME_OBJECTCOLLECTION_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
class IStream;
class OStream;
// Opaque position identity [INFERRED].839850/839870/8398B0 dereference a
// concrete list position's next/previous/Object* fields; base methods only pass it.
struct ObjectCollectionPosition;
class ObjectCollection
{
public:
    ObjectCollection();
    virtual ~ObjectCollection();
    virtual bool clear(System*);
    virtual ObjectCollectionPosition* first();
    virtual ObjectCollectionPosition* last();      // +0C 83C4F0
    virtual ObjectCollectionPosition* next(ObjectCollectionPosition*);
    virtual ObjectCollectionPosition* previous(ObjectCollectionPosition*);
    virtual int size();
    virtual int countBefore(ObjectCollectionPosition*); // +1C 83C5A0
    virtual int countAfter(ObjectCollectionPosition*);
    virtual Object* getObject(ObjectCollectionPosition*);
    virtual bool setObject(ObjectCollectionPosition*, Object*);
    virtual Object* getObjectAt(int index);         // +2C 83C640
    virtual int indexOf(ObjectCollectionPosition*); // +30 83C660, -1 on miss
    virtual ObjectCollectionPosition* positionAt(int index);
    virtual ObjectCollectionPosition* find(Object*);
    virtual TypeDescriptor* getObjectType();        // +3C 82E490 -> Object type
    virtual bool append(System*, Object*);
    virtual bool insert(System*, ObjectCollectionPosition*, Object*);
    virtual void remove(System*, ObjectCollectionPosition*);
    virtual const char* getObjectName(System*, ObjectCollectionPosition*); // +4C 83C740
    virtual bool readObjects(System*, IStream*);
    virtual bool writeObjects(System*, OStream*);
    bool read(System*, IStream*);                  // 626BF0 -> virtual+50
    bool write(System*, OStream*);                 // 83C790 -> virtual+54
};
} }
#endif
#endif
