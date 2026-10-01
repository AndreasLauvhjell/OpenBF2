// Value list with a 12-byte VC7.1-compatible header and engine-heap nodes.
// Nodes use the host value layout and remain private; sentinel value and allocator
// words are intentionally uninitialized.


#ifdef BF2_IMAGE_RENDDX9

#ifndef BF2_GEOM_VC71VALUELIST_H
#define BF2_GEOM_VC71VALUELIST_H

#include "../../RendDX9Abi/BfMemory.h"

#include <new>
#include <stdexcept>

namespace dice {
namespace hfe {
namespace geom {

template <typename T>
struct Vc71ValueListNode
{
    Vc71ValueListNode* next;     // +0
    Vc71ValueListNode* prev;     // +4
    T                  value;    // +8
};

template <typename T>
struct Vc71ValueList
{
    typedef Vc71ValueListNode<T> Node;

    void*        alloc;    // +0  std::allocator, never written
    Node*        head;     // +4
    unsigned int size;     // +8

    Vc71ValueList()
    {
        head = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
        if (head)
        {
            head->next = head;
            head->prev = head;
        }
        size = 0;
    }

    ~Vc71ValueList()
    {
        Node* node = head->next;
        head->next = head;
        head->prev = head;
        size = 0;
        while (node != head)
        {
            Node* const next = node->next;
            node->value.~T();
            ::renddx9abi::engineFree(node);
            node = next;
        }
        ::renddx9abi::engineFree(head);
        head = 0;
    }

    T& front() { return head->next->value; }

    void push_back(const T& value)
    {
        Node* const node = static_cast<Node*>(::renddx9abi::engineAlloc(sizeof(Node)));
        if (node)
        {
            node->next = head;
            node->prev = head->prev;
            ::new (static_cast<void*>(&node->value)) T(value);
        }
        if (0x0FFFFFFFu - size < 1u)                                         // _Incsize's max_size test
            throw std::length_error("list<T> too long");
        size += 1u;
        head->prev = node;
        node->prev->next = node;
    }

    void pop_front()
    {
        Node* const node = head->next;
        if (node != head)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->value.~T();
            ::renddx9abi::engineFree(node);
            size -= 1u;
        }
    }

private:
    Vc71ValueList(const Vc71ValueList&);
    Vc71ValueList& operator=(const Vc71ValueList&);
};

#if defined(_M_IX86)
static_assert(sizeof(Vc71ValueList<int>) == 12, "the VC7.1 std::list header is 12 bytes");
#endif

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_VC71VALUELIST_H
#endif // BF2_IMAGE_RENDDX9
