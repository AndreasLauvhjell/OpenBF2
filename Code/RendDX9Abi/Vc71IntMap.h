// VC7.1 map<int, four-byte value> adapter, [CHOSEN, not recovered].
// Nodes occupy 24 bytes: links at +0/+4/+8, signed key +12, value +16, color +20 and isnil +21.
// The 12-byte header owns a self-linked sentinel; absent children point to it.
// Chosen unbalanced-tree operations preserve ordering, duplicates and surviving node addresses;
// they do not reproduce retail red-black rotations. [RETIRE-WHEN: RW-74] Replace the chosen tree
// policy when the remaining ABI-container work supplies the required implementation.

#ifndef RENDDX9ABI_VC71INTMAP_H
#define RENDDX9ABI_VC71INTMAP_H

#include "BfMemory.h"

namespace vc71 {

template <typename V>
struct int_map_node
{
    int_map_node* left;      // +0x00
    int_map_node* parent;    // +0x04
    int_map_node* right;     // +0x08
    int           key;       // +0x0C
    V             value;     // +0x10
    unsigned char color;     // +0x14  1 == black
    unsigned char isNil;     // +0x15
    unsigned char pad[2];    // +0x16
};

template <typename V>
struct int_map_layout
{
    typedef int_map_node<V> node;

    void*    comp;   // +0  empty comparator, padded - the original never writes it
    node*    head;   // +4  _Myhead
    unsigned size;   // +8  _Mysize

    // _Buynode for the head (sub_10131690 / sub_1008A320), then the owner constructor's
    // self-linking: _Isnil = 1, _Parent = _Left = _Right = head, _Mysize = 0.
    // The `comp = 0` and head `key = 0` stores below are not shipped - the original leaves both
    //   words as allocated (see +0 above). [CHOSEN] zeroing: neither word is ever read as data, except
    //   that a key read through end() would see 0 here where retail sees heap garbage.
    void construct()
    {
        comp = 0;
        node* n = static_cast<node*>(::renddx9abi::engineAlloc(sizeof(node)));
        if (n)
        {
            n->left = 0; n->parent = 0; n->right = 0;
            n->key = 0;
            n->color = 1;
            n->isNil = 0;
            n->pad[0] = 0; n->pad[1] = 0;

            n->isNil  = 1;
            n->parent = n;
            n->left   = n;
            n->right  = n;
        }
        head = n;
        size = 0;
    }

    // The owner destructor's `erase(begin(), end())` + free the head + zero both words
    // (0x1012F680..0x1012F6AB for the slot maps, 0x1012F6ED..0x1012F714 for the item map).
    void destroy()
    {
        clear();
        if (head)
            ::renddx9abi::engineFree(head);
        head = 0;
        size = 0;
    }

    node* begin() const { return head ? head->left : 0; }
    node* end()   const { return head; }

    // The in-order successor, the same walk the recovered iterations inline (0x1012ED6A.. etc.).
    static node* next(node* n)
    {
        if (n->isNil)
            return n;
        node* r = n->right;
        if (r->isNil)
        {
            node* p = n->parent;
            while (!p->isNil && n == p->right) { n = p; p = p->parent; }
            return p;
        }
        n = r;
        for (node* l = r->left; !l->isNil; l = l->left)
            n = l;
        return n;
    }

    // _Lbound: the first node whose key is not less than `key` (sub_10132750's descent).
    node* lowerBound(int key) const
    {
        if (!head)
            return 0;
        node* result = head;
        node* cur = head->parent;
        while (!cur->isNil)
        {
            if (cur->key < key)
                cur = cur->right;
            else
            {
                result = cur;
                cur = cur->left;
            }
        }
        return result;
    }

    // find (sub_10049650): lower bound, then end() unless the key really matches.
    node* find(int key) const
    {
        node* n = lowerBound(key);
        if (!n || n == head || key < n->key)
            return head;
        return n;
    }

    // count(key) - the equal_range + _Distance pair (sub_101319E0 + sub_10168350). Unique keys, so
    // zero or one.
    unsigned count(int key) const
    {
        return find(key) != head ? 1u : 0u;
    }

    // insert(value_type) - insert_unique (sub_1008A8C0 / sub_10132420). Returns true and the new
    // node when inserted, false and the existing node when the key is already present.
    bool insert(int key, const V& value, node** where)
    {
        if (where)
            *where = head;
        if (!head)
            return false;

        node* root = head->parent;
        if (root == head)
        {
            node* n = newNode(key, value);
            if (!n) return false;
            n->parent = head;
            head->parent = n;
            head->left   = n;
            head->right  = n;
            ++size;
            if (where) *where = n;
            return true;
        }

        node* cur = root;
        for (;;)
        {
            if (key < cur->key)
            {
                if (cur->left->isNil)
                {
                    node* n = newNode(key, value);
                    if (!n) return false;
                    n->parent = cur;
                    cur->left = n;
                    if (cur == head->left)
                        head->left = n;
                    ++size;
                    if (where) *where = n;
                    return true;
                }
                cur = cur->left;
            }
            else if (cur->key < key)
            {
                if (cur->right->isNil)
                {
                    node* n = newNode(key, value);
                    if (!n) return false;
                    n->parent = cur;
                    cur->right = n;
                    if (cur == head->right)
                        head->right = n;
                    ++size;
                    if (where) *where = n;
                    return true;
                }
                cur = cur->right;
            }
            else
            {
                if (where) *where = cur;
                return false;
            }
        }
    }

    // operator[] (sub_10132750 / sub_1008AB60): lower_bound; insert (key, V()) when absent; return
    // the mapped value's address.
    V& operator[](int key)
    {
        node* n = find(key);
        if (n == head)
            insert(key, V(), &n);
        return n->value;
    }

    // erase(iterator) (sub_100F63D0 / sub_10128AD0's shape). Transplants the successor node on a
    // two-child erase so every surviving node keeps its address, as the shipped erase does.
    void erase(node* z)
    {
        if (!head || !z || z == head)
            return;

        node* replacement;
        if (z->left == head)
            replacement = z->right;
        else if (z->right == head)
            replacement = z->left;
        else
        {
            node* successor = z->right;
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

        node* root = head->parent;
        if (root == head)
        {
            head->left  = head;
            head->right = head;
        }
        else
        {
            node* leftmost = root;
            while (leftmost->left != head)
                leftmost = leftmost->left;
            node* rightmost = root;
            while (rightmost->right != head)
                rightmost = rightmost->right;
            head->left  = leftmost;
            head->right = rightmost;
        }

        ::renddx9abi::engineFree(z);
        --size;
    }

    // erase(key) (sub_10131EB0 / sub_10132340): equal_range + erase, returning the count removed.
    unsigned erase(int key)
    {
        node* n = find(key);
        if (n == head)
            return 0u;
        erase(n);
        return 1u;
    }

    // erase(first, last) (sub_10132280, VC7.1 _Tree::erase(iterator, iterator)): the whole range
    // takes the clear arm and returns begin(); any other range goes node by node and returns
    // `last`. Taking the successor before each erase is safe because erase(node*) keeps every
    // surviving node at its address.
    node* erase(node* first, node* last)
    {
        if (!head)
            return last;
        if (size != 0 && first == begin() && last == end())
        {
            clear();
            return begin();
        }
        while (first != last)
        {
            node* const victim = first;
            first = next(first);
            erase(victim);
        }
        return first;
    }

    // Copies tree shape and colors after clearing this constructed map, then fixes extrema
    // and size. Uses an iterative walk; allocation failure leaves the map empty and returns false.
    bool copyFrom(const int_map_layout& other)
    {
        if (!head || !other.head)
            return false;
        clear();

        node* const sourceRoot = other.head->parent;
        if (sourceRoot == other.head)
            return true;

        node* const root = cloneNode(sourceRoot, head);
        if (!root)
            return false;
        head->parent = root;

        node* s = sourceRoot;
        node* d = root;
        for (;;)
        {
            if (!s->left->isNil && d->left == head)
            {
                node* const c = cloneNode(s->left, d);
                if (!c) { clear(); return false; }
                d->left = c;
                s = s->left;
                d = c;
                continue;
            }
            if (!s->right->isNil && d->right == head)
            {
                node* const c = cloneNode(s->right, d);
                if (!c) { clear(); return false; }
                d->right = c;
                s = s->right;
                d = c;
                continue;
            }
            if (s == sourceRoot)
                break;
            s = s->parent;
            d = d->parent;
        }

        node* leftmost = root;
        while (leftmost->left != head)
            leftmost = leftmost->left;
        node* rightmost = root;
        while (rightmost->right != head)
            rightmost = rightmost->right;
        head->left  = leftmost;
        head->right = rightmost;
        size = other.size;
        return true;
    }

    // Frees nodes iteratively and restores the sentinel, avoiding recursion on unbalanced
    // trees. Mapped pointer/integer values are not destroyed.
    void clear()
    {
        if (!head)
        {
            size = 0;
            return;
        }
        node* n = head->parent;
        while (n != head)
        {
            if (n->left != head)  { n = n->left;  continue; }
            if (n->right != head) { n = n->right; continue; }
            node* parent = n->parent;
            if (parent != head)
            {
                if (parent->left == n) parent->left = head;
                else                   parent->right = head;
            }
            ::renddx9abi::engineFree(n);
            n = parent;
        }
        head->parent = head;
        head->left   = head;
        head->right  = head;
        size = 0;
    }

private:
    node* newNode(int key, const V& value)
    {
        node* n = static_cast<node*>(::renddx9abi::engineAlloc(sizeof(node)));
        if (!n)
            return 0;
        n->left = head; n->parent = head; n->right = head;
        n->key = key;
        n->value = value;
        n->color = 1;
        n->isNil = 0;
        n->pad[0] = 0; n->pad[1] = 0;
        return n;
    }

    // sub_10131730 (_Buynode for the copy): the source's key, value and colour under `parent`.
    node* cloneNode(const node* source, node* parent)
    {
        node* const n = newNode(source->key, source->value);
        if (n)
        {
            n->parent = parent;
            n->color = source->color;
        }
        return n;
    }
};

#if defined(_M_IX86)
static_assert(sizeof(int_map_node<int>) == 0x18, "sub_10131690/sub_1008A320 allocate 0x18");
static_assert(sizeof(int_map_node<void*>) == 0x18, "a pointer-valued node is the same 0x18");
static_assert(sizeof(int_map_layout<int>) == 12, "the VC7.1 std::map header is 12 bytes");
#endif

} // namespace vc71

#endif // RENDDX9ABI_VC71INTMAP_H
