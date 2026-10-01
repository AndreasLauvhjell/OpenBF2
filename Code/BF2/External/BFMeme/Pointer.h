// Path [INFERRED] beside the attested BFMeme sources. Descriptor8291C0
// attests dice::meme::Pointer; complete23-slot client table9493A8.
#ifndef BFMEME_POINTER_H
#define BFMEME_POINTER_H
#include "Object.h"
#if !DEDICATED
namespace dice { namespace meme {
class Node;
class Pointer : public Object
{
public:
    Pointer();
    ~Pointer() override;                           // 829160, does not own graphic
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    virtual bool paint(System*);
    virtual bool update(System*, float delta);
    virtual bool setPosition(System*, float x, float y);
    virtual bool event1(System*, int first, int index, int second); // +4C 829050
    virtual bool event2(System*, int first, int index);
    virtual bool event4(System*, int first, int index, int second);
    virtual bool event5(System*, int first, int index);
    void setGraphic(System*, Node*);
    void setSize(float width, float height) { m_width=width; m_height=height; } // [INFERRED] accessor; HUD75121F/75124C direct stores
    bool isEnabled() const { return m_enabled; }    // inlined828D70..828E20
    void setEnabled(bool enabled) { m_enabled = enabled; } // [INFERRED] accessor, HUD751318 direct store
    static TypeDescriptor* getClassType();
protected:
    float m_width, m_height;                        // +04/+08
    Node* m_graphic;                                // +0C
    bool m_enabled;                                // +10, padding untouched
};
// [INFERRED] source names; typed versions of the guarded fastcall wrappers.
bool paintPointer(System*, Pointer*);
bool updatePointer(System*, Pointer*, float);
bool positionPointer(System*, Pointer*, float, float);
bool pointerEvent1(System*, Pointer*, int, int, int);
bool pointerEvent2(System*, Pointer*, int, int);
} }
#endif
#endif
