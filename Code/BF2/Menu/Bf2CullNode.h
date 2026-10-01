// HUD culling node. [INFERRED] Header placement; undeclared Node slots remain excluded.
#ifndef MENU_BF2CULLNODE_H
#define MENU_BF2CULLNODE_H
#include "../External/BFMeme/CullNode.h"
#if !DEDICATED
namespace dice { namespace meme {
class Bf2CullNode : public CullNode
{
public:
    Bf2CullNode(const char* name, Data* data);
    ~Bf2CullNode() override;                              // 7CE260, owned name only
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    const char* getName() override;
    bool setName(const char*) override;
    bool slot_0x24() override;
    bool update(System*, const NodeState*, const Rect*, float) override;
    static TypeDescriptor* getClassType();                // 7CE300, metadata9385E0

    // [CHOSEN, not recovered] Direct accessors for builder-created culling nodes.
    // [RETIRE-WHEN: bf2-cull-native-accessors] Replace after the complete Node interface permits validated virtual dispatch.
    Data* getDataForHud();                                // 68E7B0, +C8
    bool setDataForHud(Data*);                            // 7CDFC0, +CC
    bool setOutTimeForHud(float);
    bool setInTimeForHud(float);                          // 7CE250, +D4
protected:
    char* m_name;                                        // +18, owns nullable clone
    int m_previousVisibility;                            // +1C, [INFERRED] name
};
} }
#endif
#endif
