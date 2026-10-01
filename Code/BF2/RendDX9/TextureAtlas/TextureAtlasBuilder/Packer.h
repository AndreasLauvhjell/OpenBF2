// Binary-tree rectangle packer. Every placement splits the leaf, including perfect fits.

#ifndef TEXTUREATLASBUILDER_PACKER_H
#define TEXTUREATLASBUILDER_PACKER_H

#include "ITextureAtlas.h"      // Rect

namespace textureatlas {

// The 0x18-byte tree node: the free rectangle, then the two children (both null for a leaf).
struct Node
{
    Node(int x_, int y_, int w_, int h_);   // inlined at every `new` (TAB 0x10001F06..0x10001F1A)
    ~Node();                                // (the scalar deleting destructor)

    bool fits(const Rect& r) const;         // E9 0x10009AED; inlined in the 1.50 image
    bool insert(Rect& r);

    Rect  rect;                             // +0x00
    Node* child[2];                         // +0x10, +0x14
};

class Packer
{
public:
    Packer();                               // 0x10001EA0 `root = 0`
    ~Packer();

    void init(int width, int height);       // (E9: assert(!root))
    void clear();                           // (E9: assert(root))
    bool insert(Rect& r);                   // (E9: assert(root))
    bool fits(const Rect& r);               // (E9: assert(root))

private:
    Node* root;                             // +0x00
};

} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_PACKER_H
