// Path [INFERRED]. All primary/metadata targets, eager/factory and differing
// bodies read from the client PE before implementation. See the status document.
#include "AlphaFadeEffect.h"
#if !DEDICATED
#include "Node.h"
#include "Quad.h"
#include "TypeDescriptor.h"

namespace dice { namespace meme {
namespace {
int alphaFadeEffectInstances = 0;                     // A39FC0, loader-zeroed
// Concrete descriptor spelling [INFERRED]; complete table94DCD8.
class AlphaFadeEffectTypeDescriptor : public TypeDescriptor
{
public:
    AlphaFadeEffectTypeDescriptor() : TypeDescriptor("dice::meme::AlphaFadeEffect") {}
    TypeDescriptor* getBaseType() override { return Effect::getClassType(); }
    RuntimeObject* create() override
    {
        void* allocation = ::operator new(sizeof(AlphaFadeEffect));
        return allocation ? new(allocation) AlphaFadeEffect : nullptr;
    }
    void addInstance() override { ++alphaFadeEffectInstances; }
    void removeInstance() override { --alphaFadeEffectInstances; }
    int getInstanceCount() override { return alphaFadeEffectInstances; }
    unsigned int getInstanceSize() override { return sizeof(AlphaFadeEffect); }
};

// [INFERRED] helper spelling for the repeated 834F10/834F70 instructions.
// Keep x87 operand order/control semantics, including two-NaN payload choice;
// native mulss does not reproduce that case. Both operands are loaded afresh.
void multiplyAlpha(float* alpha, const float* factor)
{
    __asm {
        mov edx, factor
        mov eax, alpha
        fld dword ptr [edx]
        fmul dword ptr [eax]
        fstp dword ptr [eax]
    }
}
}

TypeDescriptor* AlphaFadeEffect::getClassType()
{
    static AlphaFadeEffectTypeDescriptor type;
    return &type;
}
TypeDescriptor* AlphaFadeEffect::getType() { return getClassType(); }
namespace {
// CRT96E1D0 ->871560 uses the same guarded descriptor as the getter, stores
// A3A03C and registers87E0A0 teardown. Native static lifetime owns that storage.
TypeDescriptor* const registeredAlphaFadeEffectType = AlphaFadeEffect::getClassType();
}

bool AlphaFadeEffect::drawCharacter(System* system, int character, int font,
                                    int elementIndex, Quad* quad, RenderContext* context)
{
    // each component is read, multiplied and stored as float32 before
    // the next. Do not snapshot the context value or restore colors afterward.
    for (auto& corner : quad->corners)
        multiplyAlpha(&corner.color.a, &context->field18);
    return Effect::drawCharacter(system, character, font, elementIndex, quad, context);
}
bool AlphaFadeEffect::drawTexture(System* system, int texture, Quad* quad,
                                  RenderContext* context)
{
    for (auto& corner : quad->corners)
        multiplyAlpha(&corner.color.a, &context->field18);
    return Effect::drawTexture(system, texture, quad, context);
}

static_assert(sizeof(AlphaFadeEffect) == 4, "Native x86 AlphaFadeEffect allocation");
} }
#endif
