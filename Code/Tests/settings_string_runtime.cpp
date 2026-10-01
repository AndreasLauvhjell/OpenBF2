// [TEMP] [CHOSEN, not recovered] Bounded console/repository/event collaborators.
#include "../BF2/Core/ConsoleObjectMacro.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/IO/SettingsManager.h"
#include <cstdio>
#include <cstring>
using namespace dice::hfe;
namespace dice { namespace hfe { namespace io { ConsoleObjectList& fileManagerConsoleObjects(); } } }
namespace {
int checks, failures;
void check(bool ok, const char* label)
{ ++checks; if (!ok) ++failures; std::printf("%s %s\n", ok ? "PASS" : "FAIL", label); }
struct Repository : SettingsRepostitory
{
    int writes = 0;
    std::string* mutate = 0;
    const std::string& stringGet(const std::string& key, const std::string& fallback) override
    {
        if (mutate) { mutate[0] = "changed key"; mutate[1] = "changed value"; mutate = 0; }
        return SettingsRepostitory::stringGet(key, fallback);
    }
    void stringSet(const std::string& key, const std::string& value) override
    { ++writes; SettingsRepostitory::stringSet(key, value); }
};
struct Events : EventManager
{
    int posts = 0;
    std::string key, seenValue;
    unsigned int postEvent(EventCategory category, unsigned id, void* data, float delay) override
    {
        ++posts; key = *static_cast<std::string*>(data);
        seenValue = settings->stringGet(key, std::string("UnSet"));
        check(int(category) == 5 && id == 1 && delay == 0.0f, "retail event category/id/delay");
        return 0;
    }
};
}
int main()
{
    ConsoleObject* binding = 0;
    for (ConsoleObject* o : io::fileManagerConsoleObjects())
        if (!std::strcmp(o->getFunctionName(), "stringSet")) binding = o;
    check(binding != 0, "stringSet registered");
    if (!binding) return 1;
    check(!std::strcmp(binding->getObjectName(), "SettingsManager") && !binding->isDynamic(),
          "fixed retail object name");
    check(binding->getAccess() == 1 && binding->getCategory() == 0 && binding->getType() == 0,
          "method access/category/type");
    check(binding->getMinNoArgs() == 2 && binding->getMaxNoArgs() == 2 && binding->isActive()
          && !binding->hasRange(), "two arguments, always active, no range");
    check(!std::strcmp(binding->getArgType(0), "std::string")
          && !std::strcmp(binding->getArgType(1), "std::string")
          && !std::strcmp(binding->getReturnType(), "bool"), "retail argument/return types");
    Repository repository;
    Events events;
    auto* oldSettings = settings;
    auto* oldEvents = eventManager;
    settings = &repository; eventManager = &events;
    std::string args[] = {"SoakFixtureKey", "first value with spaces"};
    check(binding->execute(args, 2) == "0" && repository.writes == 0 && events.posts == 0,
          "unregistered name rejected without write or event");
    io::settingsManager->registerUserVar(args[0], false, io::VariableSet(0));
    check(binding->execute(args, 2) == "1" && binding->hasReturnValue(), "registered write succeeds");
    check(repository.writes == 1 && events.posts == 1 && events.key == args[0]
          && events.seenValue == args[1], "event sees committed raw string");
    check(binding->execute(args, 2) == "1" && repository.writes == 2 && events.posts == 1,
          "unchanged value still writes but does not post");
    args[1] = "";
    check(binding->execute(args, 2) == "1" && events.seenValue.empty() && events.posts == 2,
          "empty string is retained and notified");
    args[1] = "snapshot";
    repository.mutate = args;
    check(binding->execute(args, 2) == "1" && events.key == "SoakFixtureKey"
          && events.seenValue == "snapshot", "both arguments copied before repository callbacks");
    eventManager = 0;
    args[0] = "SoakFixtureKey"; args[1] = "without event manager";
    check(binding->execute(args, 2) == "1" && events.posts == 3, "retail absent-event-manager guard");
    args[0] = "SoakFixtureFallback"; args[1] = "UnSet";
    io::settingsManager->registerUserVar(args[0], false, io::VariableSet(0));
    eventManager = &events;
    check(binding->execute(args, 2) == "1" && repository.stringExists(args[0]) && events.posts == 3,
          "absent value equal to UnSet fallback writes without posting");
    const int writes = repository.writes;
    binding->execute(args, 1);
    check(repository.writes == writes, "wrong argument count does not execute");
    settings = oldSettings; eventManager = oldEvents;
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
