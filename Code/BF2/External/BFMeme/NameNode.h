// Named node with the known Node prefix. Unknown suffix slots remain undeclared.
#ifndef BFMEME_NAMENODE_H
#define BFMEME_NAMENODE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
class NameNode : public Node
{
public:
    explicit NameNode(const char* name = nullptr);         // 837620; factory8376A0 supplies null
    ~NameNode() override;                                 // 8375A0/scalar837660
    TypeDescriptor* getType() override;                   // 8376F0, real metadata; suffix caller stops retained
    unsigned int getObjectSize() override;
    const char* getName() override;
    bool setName(const char*) override;
    bool slot_0x20() override;                            // 5A5E20, actual true default
    bool slot_0x24() override;                            // 5A5E20, actual true default
    static TypeDescriptor* getClassType();
private:
    char* m_name;                                        // [INFERRED] field name; retail+08 owns nullable clone
};
} }
#endif
#endif
