// Path:     [INFERRED] TextureAtlasBuilder/Packer.cpp - the 2006 sibling's name, which is attested (see
//           Packer.h).
// Contents: from TextureAtlasBuilder.dll 1.50 (TAB 0x10001E60..0x1000216C), every
//           instruction read, cross-checked against the 2006 debug build (E9 0x10066A10..0x10066E80):
//           the same split rule, the same child order; release drops the asserts.

// TextureAtlasBuilder.dll only - see TextureFormat.cpp for the guard.
#if defined(BF2_IMAGE_TEXTUREATLASBUILDER)

#include "Packer.h"

namespace textureatlas {

// Inlined at each of the four `new Node` sites: x, y, w, h, then both children null.
Node::Node(int x_, int y_, int w_, int h_)
    : rect(x_, y_, w_, h_)
{
    child[0] = 0;
    child[1] = 0;
}

// (56 B, the scalar deleting destructor, `retn 4`): child 0, then child 1, each deleted
// through the same function when present; then the node itself when the flag says so.
Node::~Node()
{
    delete child[0];
    delete child[1];
}

// E9 0x10009AED; the 1.50 image inlines it into insert (0x10001FBB..0x10001FD3) and Packer::fits.
bool Node::fits(const Rect& r) const
{
    return r.w <= rect.w && r.h <= rect.h;
}


// Places r at a fitting leaf's corner, splitting along the shorter leftover dimension.
// Internal nodes try child 0 then child 1. Children are created as a pair and are not
// independently null-checked. Even a perfect fit creates remainder children.

bool Node::insert(Rect& r)
{
    if (!fits(r))
        return false;

    if (child[0])
        return child[0]->insert(r) || child[1]->insert(r);

    const int dw = rect.w - r.w;
    const int dh = rect.h - r.h;
    if (dw <= dh)
    {
        child[0] = new Node(rect.x + r.w, rect.y, dw, r.h);
        child[1] = new Node(rect.x, rect.y + r.h, rect.w, dh);
    }
    else
    {
        child[1] = new Node(rect.x + r.w, rect.y, dw, rect.h);
        child[0] = new Node(rect.x, rect.y + r.h, r.w, dh);
    }

    r.x = rect.x;
    r.y = rect.y;
    return true;
}

// (9 B).
Packer::Packer()
    : root(0)
{
}

// the null test and the reset are both inside the branch (0x10001EB8 jumps past
// the `root = 0` store), unlike clear() below - the two bodies are not the same function.
Packer::~Packer()
{
    if (root)
    {
        delete root;
        root = 0;
    }
}

// a new root covering the whole page. The previous root is NOT
// deleted here - shipped (build() clears after every page; E9 asserts it is null).
void Packer::init(int width, int height)
{
    root = new Node(0, 0, width, height);
}

// delete the tree, then `root = 0` unconditionally.
void Packer::clear()
{
    delete root;
    root = 0;
}

// (11 B): `mov ecx, [ecx]; jmp Node::insert` - no null test in release.
bool Packer::insert(Rect& r)
{
    return root->insert(r);
}

// would the rectangle fit an empty page? - the root's own size.
bool Packer::fits(const Rect& r)
{
    return root->fits(r);
}

} // namespace textureatlas

#endif // BF2_IMAGE_TEXTUREATLASBUILDER
