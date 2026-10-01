// Path and helper name [INFERRED] beside the attested BFMeme sources.
// Client838FB0 operates on the two-float NodeState defined in Node.h.
#ifndef BFMEME_NODESTATE_H
#define BFMEME_NODESTATE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
// transitionSource is a Node pointer in callers; its nonvirtual rate helper
// 82BD30 does not read the receiver, including when the root supplies null.
void updateNodeState(NodeState*, System*, Node* transitionSource, Node* eventRoot,
                     const Rect*, float delta, bool pointerInside, bool enabled);
} }
#endif
#endif
