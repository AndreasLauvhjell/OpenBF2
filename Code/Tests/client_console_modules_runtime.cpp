// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: CLIENT-CONSOLE-MODULE-FIXTURE]
// Tests real module registration, profile ownership, and release-to-zero allocation behavior.
// Models only HUD singleton availability. Unexercised builder graph dependencies remain visible in diagnostic links.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>
#include <crtdbg.h>
#include "../BF2/Menu/ClientConsoleModules.h"
#include "../BF2/Menu/HudConsoleBindings.h"
#include "../BF2/Menu/Bf2HudBuilder.h"
#include "../BF2/Menu/ProfileConsoleBindings.h"
#include "../BF2/Core/ConsoleObjectBase.h"
#include <set>
#include <cstdint>
#include "../BF2/Core/Ids.h"

namespace dice { namespace hfe { namespace hud { class BF2HudManager; }
hud::BF2HudManager* hudManager = nullptr;
IBase* g_hudInformationLayer = nullptr; // Uncalled real builder lookup dependency.
} }

static void* watchedAllocation;
static unsigned watchedDeletes;
void* operator new(std::size_t size)
{
    void* value = std::malloc(size ? size : 1);
    if (!value) throw std::bad_alloc();
    return value;
}
void operator delete(void* value) noexcept
{
    if (value && value == watchedAllocation) ++watchedDeletes;
    std::free(value);
}
void operator delete(void* value, std::size_t) noexcept { ::operator delete(value); }

using namespace dice::hfe;
using namespace dice::hfe::hud;
static unsigned checks, failures;
static void check(bool condition, const char* label)
{ ++checks; if (!condition) { ++failures; std::printf("FAIL %s\n", label); } }

template<class Module> void lifetime()
{
    check(sizeof(Module) == 8, "native x86 module size8");
    Module* concrete = new Module;
    watchedAllocation = concrete;
    watchedDeletes = 0;
    IModule* module = concrete;
    const IModule* query = module;
    check(module->getRef() == 1, "initial reference is one");
    for (unsigned id : {IID_IBase, IID_IClassServer, IID_IModule})
        check(query->queryInterface(id) == concrete, "accepted interface uses primary address");
    for (unsigned id : {0u, 2u, 39999u, 40001u, 0xffffffffu})
        check(query->queryInterface(id) == nullptr, "unknown interface rejected");
    check(module->getRef() == 1, "queryInterface does not add reference");
    std::vector<ClassRegInfo> classes;
    std::vector<SingletonRegInfo> singletons;
    module->getClassList(classes);
    module->getSingletonList(singletons);
    check(classes.empty() && singletons.empty(), "actual empty class and singleton declarations");
    classes.push_back(ClassRegInfo{17, "preserve-class", nullptr});
    singletons.push_back(SingletonRegInfo{"preserve-singleton", nullptr, 23, 41, 7});
    const auto* classStorage = classes.data();
    const auto* singletonStorage = singletons.data();
    const auto classCapacity = classes.capacity();
    const auto singletonCapacity = singletons.capacity();
    module->getClassList(classes);
    module->getSingletonList(singletons);
    check(classes.size() == 1 && classes.data() == classStorage && classes.capacity() == classCapacity,
        "class list storage remains untouched");
    check(classes[0].classId == 17 && classes[0].name == "preserve-class" && !classes[0].create,
        "class list contents remain untouched");
    check(singletons.size() == 1 && singletons.data() == singletonStorage && singletons.capacity() == singletonCapacity,
        "singleton list storage remains untouched");
    check(singletons[0].name == "preserve-singleton" && !singletons[0].instance && singletons[0].iid == 23
        && singletons[0].classId == 41 && singletons[0].flags == 7, "singleton contents remain untouched");
    module->addRef();
    module->addRef();
    check(module->getRef() == 3, "two references retained");
    check(module->release() == 2 && watchedDeletes == 0, "release retains live module");
    check(module->release() == 1 && watchedDeletes == 0, "second release retains initial owner");
    check(module->release() == 0 && watchedDeletes == 1, "final release frees allocation once");
    watchedAllocation = nullptr;
}

static void frontier(const char* mode, const char* expected)
{
    char executable[MAX_PATH];
    GetModuleFileNameA(nullptr, executable, MAX_PATH);
    std::string command = std::string("\"") + executable + "\" " + mode;
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0)) { check(false, "create diagnostic pipe"); return; }
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION process{};
    const BOOL started = CreateProcessA(nullptr, &command[0], nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(writePipe);
    check(started != FALSE, "start frontier child");
    if (!started) { CloseHandle(readPipe); return; }
    const DWORD wait = WaitForSingleObject(process.hProcess, 10000);
    check(wait == WAIT_OBJECT_0, "frontier child terminates promptly");
    if (wait != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 99);
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    std::string output;
    char buffer[512]; DWORD count;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &count, nullptr) && count) output.append(buffer, count);
    CloseHandle(readPipe); CloseHandle(process.hThread); CloseHandle(process.hProcess);
    check(exitCode != 0 && exitCode != STILL_ACTIVE, "frontier cannot return success");
    check(output.find(expected) == 0, "first output is qualified flushed frontier diagnostic");
    check(output.find("unexpected return") == std::string::npos, "frontier never returns");
}

static void bindings()
{
    const auto& list = hudConsoleObjects();
    check(list.size() == 228, "all 228 retail bindings present");
    std::string serialized;
    std::set<std::string> namespaces;
    std::set<const ConsoleObject*> identities;
    unsigned dynamic = 0, properties = 0;
    for (ConsoleObject* object : list) {
        namespaces.insert(object->getObjectName());
        identities.insert(object);
        check(object->getObject() == nullptr, "all HUD getObject hooks are actual null base hook");
        check(object->getAccess() == 2 && object->getCategory() == 0, "complete default access/category");
        check(!object->hasReturnValue(), "PE A0 zero even for advertised returns");
        check(!object->hasRange() && !object->hasBundledType(), "actual no-range and no-bundled hooks");
        check(object->getRange() == std::make_pair<const void*, const void*>(nullptr, nullptr), "native null range pair");
        check(object->getRangeAsString() == std::make_pair(std::string(), std::string()), "native empty range strings");
        check(object->getDeltaAsString() == "No Delta" && object->getTypeDescription().empty(), "PE textual default hooks");
        serialized += std::string(object->getObjectName()) + "|" + object->getFunctionName()
            + "|" + std::to_string(object->getType()) + "|" + std::to_string(object->getAccess())
            + "|" + std::to_string(object->getCategory()) + "|" + std::to_string(object->getMinNoArgs())
            + "|" + std::to_string(object->getMaxNoArgs()) + "|" + object->getReturnType()
            + "|" + std::to_string(object->hasReturnValue()) + "|" + std::to_string(object->isDynamic());
        for (int i = 0; i < object->getMaxNoArgs(); ++i) serialized += std::string("|") + object->getArgType(i);
        serialized += '\n';
        dynamic += object->isDynamic();
        properties += object->isSetGet();
    }
    std::uint32_t hash = 2166136261u;
    for (unsigned char c : serialized) hash = (hash ^ c) * 16777619u;
    // Independent retained constructor/default extraction, metadata_golden.txt.
    check(hash == 0x5f3ca545u, "all ordered metadata matches complete PE constructor golden");
    check(identities.size() == 228 && namespaces.size() == 21, "distinct native objects in all 21 populated namespaces");
    check(properties == 1 && dynamic == 142, "actual property and dynamic flag counts");
    ConsoleObject* refresh = list.front();
    check(!refresh->isActive(), "manager availability observes null singleton");
    // Presence token is only compared to null by PE76CCAF; never dereferenced.
    int presence;
    hudManager = reinterpret_cast<BF2HudManager*>(&presence);
    check(refresh->isActive(), "manager availability observes present singleton");
    HudConsoleModule hud;
    ProfileConsoleModule profile;
    const auto before = mainConsoleObjects->getConsoleObjects().size();
    check(profile.init() && profileConsoleOwner, "real Profile init constructs and publishes owner");
    check(mainConsoleObjects->getConsoleObjects().size() == before + 17, "real Profile registry has 17 bindings");
    const auto profileFirst = mainConsoleObjects->getConsoleObjects().front();
    check(hud.init(), "real HUD registration returns shipped true");
    check(mainConsoleObjects->getConsoleObjects().size() == before + 245, "real registry contains both complete lists");
    auto registered = mainConsoleObjects->getConsoleObjects().begin();
    std::advance(registered, before + 17);
    for (ConsoleObject* object : list) check(*registered++ == object, "real registry preserves every HUD pointer/order");
    check(mainConsoleObjects->getConsoleObject("HUDMANAGER", "REFRESH") == refresh, "case-insensitive real registry lookup and active hook");
    hudManager = nullptr;
    check(!mainConsoleObjects->getConsoleObject("HudManager", "refresh"), "real registry skips inactive HUD binding");
    check(hud.close() && mainConsoleObjects->getConsoleObjects().size() == before + 17,
        "HUD close unregisters full list without Profile loss");
    check(mainConsoleObjects->getConsoleObjects().front() == profileFirst, "other module insertion order survives close");
    check(hud.init() && mainConsoleObjects->getConsoleObjects().size() == before + 245, "HUD list supports re-registration");
    check(hud.close() && profile.close() && !profileConsoleOwner, "both real close methods complete and release profile");
    check(mainConsoleObjects->getConsoleObjects().size() == before, "registry restored after full module lifecycle");
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc == 2) {
        auto& list = hudConsoleObjects();
        auto iter = list.begin();
        std::advance(iter, std::atoi(argv[1]));
        ConsoleObjectBaseImpl* object = static_cast<ConsoleObjectBaseImpl*>(*iter);
        if (std::strstr(argv[1], "-method")) object->executeObjectMethod();
        else if (std::strstr(argv[1], "-convert")) object->setArgFromString(1, "1");
        else if (std::strstr(argv[1], "-format")) object->getReturnValueAsString();
        else object->execute(nullptr, 0);
        std::puts("unexpected return");
        return 0;
    }
    lifetime<HudConsoleModule>();
    lifetime<ProfileConsoleModule>();
    bindings();
    frontier("0", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::execute: hudManager.refresh");
    frontier("16", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::execute: hudManager.setSpottedAngle");
    // The index has to name a row that is still a stop, and the list shrinks as families
    // land: 107 createCompassNode went with family C, 116 createButtonNode with family M.
    // 93 setPictureNodeAlphaMask is deliberately chosen instead - no shipped HUD script
    // calls it, so it is the last kind of row any family will convert. If it ever does
    // become a real invoker, repoint this at another uncalled row (127, 134, 145, 146).
    frontier("93", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::execute: hudBuilder.setPictureNodeAlphaMask");
    frontier("0-method", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::executeObjectMethod:");
    frontier("16-convert", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::setArgFromString:");
    frontier("0-format", "[FRONTIER] dice::hfe::hud::HudConsoleBinding::getReturnValueAsString:");
    std::printf("Client console modules: %u/%u checks passed\n", checks-failures, checks);
    return failures ? 1 : 0;
}
