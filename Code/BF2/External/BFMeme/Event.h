// Name descriptor-attested83B080; path/method names [INFERRED]. This Object
// family is an input predicate, distinct from the plain NodeInputEvent payload.
#ifndef BFMEME_EVENT_H
#define BFMEME_EVENT_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
struct NodeInputEvent;
class Event : public Object
{
public:
    Event();                                      // 83B0C0 inlined construction
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    virtual bool matches(System*, const NodeInputEvent*);
    static TypeDescriptor* getClassType();
private:
    unsigned char m_inputIndex;                    // +4, zero means every input
};
} }
#endif
#endif
