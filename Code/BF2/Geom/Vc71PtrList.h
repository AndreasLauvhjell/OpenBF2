// [CHOSEN, not recovered] pointer list with the VC7.1 header/node layout used by road owners.
// Nodes use the engine heap; the sentinel's value and allocator words are not initialized.
// Copies allocate new nodes. Inserting before the sentinel appends; clear detaches the chain
// before freeing nodes and leaves the sentinel for reuse.

#ifdef BF2_IMAGE_RENDDX9

#ifndef BF2_GEOM_VC71PTRLIST_H
#define BF2_GEOM_VC71PTRLIST_H

#include "../../RendDX9Abi/BfMemory.h"

#include <stdexcept>

namespace dice {
namespace hfe {
namespace geom {

template <typename T>
struct Vc71PtrListNode
{
    Vc71PtrListNode* next;     // +0
    Vc71PtrListNode* prev;     // +4
    T*               value;    // +8
};

template <typename T>
struct Vc71PtrList
{
    typedef Vc71PtrListNode<T> Node;

    void*        alloc;    // +0  std::allocator, never written
    Node*        head;     // +4
    unsigned int size;     // +8

    Vc71PtrList()
    {
        head = buyHead();
        size = 0;
    }

    // list(const list&) - 0x101446C0 for std::list<RoadEditable*>: a fresh head, _Mysize = 0, then
    // the source's elements inserted at end() one by one (each with its own _Incsize test).
    Vc71PtrList(const Vc71PtrList& other)
    {
        head = buyHead();
        size = 0;
        for (Node* node = other.head->next; node != other.head; node = node->next)
            insert(head, node->value);
    }

    ~Vc71PtrList()
    {
        clear();
        ::renddx9abi::engineFree(head);
        head = 0;
    }

    void insert(Node* where, T* value)
    {
        Node* const node = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
        if (node)
        {
            node->next  = where;
            node->prev  = where->prev;
            node->value = value;
        }
        if (0x3FFFFFFFu - size < 1u)                                         // _Incsize's max_size test
            throw std::length_error("list<T> too long");
        size += 1u;
        where->prev = node;
        node->prev->next = node;
    }

    void erase(Node* node)
    {
        if (node != head)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            ::renddx9abi::engineFree(node);
            size -= 1u;
        }
    }

    void clear()
    {
        Node* node = head->next;
        head->next = head;
        head->prev = head;
        size = 0;
        while (node != head)
        {
            Node* const next = node->next;
            ::renddx9abi::engineFree(node);
            node = next;
        }
    }

private:
    Vc71PtrList& operator=(const Vc71PtrList&);                              // not used on any path

    static Node* buyHead()
    {
        Node* const node = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
        if (node)
            node->next = node;                                               // `mov [eax], eax`
        if (node)
            node->prev = node;                                               // `lea ecx,[eax+4]; mov [ecx], eax`
        return node;
    }
};

#if defined(_M_IX86)
static_assert(sizeof(Vc71PtrListNode<void>) == 12, "the VC7.1 list node is 12 bytes");
static_assert(sizeof(Vc71PtrList<void>) == 12, "the VC7.1 std::list header is 12 bytes");
#endif

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_VC71PTRLIST_H
#endif // BF2_IMAGE_RENDDX9
