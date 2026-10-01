// VC7.1 tree layout adapter, [CHOSEN, not recovered] source.
// The 12-byte header contains comparator padding, sentinel and count. Node variants have distinct
// payload and flag offsets; use the matching variant for each mapped type.
// Even an empty tree requires a self-linked sentinel. Its payload is never constructed or read.
// These layouts support recovered operations and are not general-purpose STL replacements.

#ifndef RENDDX9ABI_VC71MAP_H
#define RENDDX9ABI_VC71MAP_H

#include "BfMemory.h"
#include "Vc71String.h"

#include <string.h>
#include <new>

namespace vc71 {

// The node, exactly as _Buynode shapes it.
struct map_node
{
    map_node*     left;     // +0
    map_node*     parent;   // +4
    map_node*     right;    // +8
    char          value[32];// +12  28-byte key + 4-byte value; see the note on _Buynode
    unsigned char color;    // +44  1 == black
    unsigned char isNil;    // +45
    unsigned char pad[2];   // +46  to the 4-byte alignment the 0x30 allocation implies
};

static_assert(sizeof(map_node) == 0x30, "_Buynode (0x100B1AA3) allocates 0x30");

// The value half of _Myval, at node+40. Every consumer read so far treats it as a pointer.
inline void* mapNodeValue(const map_node* node)
{
    return *reinterpret_cast<void* const*>(node->value + 28);
}

struct map_layout
{
    void*     comp;     // +0   empty comparator, padded - the original never writes it
    map_node* head;     // +4   _Myhead
    unsigned  size;     // +8   _Mysize

    // Allocates a self-linked nil header using nodeSize.
    // Color and nil flags live at nodeSize-4 and nodeSize-3 across the supported node layouts.
    void construct(unsigned nodeSize = 0x30)
    {
        comp = 0;

        unsigned char* node = static_cast<unsigned char*>(::renddx9abi::engineAlloc(nodeSize));
        if (node)
        {
            for (unsigned i = 0; i < nodeSize; ++i)
                node[i] = 0;

            void** links = reinterpret_cast<void**>(node);
            links[0] = node;                 // _Left   -> self
            links[1] = node;                 // _Parent -> self
            links[2] = node;                 // _Right  -> self

            node[nodeSize - 4] = 1;          // _Color = black
            node[nodeSize - 3] = 1;          // _Isnil = 1  (the header is the nil node)
        }

        head = reinterpret_cast<map_node*>(node);
        size = 0;
    }


    // [CHOSEN, not recovered] unbalanced multimap insertion retaining equal keys in insertion order.
    // Float key/value occupy +0x0C/+0x10; trailing flags depend on nodeSize.
    // [RETIRE-WHEN: RW-74] Replace with the observed red-black balancing algorithm.
    map_node* insertMultiFloat(float key, void* value, unsigned nodeSize)
    {
        if (!head)
            return 0;

        unsigned char* const raw = static_cast<unsigned char*>(::renddx9abi::engineAlloc(nodeSize));
        if (!raw)
            return 0;
        for (unsigned i = 0; i < nodeSize; ++i)
            raw[i] = 0;

        map_node* const n = reinterpret_cast<map_node*>(raw);
        n->left  = head;
        n->right = head;
        *reinterpret_cast<float*>(raw + 0x0C) = key;
        *reinterpret_cast<void**>(raw + 0x10) = value;
        raw[nodeSize - 4] = 0;                 // _Color = red, as a fresh node is
        raw[nodeSize - 3] = 0;                 // _Isnil = 0

        map_node* const root = head->parent;
        if (root == head)                      // empty: the new node becomes the root
        {
            n->parent    = head;
            head->parent = n;
            head->left   = n;
            head->right  = n;
            ++size;
            return n;
        }

        // The shipped descent, verbatim: right on `key >= node->key`, left otherwise.
        map_node* parent   = head;
        bool      wentLeft = true;
        map_node* cur      = root;
        while (!cur->isNil)
        {
            parent = cur;
            if (key >= *reinterpret_cast<const float*>(
                           reinterpret_cast<const unsigned char*>(cur) + 0x0C))
            {
                cur      = cur->right;
                wentLeft = false;
            }
            else
            {
                cur      = cur->left;
                wentLeft = true;
            }
        }

        n->parent = parent;
        if (wentLeft)
        {
            parent->left = n;
            if (parent == head->left)
                head->left = n;                // leftmost
        }
        else
        {
            parent->right = n;
            if (parent == head->right)
                head->right = n;               // rightmost
        }
        ++size;
        return n;
    }

    // The emptiness test both recovered walks use: _Myhead->_Left == _Myhead.
    bool empty() const { return !head || head->left == head; }

    // The relink half of slot +0x58 (sub_1015DC00). The recursive node destroy that precedes it
    // (sub_100C78BC) is not reproduced - see the caller, which owns that frontier.
    void relinkEmpty()
    {
        if (!head)
            return;
        head->parent = head;
        head->left   = head;
        head->right  = head;
        size = 0;
    }

    // In-order iteration, transcribed from the walk in sub_1015D8D0. begin() is _Myhead->_Left and
    // the end sentinel is _Myhead itself.
    map_node* begin() const { return head ? head->left : 0; }
    map_node* end()   const { return head; }

    static map_node* next(map_node* n)
    {
        if (!n->isNil)
        {
            map_node* r = n->right;
            if (r->isNil)
            {
                // Climb while we are the right child.
                map_node* p = n->parent;
                while (!p->isNil && n == p->right)
                {
                    n = p;
                    p = p->parent;
                }
                return p;
            }
            // Otherwise the leftmost of the right subtree.
            n = r;
            for (map_node* l = r->left; !l->isNil; l = l->left)
                n = l;
            return n;
        }
        return n;
    }
};

static_assert(sizeof(map_layout) == 12, "the host's std::map is 12 bytes on VC7.1");

// Set node for a four-byte integer/pointer key. Its x86 size is 20 bytes, with color/nil at +16/+17;
// do not reuse the larger string-map node layout.

struct set_node
{
    set_node*     left;     // +0
    set_node*     parent;   // +4
    set_node*     right;    // +8
    unsigned int  key;      // +12
    unsigned char color;    // +16
    unsigned char isNil;    // +17
    unsigned char pad[2];   // +18
};

static_assert(sizeof(set_node) == 0x14, "_Buynode (0x100047E0) allocates 0x14");

struct set_layout
{
    void*     comp;     // +0   empty comparator, padded
    set_node* head;     // +4   _Myhead
    unsigned  size;     // +8   _Mysize

    // The header constructor, transcribed from QueryManager_ctor's two inlined copies.
    void construct()
    {
        comp = 0;
        set_node* node = static_cast<set_node*>(::renddx9abi::engineAlloc(sizeof(set_node)));
        if (node)
        {
            node->left = 0; node->parent = 0; node->right = 0;
            node->key = 0;
            node->color = 1;
            node->isNil = 0;
            node->pad[0] = 0; node->pad[1] = 0;

            node->isNil  = 1;
            node->parent = node;
            node->left   = node;
            node->right  = node;
        }
        head = node;
        size = 0;
    }

    bool      empty() const { return !head || head->left == head; }
    set_node* begin() const { return head ? head->left : 0; }
    set_node* end()   const { return head; }

    // find(), transcribed from sub_100048C0: descend, then the classic lower_bound check that
    // turns a not-less-than result into end().
    set_node* find(unsigned int key) const
    {
        if (!head)
            return 0;
        set_node* parent = head;
        set_node* node   = head->parent;      // the root
        while (!node->isNil)
        {
            if (node->key >= key) { parent = node; node = node->left; }
            else                  { node = node->right; }
        }
        if (parent == head || key < parent->key)
            return head;                      // end()
        return parent;
    }

    // In-order successor, same shape as map_layout::next but testing +17.
    static set_node* next(set_node* n)
    {
        if (n->isNil)
            return n;
        set_node* r = n->right;
        if (r->isNil)
        {
            set_node* p = n->parent;
            while (!p->isNil && n == p->right) { n = p; p = p->parent; }
            return p;
        }
        n = r;
        for (set_node* l = r->left; !l->isNil; l = l->left)
            n = l;
        return n;
    }

    // [CHOSEN, not recovered] unbalanced set insertion with duplicate rejection.
    // Maintains root/extreme links and nil flags. [RETIRE-WHEN: RW-74] Replace with observed red-black balancing.
    bool insert(unsigned int key)
    {
        if (!head)
            return false;

        set_node* root = head->parent;
        if (root == head)                      // empty: the new node becomes the root
        {
            set_node* n = newNode(key);
            if (!n) return false;
            n->parent = head;
            head->parent = n;                  // root
            head->left   = n;                  // leftmost
            head->right  = n;                  // rightmost
            ++size;
            return true;
        }

        set_node* cur = root;
        for (;;)
        {
            if (key < cur->key)
            {
                if (cur->left->isNil)
                {
                    set_node* n = newNode(key);
                    if (!n) return false;
                    n->left = head; n->right = head;
                    n->parent = cur;
                    cur->left = n;
                    if (cur == head->left)
                        head->left = n;        // a new leftmost
                    ++size;
                    return true;
                }
                cur = cur->left;
            }
            else if (cur->key < key)
            {
                if (cur->right->isNil)
                {
                    set_node* n = newNode(key);
                    if (!n) return false;
                    n->left = head; n->right = head;
                    n->parent = cur;
                    cur->right = n;
                    if (cur == head->right)
                        head->right = n;       // a new rightmost
                    ++size;
                    return true;
                }
                cur = cur->right;
            }
            else
            {
                return false;                  // already present - std::set rejects duplicates
            }
        }
    }

    // [CHOSEN, not recovered] unbalanced set erase preserves surviving node addresses through transplantation.
    // clear frees nodes iteratively but retains the sentinel; destroy also frees the sentinel. Keys are non-owning.
    // [RETIRE-WHEN: RW-74] Restore the original red-black balancing while preserving iterator guarantees.
    void erase(set_node* z)
    {
        if (!head || !z || z == head)
            return;

        set_node* replacement;
        if (z->left == head)
        {
            replacement = z->right;
        }
        else if (z->right == head)
        {
            replacement = z->left;
        }
        else
        {
            // The successor has no left child. Detach it first when it is deeper than z's direct
            // right child, then transplant the node itself into z's old position.
            set_node* successor = z->right;
            while (successor->left != head)
                successor = successor->left;

            if (successor->parent != z)
            {
                successor->parent->left = successor->right;
                if (successor->right != head)
                    successor->right->parent = successor->parent;
                successor->right = z->right;
                successor->right->parent = successor;
            }

            successor->left = z->left;
            successor->left->parent = successor;
            replacement = successor;
        }

        if (z->parent == head)
            head->parent = replacement;
        else if (z == z->parent->left)
            z->parent->left = replacement;
        else
            z->parent->right = replacement;
        if (replacement != head)
            replacement->parent = z->parent;

        set_node* root = head->parent;
        if (root == head)
        {
            head->left  = head;
            head->right = head;
        }
        else
        {
            set_node* leftmost = root;
            while (leftmost->left != head)
                leftmost = leftmost->left;
            set_node* rightmost = root;
            while (rightmost->right != head)
                rightmost = rightmost->right;
            head->left  = leftmost;
            head->right = rightmost;
        }

        ::renddx9abi::engineFree(z);
        --size;
    }

    void clear()
    {
        if (!head)
        {
            size = 0;
            return;
        }

        // Repeatedly descend to a leaf, detach/free it, then continue at its parent. This is an
        // allocation-free post-order destruction even for the degenerate trees insert() can make.
        set_node* node = head->parent;
        while (node != head)
        {
            if (node->left != head)
            {
                node = node->left;
                continue;
            }
            if (node->right != head)
            {
                node = node->right;
                continue;
            }

            set_node* parent = node->parent;
            if (parent != head)
            {
                if (parent->left == node)
                    parent->left = head;
                else
                    parent->right = head;
            }
            ::renddx9abi::engineFree(node);
            node = parent;
        }

        head->parent = head;
        head->left   = head;
        head->right  = head;
        size = 0;
    }

    void destroy()
    {
        clear();
        if (head)
            ::renddx9abi::engineFree(head);
        comp = 0;
        head = 0;
        size = 0;
    }

private:
    set_node* newNode(unsigned int key)
    {
        set_node* n = static_cast<set_node*>(::renddx9abi::engineAlloc(sizeof(set_node)));
        if (!n)
            return 0;
        n->left = head; n->parent = head; n->right = head;
        n->key = key;
        n->color = 1;
        n->isNil = 0;
        n->pad[0] = 0; n->pad[1] = 0;
        return n;
    }
};

static_assert(sizeof(set_layout) == 12, "the host's std::set is 12 bytes on VC7.1");


// Find/insert operations over the host's map layout.
// [CHOSEN, not recovered] Unbalanced trees preserve lookup and in-order traversal contracts.
// [RETIRE-WHEN: RW-74] Replace the chosen balancing policy with the agreed ABI-container solution.
// Maintain sentinel parent=root, left=leftmost, right=rightmost, isnil=1; absent children point to it.
// String-key nodes store the 28-byte key at +12 and the value at +40.

inline const char* mapNodeKey(const map_node* node)
{
    return reinterpret_cast<const vc71::string*>(node->value)->c_str();
}

inline void*& mapNodeValueRef(map_node* node)
{
    return *reinterpret_cast<void**>(node->value + 28);
}

// Returns the node whose key equals `key`, or null. Never allocates.
inline map_node* mapFind(const map_layout& m, const char* key)
{
    map_node* head = m.head;
    if (!head)
        return 0;

    map_node* cur = head->parent;          // the root
    while (cur && cur != head)
    {
        const int cmp = ::strcmp(key, mapNodeKey(cur));
        if (cmp == 0)
            return cur;
        cur = cmp < 0 ? cur->left : cur->right;
    }
    return 0;
}

// Case-insensitive lookup for the geometry-template registry. Keep it separate from
// the ordinary case-sensitive map helpers used by other registries.
inline map_node* mapFindInsensitive(const map_layout& m, const vc71::string& key)
{
    map_node* head = m.head;
    if (!head)
        return 0;

    map_node* cur = head->parent;
    while (cur && cur != head)
    {
        const int cmp = ::_stricmp(key.c_str(), mapNodeKey(cur));
        if (cmp == 0)
            return cur;
        cur = cmp < 0 ? cur->left : cur->right;
    }
    return 0;
}

// Returns the existing node for `key`, or inserts a fresh one. The new node's value bytes are
// zeroed; the caller fills them. Returns null only if the allocation fails.
inline map_node* mapInsert(map_layout& m, unsigned nodeSize, const char* key)
{
    map_node* head = m.head;
    if (!head)
        return 0;

    map_node* parent = 0;
    int       cmp    = 0;

    if (head->parent != head)              // non-empty: descend to the insertion point
    {
        map_node* cur = head->parent;
        for (;;)
        {
            cmp = ::strcmp(key, mapNodeKey(cur));
            if (cmp == 0)
                return cur;                // already present

            map_node* child = cmp < 0 ? cur->left : cur->right;
            if (!child || child == head)
            {
                parent = cur;
                break;
            }
            cur = child;
        }
    }

    unsigned char* raw = static_cast<unsigned char*>(::renddx9abi::engineAlloc(nodeSize));
    if (!raw)
        return 0;
    for (unsigned i = 0; i < nodeSize; ++i)
        raw[i] = 0;

    map_node* n = reinterpret_cast<map_node*>(raw);
    new (n->value) vc71::string(key);      // the key, in place at node+12
    n->left   = head;                      // absent children point at the nil sentinel
    n->right  = head;
    raw[nodeSize - 4] = 0;                 // _Color: red
    raw[nodeSize - 3] = 0;                 // _Isnil: 0

    if (!parent)                           // the tree was empty - this becomes the root
    {
        n->parent    = head;
        head->parent = n;
        head->left   = n;
        head->right  = n;
    }
    else
    {
        n->parent = parent;
        if (cmp < 0)
        {
            parent->left = n;
            if (head->left == parent)      // a new leftmost
                head->left = n;
        }
        else
        {
            parent->right = n;
            if (head->right == parent)     // a new rightmost
                head->right = n;
        }
    }

    ++m.size;
    return n;
}

// StaticMesh lightmap-cache node: 0x28 bytes with a four-byte key and 20-byte value.
// Do not use the string-map node. Balancing is [CHOSEN, not recovered].
// [RETIRE-WHEN: RW-74] Replace with the observed red-black balancing algorithm.
struct map_node_u32
{
    map_node_u32* left;        // +0x00
    map_node_u32* parent;      // +0x04
    map_node_u32* right;       // +0x08
    unsigned int  key;         // +0x0C
    unsigned char value[20];   // +0x10
    unsigned char color;       // +0x24  1 == black
    unsigned char isNil;       // +0x25
    unsigned char pad[2];      // +0x26
};

static_assert(sizeof(map_node_u32) == 0x28, "sub_100FFB50 allocates 0x28");

// The lookup half of sub_10102500, and the whole of the `find`-shaped sub_101000E0: returns the
// node for `key`, or null. The shipped walk descends on `>=` against an unsigned key; the
// comparison is unsigned here for the same reason.
inline map_node_u32* mapFindU32(const map_layout& m, unsigned int key)
{
    map_node_u32* const head = reinterpret_cast<map_node_u32*>(m.head);
    if (!head)
        return 0;

    map_node_u32* cur = head->parent;
    while (cur && cur != head && !cur->isNil)
    {
        if (cur->key == key)
            return cur;
        cur = key < cur->key ? cur->left : cur->right;
    }
    return 0;
}

// sub_10102500 - find or insert. The new node's value bytes are zeroed; the caller fills them,
// which is exactly what the shipped body does with its `memset(&v5[1], 0, 20)`.
inline map_node_u32* mapInsertU32(map_layout& m, unsigned int key)
{
    map_node_u32* const head = reinterpret_cast<map_node_u32*>(m.head);
    if (!head)
        return 0;

    map_node_u32* parent = 0;
    bool          goLeft = false;

    if (head->parent != head)                  // non-empty: descend to the insertion point
    {
        map_node_u32* cur = head->parent;
        for (;;)
        {
            if (cur->key == key)
                return cur;                    // already present

            goLeft = key < cur->key;
            map_node_u32* const child = goLeft ? cur->left : cur->right;
            if (!child || child == head || child->isNil)
            {
                parent = cur;
                break;
            }
            cur = child;
        }
    }

    unsigned char* const raw =
        static_cast<unsigned char*>(::renddx9abi::engineAlloc(sizeof(map_node_u32)));
    if (!raw)
        return 0;
    for (unsigned i = 0; i < sizeof(map_node_u32); ++i)
        raw[i] = 0;

    map_node_u32* const n = reinterpret_cast<map_node_u32*>(raw);
    n->key   = key;
    n->left  = head;                           // absent children point at the nil sentinel
    n->right = head;
    n->color = 0;                              // red
    n->isNil = 0;

    if (!parent)                               // the tree was empty - this becomes the root
    {
        n->parent    = head;
        head->parent = n;
        head->left   = n;
        head->right  = n;
    }
    else
    {
        n->parent = parent;
        if (goLeft)
        {
            parent->left = n;
            if (head->left == parent)
                head->left = n;
        }
        else
        {
            parent->right = n;
            if (head->right == parent)
                head->right = n;
        }
    }

    ++m.size;
    return n;
}

// Node destruction for the int-keyed map, read from sub_100FFBF0: recurse on the right child,
// iterate down the left, freeing as it goes. The shipped shape exactly - it is the VC7.1 _Erase.

// it frees nodes only. Whatever the values own is the caller's problem, and StaticMesh's
// destructor does release each entry's texture before calling this.
inline void mapEraseTreeU32(map_node_u32* node)
{
    while (node && !node->isNil)
    {
        mapEraseTreeU32(node->right);
        map_node_u32* const next = node->left;
        ::renddx9abi::engineFree(node);
        node = next;
    }
}

// sub_101007A0's fast path - erase(begin(), end()), i.e. clear(). The sentinel survives and is
// re-self-linked, which is what keeps the map usable afterwards.
inline void mapClearU32(map_layout& m)
{
    map_node_u32* const head = reinterpret_cast<map_node_u32*>(m.head);
    if (!head)
        return;

    mapEraseTreeU32(head->parent);

    head->parent = head;
    head->left   = head;
    head->right  = head;
    m.size       = 0;
}

// Removes a set value stored in the node key word. [CHOSEN, not recovered] Rebalancing
// remains governed by RW-74; descent and unlink retain the original behavior.
inline void setEraseValue(set_layout& s, void* value)
{
    set_node* const head = s.head;
    if (!head)
        return;

    const unsigned int key = reinterpret_cast<unsigned int>(value);

    set_node* node = head->parent;
    while (node && !node->isNil && node->key != key)
        node = key < node->key ? node->left : node->right;

    if (!node || node->isNil)
        return;

    // Unlink: splice in whichever child exists. A two-child node hands its slot to its
    // in-order successor, which is the ordinary transplant mapEraseWithValueDestroy performs.
    set_node* replacement = 0;
    if (!node->left || node->left->isNil)
        replacement = (node->right && !node->right->isNil) ? node->right : 0;
    else if (!node->right || node->right->isNil)
        replacement = node->left;
    else
    {
        set_node* succ = node->right;
        while (succ->left && !succ->left->isNil)
            succ = succ->left;
        node->key = succ->key;          // move the value up, then drop the successor
        node = succ;
        replacement = (node->right && !node->right->isNil) ? node->right : 0;
    }

    set_node* const parent = node->parent;
    if (parent && !parent->isNil)
    {
        if (parent->left == node)
            parent->left = replacement ? replacement : head;
        else if (parent->right == node)
            parent->right = replacement ? replacement : head;
    }
    if (head->parent == node)
        head->parent = replacement ? replacement : head;
    if (replacement)
        replacement->parent = parent;

    // The sentinel caches leftmost/rightmost; re-derive rather than track them through a splice.
    if (head->parent == head)
    {
        head->left  = head;
        head->right = head;
    }
    else
    {
        set_node* n = head->parent;
        while (n->left && !n->left->isNil) n = n->left;
        head->left = n;
        n = head->parent;
        while (n->right && !n->right->isNil) n = n->right;
        head->right = n;
    }

    ::renddx9abi::engineFree(node);
    if (s.size)
        --s.size;
}

// The case-insensitive twin of mapInsert, for GeometryTemplateManager's template registry.  Tree
// balancing remains under RW-74 exactly as it does for mapInsert; key equivalence and ordering are
// the instruction-proven `_stricmp` semantics.
inline map_node* mapInsertInsensitive(map_layout& m, unsigned nodeSize,
                                      const vc71::string& key)
{
    map_node* head = m.head;
    if (!head)
        return 0;

    map_node* parent = 0;
    int       cmp    = 0;

    if (head->parent != head)
    {
        map_node* cur = head->parent;
        for (;;)
        {
            cmp = ::_stricmp(key.c_str(), mapNodeKey(cur));
            if (cmp == 0)
                return cur;

            map_node* child = cmp < 0 ? cur->left : cur->right;
            if (!child || child == head)
            {
                parent = cur;
                break;
            }
            cur = child;
        }
    }

    unsigned char* raw = static_cast<unsigned char*>(::renddx9abi::engineAlloc(nodeSize));
    if (!raw)
        return 0;
    for (unsigned i = 0; i < nodeSize; ++i)
        raw[i] = 0;

    map_node* n = reinterpret_cast<map_node*>(raw);
    // Retail's operator[] copy-constructs the complete 28-byte string value.  The comparator still
    // sees C strings through `_stricmp`, but retaining the counted copy preserves bytes after an
    // embedded NUL in the first spelling inserted.
    new (n->value) vc71::string(key);
    n->left   = head;
    n->right  = head;
    raw[nodeSize - 4] = 0;
    raw[nodeSize - 3] = 0;

    if (!parent)
    {
        n->parent    = head;
        head->parent = n;
        head->left   = n;
        head->right  = n;
    }
    else
    {
        n->parent = parent;
        if (cmp < 0)
        {
            parent->left = n;
            if (head->left == parent)
                head->left = n;
        }
        else
        {
            parent->right = n;
            if (head->right == parent)
                head->right = n;
        }
    }

    ++m.size;
    return n;
}

// [CHOSEN, not recovered] erase preserves surviving node addresses by transplanting links, never values.
// Callers may retain the successor iterator. Recomputes extreme links after removal.
// [RETIRE-WHEN: RW-74] Restore the observed red-black rebalancing step.
typedef void (*map_value_destroy_fn)(map_node* node);

inline void mapEraseWithValueDestroy(map_layout& m, map_node* z,
                                     map_value_destroy_fn destroyValue)
{
    map_node* head = m.head;
    if (!head || !z || z == head)
        return;

    // transplant(u, v): replace subtree u by subtree v in u's parent.
    // (nested lambdas are off the table in C++14-without-captures style scaffolding; inline twice)
    map_node* y;                      // the node that structurally replaces z
    if (z->left == head || !z->left)
    {
        y = z->right;
    }
    else if (z->right == head || !z->right)
    {
        y = z->left;
    }
    else
    {
        // two children: the in-order successor (leftmost of the right subtree) takes z's place.
        map_node* s = z->right;
        while (s->left && s->left != head)
            s = s->left;
        if (s->parent != z)
        {
            // detach s from its parent, giving the parent s's right subtree
            s->parent->left = s->right;
            if (s->right && s->right != head)
                s->right->parent = s->parent;
            s->right = z->right;
            s->right->parent = s;
        }
        s->left = z->left;
        if (s->left && s->left != head)
            s->left->parent = s;
        y = s;
    }

    // hook y into z's parent slot
    if (z->parent == head)
        head->parent = y;
    else if (z == z->parent->left)
        z->parent->left = y;
    else
        z->parent->right = y;
    if (y && y != head)
        y->parent = z->parent;

    // recompute _Myhead's leftmost/rightmost from the (possibly new) root
    map_node* root = head->parent;
    if (!root || root == head)
    {
        head->parent = head;
        head->left   = head;
        head->right  = head;
    }
    else
    {
        map_node* l = root;
        while (l->left && l->left != head) l = l->left;
        map_node* r = root;
        while (r->right && r->right != head) r = r->right;
        head->left  = l;
        head->right = r;
    }

    // VC7.1 destroys the mapped value only after unlink/rebalance, then destroys the key and frees
    // the node.  That ordering is observable if a reference-counted value re-enters its owner from
    // release(), so callers with an owning mapped type need this hook rather than releasing before
    // mapErase.  Raw-pointer maps pass no hook and retain the previous behaviour.
    if (destroyValue)
        destroyValue(z);
    reinterpret_cast<vc71::string*>(z->value)->~string();   // the key, in place at node+12
    ::renddx9abi::engineFree(z);
    --m.size;
}

inline void mapErase(map_layout& m, map_node* z)
{
    mapEraseWithValueDestroy(m, z, 0);
}

// The in-order successor, by the same rules the recovered walks use. Returns null past the end.
inline map_node* mapNext(const map_layout& m, map_node* node)
{
    map_node* head = m.head;
    if (!node || node == head)
        return 0;

    if (node->right && node->right != head)
    {
        node = node->right;
        while (node->left && node->left != head)
            node = node->left;
        return node;
    }

    map_node* p = node->parent;
    while (p && p != head && node == p->right)
    {
        node = p;
        p = p->parent;
    }
    return (p == head) ? 0 : p;
}

// The leftmost node, i.e. the start of an in-order walk. Null when empty.
inline map_node* mapFirst(const map_layout& m)
{
    if (!m.head || m.head->left == m.head)
        return 0;
    return m.head->left;
}

} // namespace vc71

#endif // RENDDX9ABI_VC71MAP_H
