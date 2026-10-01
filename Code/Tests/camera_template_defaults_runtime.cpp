// [TEMP] [CHOSEN, not recovered] Bounded constructor regression, not a game launch.
// Uses the real CameraTemplate and all real base constructors/destructors. The
// isolated engine link is diagnostic /FORCE; only these exercised paths are checked.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <new>
#include "../BF2/Game/Objects/CameraTemplate.h"

namespace {
int checks = 0;
int failures = 0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) ++failures;
    std::printf("%s %s\n", value ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}

// Test-only access to the fields retail console bindings author directly.
// Constructor and getters remain the production implementations.
struct InspectCameraTemplate : dice::hfe::world::CameraTemplate
{
    using CameraTemplate::m_cockpitLod;
    using CameraTemplate::m_cockpitSubGeom;
};

LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    std::fprintf(stderr, "FAIL: constructor fixture exception %08lx at %p\n",
        fault->ExceptionRecord->ExceptionCode, fault->ExceptionRecord->ExceptionAddress);
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
}

int main()
{
    static_assert(sizeof(void*) == 4, "CameraTemplate fixture requires x86");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    const unsigned char patterns[] = { 0xCD, 0x3C, 0x00 };
    for (unsigned int i = 0; i != sizeof(patterns); ++i)
    {
        alignas(InspectCameraTemplate) unsigned char storage[sizeof(InspectCameraTemplate)];
        std::memset(storage, patterns[i], sizeof(storage));
        InspectCameraTemplate* camera = new (storage) InspectCameraTemplate;
        std::printf("preallocation fill=%02X: cockpitLod=%08X cockpitSubGeom=%08X\n",
            patterns[i], static_cast<unsigned int>(camera->getCockpitLod()),
            static_cast<unsigned int>(camera->getCockpitSubGeom()));
        check(camera->getCockpitLod() == -1, "constructor installs the cockpit LOD fallback sentinel");
        check(camera->getCockpitSubGeom() == -1, "constructor installs the cockpit geometry fallback sentinel");
        camera->m_cockpitLod = 2;
        camera->m_cockpitSubGeom = 7;
        check(camera->getCockpitLod() == 2 && camera->getCockpitSubGeom() == 7,
              "explicit template overrides remain independent and visible through real getters");
        camera->m_cockpitSubGeom = -1;
        check(camera->getCockpitLod() == 2 && camera->getCockpitSubGeom() == -1,
              "restoring geometry fallback preserves the explicit LOD override");
        camera->~InspectCameraTemplate();
    }
    std::printf("CameraTemplate defaults: %d/%d checks passed (actual constructor/base lifetimes; diagnostic /FORCE link)\n",
        checks - failures, checks);
    return failures ? 1 : 0;
}
