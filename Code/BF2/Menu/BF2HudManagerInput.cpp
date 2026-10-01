// [INFERRED] split in the attested BF2HudManager.cpp family. Complete653-byte
// PE752B00 and its helpers read. Filters stay separate.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Ref2System.h"
#include "../External/BFMeme/Pointer.h"
#include "../External/BFMeme/Style.h"
#include "../IO/Input/Input.h"
namespace dice { namespace hfe { namespace io { extern IBase* inputDeviceManager; } } }
namespace dice { namespace hfe { namespace hud {
namespace {
float centeredInputOrigin(float origin, float dimension)
{
    const float half = 0.5f;
    float result;
    __asm {
        fld dimension
        fmul half
        fsubr origin
        fstp result
    }
    return result;
}

void moveInputPointer(float* position, float x, float y, int sensitivity,
                      bool absolute, float width, float height)
{
    const float half = 0.5f;
    if (absolute) {
        __asm {
            mov eax, position
            fld x
            fmul width
            fmul half
            fstp dword ptr [eax]
            fld y
            fmul height
            fmul half
            fstp dword ptr [eax+4]
        }
    } else {
        // fild preserves all bits of the signed sensitivity until multiplication.
        __asm {
            mov eax, position
            fild sensitivity
            fld x
            fmul st(0), st(1)
            fadd dword ptr [eax]
            fstp dword ptr [eax]
            fmul y
            fadd dword ptr [eax+4]
            fstp dword ptr [eax+4]
        }
    }
}

void clampInputCoordinate(float& value, float origin, float extent)
{
    // Keep the unrounded upper-bound sum and the shipped unordered comparisons.
    __asm {
        mov edx, value
        fld dword ptr [edx]
        fcomp origin
        fnstsw ax
        test ah, 5
        jp upper_bound
        mov eax, origin
        mov [edx], eax
        jmp clamp_done
    upper_bound:
        fld extent
        fadd origin
        fcom dword ptr [edx]
        fnstsw ax
        test ah, 5
        jp upper_unchanged
        fstp dword ptr [edx]
        jmp clamp_done
    upper_unchanged:
        fstp st(0)
    clamp_done:
    }
}
}

void BF2HudManager::handleGameInput(float /*delta*/, io::GameInput& input)
{
    if (!ref2System) return;
    ref2System->setInput(input, 0);
    if (!m_field258) return;
    int sensitivity = m_field180;
    const float y = (input.mappingFlags & 0x200000u) ? input.inputs[21] : 0.0f;
    const float x = (input.mappingFlags & 0x100000u) ? input.inputs[20] : 0.0f;
    meme::Rect bounds = {-400.0f, -300.0f, 800.0f, 600.0f, 0.0f};
    // Do not cache getMode: every comparison is a separate retail virtual call.
    if (getMode() == 3 || getMode() == 4 || getMode() == 5 || getMode() == 7
            || getMode() == 8 || getMode() == 30 || getMode() == 31) {
        bounds.width = m_field200;
        bounds.height = m_field204;
        bounds.x = centeredInputOrigin(m_field208, ref2System->getWidth());
        bounds.y = centeredInputOrigin(m_field20c, ref2System->getHeight());
        sensitivity = m_field17c;
    } else if (getMode() == 6) {
        bounds.width = m_field218;
        bounds.height = m_field21c;
        bounds.x = centeredInputOrigin(m_field220, ref2System->getWidth());
        bounds.y = centeredInputOrigin(m_field224, ref2System->getHeight());
        sensitivity = m_field17c;
    }
    float* position = ref2System->getPointerPosition();
    const bool absolute = static_cast<io::IInputDeviceManager*>(io::inputDeviceManager)
            ->getInputDevice(io::IDFFalcon)
        && static_cast<io::IInputDeviceManager*>(io::inputDeviceManager)
            ->getInputDevice(io::IDFFalcon)->isActive();
    moveInputPointer(position, x, y, sensitivity, absolute, bounds.width, bounds.height);
    clampInputCoordinate(position[0], bounds.x, bounds.width);
    clampInputCoordinate(position[1], bounds.y, bounds.height);
    const float pointerX = position[0], pointerY = position[1];
    auto* pointer = static_cast<meme::Pointer*>(ref2System->getPointerObject());
    meme::positionPointer(ref2System, pointer, pointerX, pointerY);
}
} } }
#endif
