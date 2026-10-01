// [TEMP] [CHOSEN, not recovered] Focused xpack mask regression, never a production target.
// Uses actual registered factories, template ID allocation, console bindings and MaskObject bodies.
// The class-manager adapter exposes real module rows and deliberately models an absent class only.
// No socket, map load, retail client, geometry carrier, or runtime destruction is exercised.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/ConsoleObjectMacro.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/Modules.h"
#include "../BF2/Game/Objects/Game/MaskObject.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/Physics/ResponsePhysicsManager.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

namespace dice { namespace hfe { extern ClassManager* classManager;
namespace world { extern ResponsePhysicsManager* responsePhysicsManager; } } }
using namespace dice::hfe;
using namespace dice::hfe::world;

namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 14; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset, symbol->Name, displacement);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                         0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
class FixtureClasses : public ClassManager
{
public:
    std::vector<ClassRegInfo> entries;
    FixtureClasses()
    {
        GameLogicObjTemplBFModule().getClassList(entries);
        GameLogicObjTemplBaseModule().getClassList(entries);
        ObjTemplBaseModule().getClassList(entries);
    }
    IBase* createInstance(unsigned int cid, unsigned int iid, IBase* outer) override
    {
        if (cid == CID_BinTreeCuller) return BinTreeCuller::create(iid, outer);
        if (cid == CID_QuadTreeCuller) return QuadTreeCuller::create(iid, outer);
        for (const auto& entry : entries)
            if (entry.classId == cid) return entry.create(iid, outer);
        return 0; // [TEMP] The test's explicitly absent class, never a production registration.
    }
    bool getClassName(unsigned int cid, std::string& name) override
    {
        for (const auto& entry : entries)
            if (entry.classId == cid) { name = entry.name; return true; }
        return false;
    }
};
ConsoleObject* binding(const char* name)
{
    for (ConsoleObject* object : consoleBindingList("world.objTemplGame"))
        if (std::strcmp(object->getFunctionName(), name) == 0) return object;
    return 0;
}
std::string set(ConsoleObject* object, const char* value)
{
    std::string argument(value);
    return object->execute(&argument, 1);
}
std::string get(ConsoleObject* object) { return object->execute(0, 0); }
bool vectorTextIs(const std::string& text, float x, float y, float z)
{
    Vec3 value = {};
    std::istringstream input(text);
    input >> value;
    return !input.fail() && std::fabs(value.x - x) < 0.00001f
        && std::fabs(value.y - y) < 0.00001f && std::fabs(value.z - z) < 0.00001f;
}
void enumRegression()
{
    // Both shipped enum operators establish this external textual contract:
    // ELF 0x857E430/490, PE 0x47D840/0x47F1F0. Case and unknown-token behavior matter.
    struct Row { const char* token; int value; const char* printed; };
    const Row rows[] = {
        {"MOTNightVisionGoggles", 0, "MOTNightVisionGoggles"},
        {"NightVisionGoggles", 0, "MOTNightVisionGoggles"},
        {"0", 0, "MOTNightVisionGoggles"},
        {"MOTGasMask", 1, "MOTGasMask"}, {"GasMask", 1, "MOTGasMask"}, {"1", 1, "MOTGasMask"},
        {"MOTNoMask", 2, "MOTNoMask"}, {"NoMask", 2, "MOTNoMask"}, {"2", 2, "MOTNoMask"},
        {"gasmask", 2, "MOTNoMask"}, {"unknown", 2, "MOTNoMask"}, {"-1", 2, "MOTNoMask"}
    };
    for (const Row& row : rows)
    {
        MaskObjectType value = static_cast<MaskObjectType>(99);
        std::istringstream input(row.token);
        input >> value;
        std::ostringstream output;
        output << value;
        check(!input.fail() && static_cast<int>(value) == row.value && output.str() == row.printed,
              row.token);
    }
    std::ostringstream invalid;
    invalid << static_cast<MaskObjectType>(-1) << static_cast<MaskObjectType>(3);
    check(invalid.str().empty(), "unrecognized output enum values produce no text");
}
}

int main()
{
    static_assert(sizeof(void*) == 4, "Dedicated x86 fixture required");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    enumRegression();
    eventManager = new EventManager();
    responsePhysicsManager = new ResponsePhysicsManager();
    classManager = new FixtureClasses();
    io::mainConsole = new io::OldConsole();
    objectTemplateManager = new ObjectTemplateManager();
    objectManager = new ObjectManager();

    IObjectTemplate* before = objectTemplateManager->createTemplate(CID_SimpleObjectTemplate, "before_masks");
    check(before != 0, "control template factory is available");
    if (!before) return 1;
    std::vector<MaskObjectTemplate*> masks;
    for (int i = 0; i < 26; ++i)
    {
        std::ostringstream name;
        name << "xpack_mask_" << i;
        IObjectTemplate* objectTemplate = objectTemplateManager->createTemplate(CID_MaskObjectTemplate, name.str());
        check(objectTemplate && objectTemplate->getClassID() == CID_MaskObjectTemplate
              && objectTemplate->getId() == before->getId() + i + 1, "mask factory consumes exactly one sequential object ID");
        if (!objectTemplate) return 1;
        masks.push_back(static_cast<MaskObjectTemplate*>(objectTemplate));
    }
    check(objectTemplateManager->createTemplate(CID_MaskObjectTemplate, "XPACK_MASK_0") == masks.front(),
          "case-insensitive duplicate name reopens the existing template");
    check(!objectTemplateManager->createTemplate(0xffffffffu, "absent_class")
          && !objectTemplateManager->getActiveObjectTemplate(), "absent factory clears the active template");
    IObjectTemplate* after = objectTemplateManager->createTemplate(CID_SimpleObjectTemplate, "after_masks");
    check(after && after->getId() == before->getId() + 27,
          "26 real masks advance later template IDs by 26; duplicate and failed creates consume none");
    MaskObjectTemplate* maskTemplate = masks.front();
    check(maskTemplate->queryImplementation(CID_MaskObjectTemplate) == maskTemplate
          && maskTemplate->queryImplementation(CID_AnimatedBundleTemplate) == maskTemplate
          && maskTemplate->queryImplementation(CID_SimpleObjectTemplate) == maskTemplate,
          "registered template exposes its concrete and inherited implementation identities");
    objectTemplateManager->setActiveObjectTemplate(maskTemplate);
    ConsoleObject* type = binding("maskType");
    ConsoleObject* offset = binding("maskOffset");
    check(type && offset, "both actual mask console bindings are published");
    if (!type || !offset) return 1;
    check(type->getType() == 1 && type->getMinNoArgs() == 0 && type->getMaxNoArgs() == 1
          && std::strcmp(type->getReturnType(), "hfe::MaskObjectType") == 0 && !type->hasRange(),
          "maskType uses the retail enum property contract");
    check(offset->getType() == 1 && offset->getMinNoArgs() == 0 && offset->getMaxNoArgs() == 1
          && std::strcmp(offset->getReturnType(), "Vec3") == 0 && !offset->hasRange(),
          "maskOffset uses the retail vector property contract");
    check(get(type) == "MOTNoMask" && vectorTextIs(get(offset), 0, -1.55f, 0.08f),
          "factory product exposes retail default mask type and offset");
    set(type, "GasMask");
    check(maskTemplate->getMaskType() == MOTGasMask && get(type) == "MOTGasMask",
          "stock GasMask token updates the real template and prints canonical form");
    set(offset, "0.25/-1.75/0.12");
    check(vectorTextIs(get(offset), 0.25f, -1.75f, 0.12f), "authored slash-vector maskOffset roundtrips");
    set(type, "NightVisionGoggles");
    check(maskTemplate->getMaskType() == MOTNightVisionGoggles, "stock goggles token reaches the typed setter");
    objectTemplateManager->setActiveObjectTemplate(before);
    check(!type->isActive() && !offset->isActive(), "mask properties reject an unrelated active template");
    objectTemplateManager->setActiveObjectTemplate(maskTemplate);

    MaskObject* mask = static_cast<MaskObject*>(maskTemplate->createObject());
    check(mask && mask->getTemplate() == maskTemplate && mask->getType() == MOTNightVisionGoggles,
          "real runtime factory preserves its template and exposes the mask type");
    if (!mask) return 1;
    maskTemplate->setMaskType(MOTGasMask);
    check(mask->getType() == MOTGasMask, "runtime type forwards to the live template");
    mask->init();
    check((mask->getFlags() & 0x1800u) == 0x1800u && !(mask->getFlags() & 2u),
          "init enables mask update flags and starts hidden");
    check(!mask->getSoldier() && !mask->getNetworkable(), "unattached mask has no soldier or invented networkable");
    mask->updateFlags(2, 0);
    mask->handleUpdate(1.0f / 30.0f, 1);
    check(!(mask->getFlags() & 2u), "update hides an unattached mask");
    mask->handleVisualUpdate(1.0f / 30.0f, 0.5f);
    check(!mask->getSoldier() && !(mask->getFlags() & 2u), "empty geometry visual update remains quiescent");
    std::printf("xpack mask fixture: %d checks, %d failures\n", checks, failures);
    std::fflush(stdout);
    // Objects intentionally survive until process exit; destruction is not acceptance here.
    return failures ? 1 : 0;
}
