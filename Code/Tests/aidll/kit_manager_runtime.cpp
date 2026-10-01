// [TEMP] Genuine native collaborator lifetimes; no production header rewriting or raw objects.
// KitTemplateManager.cpp and KitTemplate.cpp compile unchanged in separate translation units.
#include "kit_manager_runtime.h"
#include <windows.h>
#include <crtdbg.h>
#include <algorithm>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static int checks;
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
[[noreturn]] void unexpected(const char* method)
{
    std::fprintf(stderr, "Unexpected collaborator %s\n", method); std::abort();
}

struct Watched { const void* pointer; int id; int deletes; };
static Watched watched[60];
static int watchCount;
static int deleted[60];
static int deletedCount;
static bool requireNullInstance;
static void watch(const void* pointer, int id)
{
    require(pointer != nullptr && watchCount < 60, "valid bounded allocation watch");
    watched[watchCount++] = {pointer, id, 0};
}
static void observeDelete(void* pointer)
{
    if (!pointer) return;
    for (int i = 0; i < watchCount; ++i)
        if (watched[i].pointer == pointer && watched[i].deletes == 0)
        {
            if (requireNullInstance && KitTemplateManager::m_instance != nullptr)
                std::abort();
            ++watched[i].deletes;
            deleted[deletedCount++] = watched[i].id;
            break;
        }
}
void operator delete(void* pointer) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete[](void* pointer) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete(void* pointer, size_t) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete[](void* pointer, size_t) noexcept { observeDelete(pointer); std::free(pointer); }
static void watchKit(KitTemplate* kit, int id)
{
    watch(kit->getStrategicStrengths(), id * 10 + 1);
    watch(kit->getBattleStrengths(), id * 10 + 2);
    watch(kit, id * 10 + 3);
}

static void* registered;
static int registrations, unregisters, refreshes;
static std::string registryTrace;
static std::string workingFile;
static std::string expectedName;
static int expectedType;
static int fileCalls;
static KitTemplateManager* observedManager;
static std::string redirectActive;
namespace dice { namespace hfe {
ClassManager* classManager = nullptr;
ClassManager::ClassManager() {}
ClassManager::~ClassManager() {}
void ClassManager::registerSimpleSingleton(const std::string& name, void* value)
{
    require(name == "IKitTemplateManager", "exact registration name");
    require(CommonSingletonServer::m_iKitTemplateManager == nullptr, "first-wins guard before registration");
    registered = value; ++registrations; registryTrace += "register,";
}
void ClassManager::unregisterSimpleSingleton(const std::string& name)
{
    require(name == "IKitTemplateManager", "exact unregister name");
    require(CommonSingletonServer::m_iKitTemplateManager == registered, "matching roster before unregister");
    require(KitTemplateManager::m_instance == nullptr, "concrete singleton clear before base unregister");
    registered = nullptr; ++unregisters; registryTrace += "unregister,";
}
namespace ai {
void* CommonSingletonServer::m_iKitTemplateManager = nullptr;
void BFSingletonServer::updateStaticMethods()
{
    ++refreshes; registryTrace += "refresh,";
    CommonSingletonServer::m_iKitTemplateManager = registered;
}
}
namespace io {
OldConsole* mainConsole = nullptr;
OldConsole::Level::Level() {}
OldConsole::OldConsole() {}
OldConsole::~OldConsole() {}
const std::string& OldConsole::getWorkingFile() const
{
    ++fileCalls;
    require(observedManager != nullptr, "expected callback manager");
    require(observedManager->getActive() == expectedName, "active publication before file callback");
    KitTemplate* kit = observedManager->getTemplate(expectedName);
    require(kit && kit == observedManager->getActiveTemplate(), "name map publication before file callback");
    require(observedManager->getTemplate(static_cast<KitType>(expectedType)) == kit,
            "type vector publication before file callback");
    if (!redirectActive.empty()) observedManager->setActive(redirectActive);
    return workingFile;
}
} } }
#include "kit_collaborators.inl"

static KitTemplate* create(KitTemplateManager& manager, const std::string& name, int type)
{
    observedManager = &manager; expectedName = name; expectedType = type;
    require(manager.create(name, static_cast<KitType>(type)) == "KitTemplate '" + name + "' successfully created",
            "exact success result");
    return manager.getTemplate(name);
}

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "x86 only");
    static_assert(std::is_same<decltype(std::declval<IKitTemplateManager&>().reset()), bool>::value, "bool reset ABI");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode argument");
    const std::string mode(argv[1]);
    ClassManager registry;
    io::OldConsole console;
    classManager = &registry; io::mainConsole = &console;
    if (mode == "lifecycle")
    {
        KitTemplateManager* first = new KitTemplateManager;
        require(registered == first && KitTemplateManager::m_instance == first, "first manager registered and published");
        require(registryTrace == "register,refresh,", "registration then refresh");
        KitTemplateManager* second = new KitTemplateManager;
        require(registrations == 1 && refreshes == 1 && registered == first, "base first-wins with second manager");
        require(KitTemplateManager::m_instance == second, "concrete singleton last-wins");
        require(second->getAllTemplates().size() == 13 && second->getActive().empty() && !second->getActiveTemplate(), "initial vector and active iterator");
        for (auto kit : second->getAllTemplates()) require(kit == nullptr, "all thirteen initially null");
        delete first;
        require(KitTemplateManager::m_instance == nullptr && registered == nullptr, "older manager destruction clears concrete singleton unconditionally");
        require(registryTrace == "register,refresh,unregister,refresh,", "matching base unregister then refresh");
        delete second;
        require(unregisters == 1 && refreshes == 2, "nonmatching base destruction does not unregister");
    }
    else
    {
        {
            KitTemplateManager manager;
            const auto& all = manager.getAllTemplates();
            if (mode == "population")
            {
                KitTemplate* first = create(manager, "Alpha", 1); watchKit(first, 1);
                const int before = fileCalls;
                require(manager.create("Refused", static_cast<KitType>(13)) == "Error: Template 'Refused' is of type 'KTNoKit'", "exact no-kit refusal");
                require(manager.getActiveTemplate() == first && manager.getActive() == "Alpha", "no-kit refusal retains active selection");
                require(manager.create("Alpha", static_cast<KitType>(2)) == "Error: Template 'Alpha' already exists", "exact duplicate refusal");
                require(!manager.getActiveTemplate() && manager.getActive().empty() && fileCalls == before, "duplicate clears active and no callback on refusals");
                KitTemplate* lower = create(manager, "alpha", 1); watchKit(lower, 2);
                require(first != lower && manager.getTemplate(std::string("Alpha")) == first && all[1] == lower, "case-sensitive names and same-type replacement");
                require(manager.getTemplate(static_cast<KitType>(13)) == nullptr, "no-kit getter guard");
                manager.setActive("Alpha"); require(manager.getActiveTemplate() == first, "select first same-type template");
                manager.setActive("missing"); require(!manager.getActiveTemplate() && manager.getActive().empty(), "missing selection becomes end");
                std::list<KitTemplate*> used; manager.getAllUsedTemplates(used);
                require(used.empty(), "empty working file is not tracked");
                requireNullInstance = true;
            }
            else if (mode == "metadata" || mode == "mutation")
            {
                workingFile = "zeta.con"; KitTemplate* z = create(manager, "z", 3);
                workingFile = "alpha.con"; KitTemplate* a = create(manager, "a", 4);
                if (mode == "mutation")
                {
                    redirectActive = "z"; workingFile = "changed.con";
                    KitTemplate* b = create(manager, "b", 5);
                    require(manager.getActiveTemplate() == z && manager.getTemplate(std::string("b")) == b, "callback mutates active without losing created template");
                    require(manager.getWorkingFileAndPath(z) == "changed.con", "file association rereads active after callback");
                    std::list<KitTemplate*> used; manager.getAllUsedTemplates(used);
                    require(used.size() == 2 && std::find(used.begin(), used.end(), b) == used.end(), "retained created pointer is not file key after mutation");
                }
                else
                {
                    workingFile = "zeta.con"; KitTemplate* b = create(manager, "b", 5);
                    std::list<KitTemplate*> used(1, nullptr); manager.getAllUsedTemplates(used);
                    std::vector<KitTemplate*> expected = {z, a, b}; std::sort(expected.begin(), expected.end(), std::less<KitTemplate*>());
                    require(used.front() == nullptr && used.size() == 4, "used-template append preserves prefix");
                    used.pop_front(); require(std::equal(used.begin(), used.end(), expected.begin()), "used-template map pointer order");
                    std::list<std::string> files(1, "prefix"); manager.getAllUsedTemplateFiles(files);
                    require(files == std::list<std::string>({"prefix", "alpha.con", "zeta.con"}), "unique lexical file append");
                    require(manager.getWorkingFileAndPath(a) == "alpha.con", "existing file copied by value");
                }
            }
            else
            {
                require(mode == "reset", "known mode");
                workingFile = "kits.con";
                KitTemplate* z = create(manager, "z", 3); watchKit(z, 2);
                KitTemplate* a = create(manager, "a", 3); watchKit(a, 1);
                IKitTemplate* saved = all[3];
                IKitTemplateManager& dispatch = manager;
                require(dispatch.reset(), "virtual reset returns true");
                require(deletedCount == 6, "reset deletes both templates and both owned arrays");
                for (int i = 0; i < 6; ++i) require(deleted[i] == (i < 3 ? 11 + i : 21 + i - 3), "name order and strategic-before-battle destruction");
                require(all.size() == 13 && all[3] == saved, "reset leaves borrowed type vector entries unchanged");
                require(!manager.getTemplate(std::string("a")) && !manager.getActiveTemplate() && manager.getActive().empty(), "reset clears names and leaves active end");
                std::list<KitTemplate*> used; manager.getAllUsedTemplates(used); require(used.empty(), "reset clears working-file map");
                require(KitTemplateManager::m_instance == &manager, "reset does not clear concrete singleton");
                require(dispatch.reset() && deletedCount == 6, "repeat reset neither double deletes nor changes vector");
            }
        }
        for (int i = 0; i < watchCount; ++i) require(watched[i].deletes == 1, "each observed allocation deleted exactly once");
        require(registered == nullptr && KitTemplateManager::m_instance == nullptr, "final registry and singleton cleared");
    }
    std::printf("PASS %s %d checks\n", mode.c_str(), checks);
}
