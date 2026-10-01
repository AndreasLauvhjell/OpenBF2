// Named HUD split node. [INFERRED] Header placement. Untyped Node slots remain excluded from native dispatch.
#ifndef MENU_BF2SPLITNODE_H
#define MENU_BF2SPLITNODE_H
#include "../External/BFMeme/SplitNode.h"
#if !DEDICATED
namespace dice { namespace meme {
class Bf2SplitNode : public SplitNode
{
public:
    explicit Bf2SplitNode(const char* name);
    ~Bf2SplitNode() override;                             // 7CB0F0, scalar name delete
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    const char* getName() override;
    bool setName(const char*) override;
    bool slot_0x24() override;                            // 5A5E20, actual true default
    static TypeDescriptor* getClassType();                // 7CB070, same descriptor as getType
private:
    char* m_name;                                        // retail+0C; owns nullable clone
};
} }
#endif
#endif
