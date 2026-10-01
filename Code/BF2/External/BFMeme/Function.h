// Name descriptor-attested82E510; path/method names inferred. Full17 slots.
#ifndef BFMEME_FUNCTION_H
#define BFMEME_FUNCTION_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
class Function : public Object
{
public:
    explicit Function(const char* name = nullptr); // 82E550; factory82E570 passes null
    ~Function() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    const char* getName() override;
    bool setName(const char*) override;
    virtual bool call(System*);
    static TypeDescriptor* getClassType();
private:
    char* m_name;                                 // +4, owned copy
};
} }
#endif
#endif
