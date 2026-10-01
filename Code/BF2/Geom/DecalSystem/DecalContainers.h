// [CHOSEN, not recovered] containers with the VC7.1 layouts embedded by decal owners.
// Vectors grow by half; resize(0) keeps storage, clear() frees it, and copies allocate exact size.
// Maps preserve ordered traversal, duplicate rejection, stable nodes and head/sentinel links.
// [CHOSEN, not recovered] insertion/erasure currently use an unbalanced search tree;
// [CHOSEN] vector moves replace deep copies and therefore change allocation traffic.
// [RETIRE-WHEN: RW-74] Replace these operations after the original container behavior is
// implemented and validated for layout, ownership, traversal and allocation semantics.

#ifndef BF2_GEOM_DECALSYSTEM_DECALCONTAINERS_H
#define BF2_GEOM_DECALSYSTEM_DECALCONTAINERS_H

#ifdef BF2_IMAGE_RENDDX9

#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/Vc71String.h"

#include <new>
#include <string.h>

namespace dice {
namespace hfe {
namespace geom {
namespace decalvc71 {


// Vector<T>

template <typename T>
class Vector
{
public:
    Vector() : m_first(0), m_last(0), m_end(0) {}           // the allocator word stays unwritten

    // VC7.1 vector(const vector&): _Buy(size()) and the uninitialised copy (0x101AB2A0).
    Vector(const Vector& other) : m_first(0), m_last(0), m_end(0)
    {
        const unsigned int n = other.size();
        if (n)
        {
            m_first = allocate(n);
            m_end   = m_first + n;
            m_last  = m_first;
            for (const T* p = other.m_first; p != other.m_last; ++p, ++m_last)
                ::new (static_cast<void*>(m_last)) T(*p);
        }
    }

    // [CHOSEN] - see the file header.
    Vector(Vector&& other) : m_first(other.m_first), m_last(other.m_last), m_end(other.m_end)
    {
        other.m_first = 0;
        other.m_last  = 0;
        other.m_end   = 0;
    }

    Vector& operator=(const Vector& other)
    {
        if (this != &other)
        {
            Vector copy(other);
            swap(copy);
        }
        return *this;
    }

    Vector& operator=(Vector&& other)
    {
        if (this != &other)
        {
            clear();
            swap(other);
        }
        return *this;
    }

    ~Vector() { clear(); }

    void swap(Vector& other)
    {
        T* const first = m_first; m_first = other.m_first; other.m_first = first;
        T* const last  = m_last;  m_last  = other.m_last;  other.m_last  = last;
        T* const end   = m_end;   m_end   = other.m_end;   other.m_end   = end;
    }

    unsigned int size() const
    {
        return m_first ? static_cast<unsigned int>(m_last - m_first) : 0u;
    }
    unsigned int capacity() const
    {
        return m_first ? static_cast<unsigned int>(m_end - m_first) : 0u;
    }

    T*       data()        { return m_first; }               // _Myfirst, unguarded
    const T* data()  const { return m_first; }
    T*       begin()       { return m_first; }
    T*       end()         { return m_last; }
    T&       operator[](unsigned int i)       { return m_first[i]; }
    const T& operator[](unsigned int i) const { return m_first[i]; }

    void push_back(const T& value)
    {
        if (m_first && size() < capacity())
        {
            ::new (static_cast<void*>(m_last)) T(value);
            ++m_last;
            return;
        }
        insertAtEnd(1u, value);
    }

    // VC7.1 resize(n) builds `T()` and calls resize(n, T()).
    void resize(unsigned int n) { resize(n, T()); }
    void resize(unsigned int n, const T& value)
    {
        const unsigned int current = size();
        if (current < n)
            insertAtEnd(n - current, value);
        else if (m_first && n < current)                     // erase(begin() + n, end())
        {
            T* const newLast = m_first + n;
            for (T* p = newLast; p != m_last; ++p)
                p->~T();
            m_last = newLast;
        }
    }

    void clear()                                              // VC7.1 clear() == _Tidy()
    {
        if (m_first)
        {
            for (T* p = m_first; p != m_last; ++p)
                p->~T();
            ::renddx9abi::engineFree(m_first);
        }
        m_first = 0;
        m_last  = 0;
        m_end   = 0;
    }

private:
    static T* allocate(unsigned int count)
    {
        return static_cast<T*>(::renddx9abi::engineAlloc(count * static_cast<unsigned int>(sizeof(T))));
    }

    // _Insert_n(end(), count, value) - the only insert position a decal path uses.
    void insertAtEnd(unsigned int count, const T& value)
    {
        if (count == 0)
            return;
        const T copy(value);                                  // `_Ty _Tmp = _Val`
        const unsigned int oldSize = size();
        if (capacity() < oldSize + count)
        {
            const unsigned int oldCap = capacity();
            unsigned int newCap = oldCap + oldCap / 2u;
            if (newCap < oldSize + count)
                newCap = oldSize + count;
            T* const block = allocate(newCap);
            T* dst = block;
            for (T* p = m_first; p != m_last; ++p, ++dst)
                ::new (static_cast<void*>(dst)) T(*p);
            for (unsigned int i = 0; i < count; ++i, ++dst)
                ::new (static_cast<void*>(dst)) T(copy);
            if (m_first)
            {
                for (T* p = m_first; p != m_last; ++p)
                    p->~T();
                ::renddx9abi::engineFree(m_first);
            }
            m_first = block;
            m_last  = dst;
            m_end   = block + newCap;
            return;
        }
        for (unsigned int i = 0; i < count; ++i, ++m_last)
            ::new (static_cast<void*>(m_last)) T(copy);
    }

    void* m_alloc;                                            // +0x00  allocator<T>, padded
    T*    m_first;                                            // +0x04  _Myfirst
    T*    m_last;                                             // +0x08  _Mylast
    T*    m_end;                                              // +0x0C  _Myend
};


// Map<K, V, Less>

template <typename K, typename V>
struct TreeNode
{
    TreeNode*     left;       // +0x00  _Left
    TreeNode*     parent;     // +0x04  _Parent
    TreeNode*     right;      // +0x08  _Right
    K             key;        // +0x0C  _Myval.first
    V             value;      //        _Myval.second
    unsigned char color;      //        _Color, 1 == black
    unsigned char isNil;      //        _Isnil
};

// std::less<T*> - VC7.1 compares the raw pointers, i.e. unsigned (0x10126190 `cmp; setb`,
// 0x100A5850 `cmp; jnb`).
template <typename K>
struct UnsignedLess
{
    static bool less(const K& a, const K& b) { return a < b; }
};

// The template map's comparator: 0x10036390 (find, comdat-shared with GeometryTemplateManager's
// registry) and 0x10126CE0 (operator[]) both call MSVCR71 `_stricmp(a.c_str(), b.c_str())`.
struct NoCaseLess
{
    static bool less(const vc71::string& a, const vc71::string& b)
    {
        return ::_stricmp(a.c_str(), b.c_str()) < 0;
    }
};

// For the two maps the image only ever constructs, clears and destroys (DecalManager +0x78 and
// +0x84 - see DecalManager.h). Their comparators are never instantiated in the shipped image, so
// they are [UNRESOLVED]; these stand in so the shape can be declared, and nothing calls them.
struct UnreadStringLess
{
    static bool less(const vc71::string& a, const vc71::string& b)
    {
        return ::strcmp(a.c_str(), b.c_str()) < 0;
    }
};

template <typename K, typename V, typename Less>
class Map
{
public:
    typedef TreeNode<K, V> Node;

    // The owner constructor's inline sequence (0x10125215..0x10125248 for +0x28, the same six
    // stores for every tree): _Buynode() for the head, then _Isnil = 1, self-links, _Mysize = 0.
    Map() : m_head(buyHead()), m_size(0)
    {
        m_head->isNil  = 1;
        m_head->parent = m_head;
        m_head->left   = m_head;
        m_head->right  = m_head;
    }

    // The owner destructor's inline sequence (0x101250B8..0x101250E1): erase(begin(), end()), free
    // the head, null _Myhead and _Mysize.
    ~Map()
    {
        clear();
        ::renddx9abi::engineFree(m_head);
        m_head = 0;
        m_size = 0;
    }

    Node*        begin() const { return m_head->left; }
    Node*        end()   const { return m_head; }
    unsigned int size()  const { return m_size; }

    // VC7.1 _Inc (inlined at 0x10124919..0x10124957 and 0x100A54F0).
    static Node* next(Node* node)
    {
        if (node->isNil)
            return node;
        if (!node->right->isNil)
        {
            node = node->right;
            while (!node->left->isNil)
                node = node->left;
            return node;
        }
        Node* p = node->parent;
        while (!p->isNil && node == p->right)
        {
            node = p;
            p = p->parent;
        }
        return p;
    }

    // _Lbound (sub_101C0A60's descent).
    Node* lowerBound(const K& key) const
    {
        Node* result = m_head;
        Node* cur = m_head->parent;
        while (!cur->isNil)
        {
            if (Less::less(cur->key, key))
                cur = cur->right;
            else
            {
                result = cur;
                cur = cur->left;
            }
        }
        return result;
    }

    // find (0x100A5850 / 0x10036390): lower bound, then end() unless the key really matches.
    Node* find(const K& key) const
    {
        Node* const n = lowerBound(key);
        if (n == m_head || Less::less(key, n->key))
            return m_head;
        return n;
    }

    // insert(value_type) - insert_unique (0x10126190), VC7.1's _Tree::insert step for step: descend
    // remembering the last node and the last direction; a left landing on begin() inserts at once;
    // otherwise the candidate is that node (right landing) or its in-order predecessor (left
    // landing), and the key is new only when the candidate is strictly less. Answers the node and
    // whether it was new.
    Node* insert(const K& key, const V& value, bool* inserted)
    {
        Node* where = m_head;
        Node* cur = m_head->parent;
        bool  addLeft = true;                                  // left of the head when empty
        while (!cur->isNil)
        {
            where = cur;
            addLeft = Less::less(key, cur->key);
            cur = addLeft ? cur->left : cur->right;
        }

        Node* candidate = where;
        if (addLeft)
        {
            if (where == m_head->left)                         // _Where == begin()
                return link(where, true, key, value, inserted);
            candidate = previous(where);                       // Where
        }
        if (Less::less(candidate->key, key))
            return link(where, addLeft, key, value, inserted);
        if (inserted)
            *inserted = false;
        return candidate;
    }

    // operator[] (0x10126CE0 / 0x10126D70): lower bound; insert (key, V()) when absent; answer the
    // mapped value.
    V& operator[](const K& key)
    {
        Node* n = lowerBound(key);
        if (n == m_head || Less::less(key, n->key))
            n = insert(key, V(), 0);
        return n->value;
    }

    // erase(iterator) (0x100F63D0 / 0x101265D0). Transplants the successor on a two-child erase so
    // every surviving node keeps its address, as the shipped erase does.
    void erase(Node* z)
    {
        if (!z || z == m_head)
            return;

        Node* replacement;
        if (z->left == m_head)
            replacement = z->right;
        else if (z->right == m_head)
            replacement = z->left;
        else
        {
            Node* successor = z->right;
            while (successor->left != m_head)
                successor = successor->left;
            if (successor->parent != z)
            {
                successor->parent->left = successor->right;
                if (successor->right != m_head)
                    successor->right->parent = successor->parent;
                successor->right = z->right;
                successor->right->parent = successor;
            }
            successor->left = z->left;
            successor->left->parent = successor;
            replacement = successor;
        }

        if (z->parent == m_head)
            m_head->parent = replacement;
        else if (z == z->parent->left)
            z->parent->left = replacement;
        else
            z->parent->right = replacement;
        if (replacement != m_head)
            replacement->parent = z->parent;

        relinkExtremes();
        destroyNode(z);
        --m_size;
    }

    // clear(): _Erase(root) + relink the head (the inlined loops at 0x10124A80..0x10124B64 and
    // sub_10126250 / sub_10126BD0). Walked iteratively - the chosen tree can be degenerate.
    void clear()
    {
        Node* n = m_head->parent;
        while (n != m_head)
        {
            if (n->left != m_head)  { n = n->left;  continue; }
            if (n->right != m_head) { n = n->right; continue; }
            Node* const parent = n->parent;
            if (parent != m_head)
            {
                if (parent->left == n) parent->left = m_head;
                else                   parent->right = m_head;
            }
            destroyNode(n);
            n = parent;
        }
        m_head->parent = m_head;
        m_size = 0;
        m_head->left  = m_head;
        m_head->right = m_head;
    }

private:
    Map(const Map&);                                          // no recovered path copies a tree
    Map& operator=(const Map&);

    // _Buynode() for the head (sub_10125880 / sub_101256B0 / sub_101257E0): the three links null,
    // _Color black, _Isnil 0 - the key and value are NOT constructed.
    static Node* buyHead()
    {
        Node* const n = static_cast<Node*>(::renddx9abi::engineAlloc(static_cast<unsigned int>(sizeof(Node))));
        n->left   = 0;                                        // unguarded, as shipped: a failed
        n->parent = 0;                                        //   allocation throws in VC7.1 and
        n->right  = 0;                                        //   faults here
        n->color  = 1;
        n->isNil  = 0;
        return n;
    }

    Node* link(Node* parent, bool asLeft, const K& key, const V& value, bool* inserted)
    {
        Node* const n = static_cast<Node*>(::renddx9abi::engineAlloc(static_cast<unsigned int>(sizeof(Node))));
        ::new (static_cast<void*>(&n->key)) K(key);
        ::new (static_cast<void*>(&n->value)) V(value);
        n->left   = m_head;
        n->right  = m_head;
        n->parent = parent;
        n->color  = 0;                                        // red, as a fresh node is
        n->isNil  = 0;
        if (parent == m_head)
        {
            m_head->parent = n;
            m_head->left   = n;
            m_head->right  = n;
        }
        else if (asLeft)
        {
            parent->left = n;
            if (parent == m_head->left)
                m_head->left = n;
        }
        else
        {
            parent->right = n;
            if (parent == m_head->right)
                m_head->right = n;
        }
        ++m_size;
        if (inserted)
            *inserted = true;
        return n;
    }

    Node* previous(Node* node) const
    {
        if (!node->left->isNil)
        {
            node = node->left;
            while (!node->right->isNil)
                node = node->right;
            return node;
        }
        Node* p = node->parent;
        while (!p->isNil && node == p->left)
        {
            node = p;
            p = p->parent;
        }
        return p;
    }

    void relinkExtremes()
    {
        Node* const root = m_head->parent;
        if (root == m_head)
        {
            m_head->left  = m_head;
            m_head->right = m_head;
            return;
        }
        Node* l = root;
        while (l->left != m_head)
            l = l->left;
        Node* r = root;
        while (r->right != m_head)
            r = r->right;
        m_head->left  = l;
        m_head->right = r;
    }

    static void destroyNode(Node* n)
    {
        n->value.~V();
        n->key.~K();
        ::renddx9abi::engineFree(n);
    }

    void*        m_comp;                                      // +0x00  the empty comparator, padded
    Node*        m_head;                                      // +0x04  _Myhead
    unsigned int m_size;                                      // +0x08  _Mysize
};

#if defined(_M_IX86)
static_assert(sizeof(Vector<int>) == 16, "the VC7.1 std::vector is 16 bytes");
static_assert(sizeof(Map<int, int, UnsignedLess<int> >) == 12, "the VC7.1 std::map is 12 bytes");
static_assert(sizeof(TreeNode<void*, void*>) == 0x18, "sub_10125880 allocates 0x18");
static_assert(sizeof(TreeNode<unsigned short, void*>) == 0x18, "the +0x6C map's node is 0x18");
static_assert(sizeof(TreeNode<vc71::string, void*>) == 0x30, "sub_101256B0 allocates 0x30");
#endif

} // namespace decalvc71
} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9

#endif // BF2_GEOM_DECALSYSTEM_DECALCONTAINERS_H
