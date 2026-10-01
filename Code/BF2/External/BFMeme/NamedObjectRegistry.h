// Path/class names [INFERRED] in attested BFMeme cluster.
// Complete 23-slot name-tree interface (PE tables9497F0/949850/9498B0/94ECE0).
#ifndef BFMEME_NAMEDOBJECTREGISTRY_H
#define BFMEME_NAMEDOBJECTREGISTRY_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
namespace dice { namespace meme {
class Object;
class System;
class Stream;
class NameTreeNode
{
public:
    NameTreeNode() : next(nullptr) {}
    virtual ~NameTreeNode();
    virtual const char* getName();
    virtual bool setName(const char*);
    virtual Object* getObject();
    virtual void setPayload(void*); // shared erased payload, container/entry differ
    virtual bool isContainer();
    virtual NameTreeNode* getChildren();
    virtual void setChildren(NameTreeNode*);
    virtual bool save(System*);
    virtual bool contains(Object*);
    virtual bool insert(System*, Object*, const char* container);
    virtual NameTreeNode* createContainer(System*, const char* path);
    virtual bool eraseObject(System*, Object*);
    virtual bool eraseNode(System*, NameTreeNode*);
    virtual bool preClose(System*);
    virtual bool close(System*);
    virtual Object* findObject(System*, const char* path);
    virtual NameTreeNode* findEntry(System*, Object*);
    virtual NameTreeNode* findChild(const char* name);
    virtual NameTreeNode* findParent(Object*);
    virtual bool objectPath(Object*, char* buffer, int capacity);
    virtual bool nodePath(NameTreeNode*, char* buffer, int capacity);
    virtual bool stream(System*, Stream*);
    NameTreeNode* next; // +4; owned successor chain, not STL
};
class NameTreeContainer : public NameTreeNode
{
public:
    explicit NameTreeContainer(const char* name = nullptr);
    ~NameTreeContainer() override;
    const char* getName() override;
    bool setName(const char*) override;
    void setPayload(void*) override;
    bool isContainer() override;
    NameTreeNode* getChildren() override;
    void setChildren(NameTreeNode*) override;
    bool save(System*) override;
    bool contains(Object*) override;
    bool insert(System*, Object*, const char*) override;
    NameTreeNode* createContainer(System*, const char*) override;
    bool eraseObject(System*, Object*) override;
    bool eraseNode(System*, NameTreeNode*) override;
    bool preClose(System*) override;
    bool close(System*) override;
    Object* findObject(System*, const char*) override;
    NameTreeNode* findEntry(System*, Object*) override;
    NameTreeNode* findChild(const char*) override;
    NameTreeNode* findParent(Object*) override;
    bool objectPath(Object*, char*, int) override;
    bool nodePath(NameTreeNode*, char*, int) override;
    bool stream(System*, Stream*) override;
protected:
    NameTreeNode* m_children;
    char* m_name;
};
class NameTreeRoot : public NameTreeContainer
{
public:
    NameTreeRoot() : NameTreeContainer() {}
    bool close(System*) override;
    bool objectPath(Object*, char*, int) override;
    bool nodePath(NameTreeNode*, char*, int) override;
};
class NameTreeObjectEntry : public NameTreeNode
{
public:
    explicit NameTreeObjectEntry(Object* object) : m_object(object) {}
    const char* getName() override;
    bool setName(const char*) override;
    Object* getObject() override;
    void setPayload(void*) override;
    bool save(System*) override;
    bool contains(Object*) override;
    bool eraseObject(System*, Object*) override;
    bool eraseNode(System*, NameTreeNode*) override;
    bool preClose(System*) override;
    bool close(System*) override;
    Object* findObject(System*, const char*) override;
    NameTreeNode* findEntry(System*, Object*) override;
    bool objectPath(Object*, char*, int) override;
    bool nodePath(NameTreeNode*, char*, int) override;
    bool stream(System*, Stream*) override;
private:
    Object* m_object; // +8, lifetime belongs to explicit close, not destructor
};
} }
#endif
#endif
