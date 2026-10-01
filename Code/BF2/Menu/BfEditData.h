// Editable HUD text data. [INFERRED] Header placement and helper names.
#ifndef BF2_MENU_BFEDITDATA_H
#define BF2_MENU_BFEDITDATA_H
#include "../External/BFMeme/Data.h"
#if !DEDICATED
#include <list>
namespace dice { namespace hfe { namespace hud { class Bf2HudBuilder; } } }
namespace dice { namespace meme {
class BfEditNode;
class BfEditNodeInt;
class BfEditData : public Data
{
    friend class BfEditNode;
    friend class BfEditNodeInt;
    // PE797119: the HUD builder's setEditNodeData row stores the node into +14
    // directly, so the builder had access in the original source.
    friend class dice::hfe::hud::Bf2HudBuilder;
public:
    explicit BfEditData(const char* name = nullptr);
    ~BfEditData() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    static TypeDescriptor* getClassType();          // descriptor934DE0

    void registerNode(BfEditNode*);                 // 7AC510; append, does not select
    void queueKey(int key);
    void queueCharacter(int character);
    void clear(System*);
    void setFocus(bool);
    void setCursorPosition(int);                  // inlined HUD7B13F7/FD; no active-node guard
private:
    std::list<BfEditNode*> m_nodes;                 // retail+08; borrowed elements
    BfEditNode* m_activeNode;                       // retail+14; borrowed, last streamed node
    bool m_focusLocked;                            // retail+18; prevents setFocus writes
    bool m_hideText;                               // retail+19; tested by BfEditNode paint
};
} }
#endif
#endif
