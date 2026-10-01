// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-CONSOLE-INVOKER-FIXTURE]
// Tests real console registration and invocation with typed HUD callbacks.
// String observers mutate by-value arguments to detect aliasing; wider HUD lifecycle dependencies are not exercised.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <list>
#include <vector>
#include <cstdint>
#include <climits>
#include <new>
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/HudCommander.h"
#include "../BF2/Menu/HudRadio.h"
#undef private
#include "../BF2/Menu/ClientConsoleModules.h"
#include "../BF2/Menu/HudConsoleBindings.h"
#include "../BF2/Core/ConsoleObjectBase.h"
#include "../BF2/Core/ConsoleObjects.h"
#include "../BF2/Menu/Bf2HudBuilder.h"

namespace dice { namespace hfe { hud::BF2HudManager* hudManager = nullptr; } }
using namespace dice::hfe;
using namespace dice::hfe::hud;

static unsigned checks, failures, calls;
static int observedMethod = -1;
static bool callbackResult, separateArgument, callbackSawReturnFlag;
static std::string observedArgument;
static const std::string* sourceArgument;
static ConsoleObject* currentBinding;
static std::vector<std::string> baseOrder;
static int observedFirst, observedSecond;
static Bf2HudBuilder* activeBuilder;
static Bf2HudBuilder* observedBuilder;
static std::vector<std::string> splitOrder;
static std::string observedParent, observedName;
static const std::string* splitSources[2];
static bool observeSplitGetter, splitLookupFlag, splitTargetFlag, splitCopied;
static unsigned splitCalls;

// Family R: the five radio setStringMap rows.
static unsigned radioCalls;
static int observedRadio = -1;
static std::string radioKey, radioLocalized, radioCommand;
static const std::string* radioSources[3];
static bool radioCopied, radioSawReturnFlag, radioOwnersPresent;
static HudRadio* radioOwners[5];

// CommanderMenu rows 183..188. The wrapper fixture supplies field-free target
// observers for the five named actions and a raw owner only for the duration
// field; the production generated invokers remain the code under test.
static HudCommander* commanderOwner;
static bool commanderOwnerPresent;
static unsigned commanderCalls, commanderGetterCalls;
static int observedCommanderMethod = -1;
static HudCommander* observedCommander;
static bool commanderSawReturnFlag, commanderGetterSawReturnFlag;
static std::string commanderMessage;
static const std::string* commanderMessageSource;
static bool commanderMessageCopied;
static TargetChannel commanderChannel = TCInvalid;

static void observeCommander(HudCommander* owner, int method)
{
    ++commanderCalls;
    observedCommander = owner;
    observedCommanderMethod = method;
    commanderSawReturnFlag = currentBinding->hasReturnValue();
}

// [TEMP] [CHOSEN, not recovered] Wrapper-only target observer. The command
// links this exact nonvirtual method instead of Bf2HudBuilder.cpp. Its builder
// pointer is an explicitly unconstructed identity token and is never
// dereferenced; no builder field, constructor or virtual method is exercised.
namespace dice { namespace hfe { namespace hud {
bool Bf2HudBuilder::createSplitNode(std::string parent, std::string name)
{
    ++splitCalls;
    splitOrder.push_back("target");
    observedBuilder = this;
    observedParent = parent;
    observedName = name;
    splitCopied = &parent != splitSources[0] && &name != splitSources[1];
    splitTargetFlag = currentBinding->hasReturnValue();
    parent.assign(509, 'p');
    name.assign(1021, 'n');
    return callbackResult;
}
void HudCommander::singleClick() { observeCommander(this, 0); }
void HudCommander::rightClick() { observeCommander(this, 1); }
void HudCommander::doubleClick() { observeCommander(this, 2); }
void HudCommander::deselect() { observeCommander(this, 3); }
void HudCommander::sendRadioMessage(std::string message, TargetChannel channel)
{
    observeCommander(this, 4);
    commanderMessage = message;
    commanderMessageCopied = &message != commanderMessageSource;
    commanderChannel = channel;
    message.assign(1025, 'm');
}
} } }

static void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL %s\n", label); }
}

// [TEMP] Hook observer only: execute itself is the real shared base body.
struct BaseDispatchObserver : ConsoleObjectBaseImpl {
    int converted[3] = {};
    int result = 0;
    bool rangeArgument = false;
    std::string* expectedRaw = nullptr;
    bool methodSawRaw = false;
    int methodCount = 0;
    BaseDispatchObserver()
    {
        m_minNoArgs = m_maxNoArgs = 1; // The shared base must not clamp argc.
        for (int i = 0; i < 10; ++i) { m_argNeedsConversion[i] = 0; m_args[i] = nullptr; }
        m_argNeedsConversion[0] = m_argNeedsConversion[2] = -1;
    }
    void setArgFromString(int index, const std::string& value) override
    {
        baseOrder.push_back("convert" + std::to_string(index));
        converted[index - 1] = std::atoi(value.c_str());
    }
    void checkObjectRange(bool clamp) override
    {
        baseOrder.push_back("range");
        rangeArgument = clamp;
        if (clamp) { converted[0] += 10; converted[2] += 10; }
    }
    void executeObjectMethod() override
    {
        baseOrder.push_back("method");
        methodCount = m_argCount;
        methodSawRaw = m_args[1] == expectedRaw;
        result = converted[0] + converted[2];
    }
    std::string getReturnValueAsString() const override
    {
        baseOrder.push_back("format");
        return std::to_string(result);
    }
};

static void sharedBaseDispatch()
{
    BaseDispatchObserver observer;
    std::string args[] = {"12", "raw text with spaces", "30"};
    observer.expectedRaw = &args[1];
    ConsoleObject* api = &observer;
    const std::string result = api->execute(args, 3);
    check(baseOrder == std::vector<std::string>{"convert1", "convert3", "range", "method", "format"},
        "real base calls one-based conversions, range, method and formatter in order");
    check(observer.methodCount == 3 && observer.getMaxNoArgs() == 1,
        "real base preserves supplied argument count without metadata clamp");
    check(observer.methodSawRaw, "real base keeps original string pointer for no-conversion argument");
    check(observer.rangeArgument && result == "62", "real base passes true to range hook before target result");
}

// [TEMP] [CHOSEN, not recovered] Family R observer. The real HudRadio
// constructor and destructor run; only the +18 addEntry slot is observed, so
// the map members it would fill stay empty and no radio state is modelled.
struct ObserverRadio : HudRadio {
    int id = -1;
    void addEntry(std::string key, std::string localizedId, std::string command) override
    {
        ++radioCalls;
        observedRadio = id;
        radioKey = key;
        radioLocalized = localizedId;
        radioCommand = command;
        radioCopied = &key != radioSources[0] && &localizedId != radioSources[1]
            && &command != radioSources[2];
        radioSawReturnFlag = currentBinding->hasReturnValue();
        key.assign(257, 'k');
        localizedId.assign(259, 'l');
        command.assign(263, 'c');
    }
};

struct ObserverHud : BF2HudManager {
    HudCommander* getCommander() override
    {
        ++commanderGetterCalls;
        commanderGetterSawReturnFlag = currentBinding && currentBinding->hasReturnValue();
        return commanderOwnerPresent ? commanderOwner : nullptr;
    }
    // Family R. Each row's own getter, so a row wired to the wrong slot shows up
    // as the wrong observer id rather than as a silent success.
    HudRadio* radioOwner(int index) { return radioOwnersPresent ? radioOwners[index] : nullptr; }
    HudRadio* getSquadLeaderRadio() override { return radioOwner(0); }
    HudRadio* getSquadRadio() override       { return radioOwner(1); }
    HudRadio* getCommanderRadio() override   { return radioOwner(2); }
    HudRadio* getInfantryRadio() override    { return radioOwner(3); }
    HudRadio* getVehicleRadio() override     { return radioOwner(4); }
    Bf2HudBuilder* getBuilder() override
    {
        if (observeSplitGetter) {
            splitOrder.push_back("builder");
            splitLookupFlag = currentBinding->hasReturnValue();
        }
        return activeBuilder;
    }
    bool observe(int method, std::string& value)
    {
        ++calls;
        observedMethod = method;
        observedArgument = value;
        separateArgument = &value != sourceArgument;
        callbackSawReturnFlag = currentBinding->hasReturnValue();
        value.assign(513, '!');
        return callbackResult;
    }
    bool setMouseTextureEmpty(std::string value) override { return observe(0, value); }
    bool setMouseTextureFull(std::string value) override { return observe(1, value); }
    bool setMouseTextureArtillery(std::string value) override { return observe(2, value); }
    bool setMouseTextureUAV(std::string value) override { return observe(3, value); }
    bool setMouseTextureSupply(std::string value) override { return observe(4, value); }
    bool setMouseTextureCancel(std::string value) override { return observe(5, value); }
    bool observeInts(int method, int first, int second)
    {
        ++calls;
        observedMethod = method;
        observedFirst = first;
        observedSecond = second;
        callbackSawReturnFlag = currentBinding->hasReturnValue();
        return callbackResult;
    }
    bool setPointerSize(int first, int second) override { return observeInts(6, first, second); }
    bool setCommMouseSensitivity(int first) override { return observeInts(7, first, 0); }
    bool setPointerMouseSensitivity(int first) override { return observeInts(8, first, 0); }
    bool setCommSize(int first, int second) override { return observeInts(9, first, second); }
    bool setCommPos(int first, int second) override { return observeInts(10, first, second); }
    bool setCommMousePos(int first, int second) override { return observeInts(11, first, second); }
    bool setSpottedMenuSize(int first, int second) override { return observeInts(12, first, second); }
    bool setSpottedMenuPos(int first, int second) override { return observeInts(13, first, second); }
    bool setSpottedMousePos(int first, int second) override { return observeInts(14, first, second); }
    void setMaximumNrOfCPs(int first) override { observeInts(15, first, 0); }
};

// Constructor-only ownership cleanup follows the existing HUD component
// fixtures. It does not invoke or replace the full production HUD destructor.
static void destroyConstructorOnly(ObserverHud* hud)
{
    delete hud->m_field2dc;
    using S = std::string; using W = std::wstring;
    using L = std::list<int>; using SL = std::list<std::string>;
    using V = std::vector<HudPendingRequest>;
    hud->m_field33c.~SL(); hud->m_field32c.~V(); hud->m_field304.~S();
    hud->m_field2e0.~SL(); hud->m_field0e8.~L(); hud->m_field0c0.~S();
    hud->m_field08c.~S(); hud->m_field060.~W(); hud->m_field038.~W(); hud->m_field010.~W();
    ::operator delete(hud);
}

static void exercise(ConsoleObject* binding, int method)
{
    currentBinding = binding;
    auto* hooks = static_cast<ConsoleObjectBaseImpl*>(binding);
    check(binding->getMinNoArgs() == 1 && binding->getMaxNoArgs() == 1, "retail one-argument metadata");
    check(std::strcmp(binding->getArgType(0), "std::string") == 0, "retail string argument metadata");
    check(std::strcmp(binding->getReturnType(), "bool") == 0 && binding->isDynamic(), "retail bool dynamic metadata");
    check(!binding->hasReturnValue() && hooks->getReturnValueAsString().empty(), "fresh binding has no formatted result");
    unsigned previousCalls = calls;
    check(binding->execute(nullptr, 0).empty(), "initial wrong arity has no result");
    check(calls == previousCalls && !binding->hasReturnValue(), "initial wrong arity skips callback and flag");

    const std::string inputs[] = {std::string(), std::string("Hud/Mouse/Short.dds"),
        std::string("texture\0suffix", 14), std::string(4096, 'x') + "/long.dds"};
    for (unsigned i = 0; i < 4; ++i) {
        std::string argument = inputs[i];
        sourceArgument = &argument;
        callbackResult = (i % 2) == 0;
        previousCalls = calls;
        const std::string result = binding->execute(&argument, 1);
        check(calls == previousCalls + 1 && observedMethod == method, "registered invoker selects exact HUD virtual once");
        check(observedArgument == inputs[i], "complete string payload reaches callback");
        check(separateArgument && argument == inputs[i], "by-value payload copy isolates callback mutation");
        check(callbackSawReturnFlag, "return flag is set before dispatching HUD callback");
        check(result == (callbackResult ? "1" : "0"), "actual bool result formats as one or zero");
        check(binding->hasReturnValue() && hooks->getReturnValueAsString() == result, "successful invocation caches flag and result");

        previousCalls = calls;
        std::string excess[] = {"ignored-first", "ignored-second"};
        check(binding->execute(excess, 2) == result, "excess arguments preserve cached result");
        check(binding->execute(nullptr, 0) == result, "missing argument preserves cached result");
        check(binding->execute(nullptr, -1) == result, "negative argument count preserves cached result");
        check(calls == previousCalls && binding->hasReturnValue(), "wrong argument counts skip HUD and preserve result flag");
    }
}

static void conversionHooks(ConsoleObject* const* bindings)
{
    // Normal string execute bypasses conversion. These direct hook calls cover
    // the generated one-based conversion path and its per-command static text.
    for (int i = 0; i < 6; ++i) {
        currentBinding = bindings[i];
        if (!currentBinding) return;
        std::string seed("prime-arity");
        sourceArgument = &seed;
        callbackResult = true;
        currentBinding->execute(&seed, 1); // establishes the real m_argCount=1
        std::string input = " \tconverted-" + std::to_string(i) + " second-token";
        static_cast<ConsoleObjectBaseImpl*>(currentBinding)->setArgFromString(1, input);
        input.assign(1024, 'z');
    }
    sourceArgument = nullptr;
    for (int i = 0; i < 6; ++i) {
        currentBinding = bindings[i];
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        const std::string expected = "converted-" + std::to_string(i);
        unsigned previousCalls = calls;
        callbackResult = (i % 2) != 0;
        hooks->executeObjectMethod();
        check(calls == previousCalls + 1 && observedMethod == i, "direct generated method hook selects exact virtual");
        check(observedArgument == expected, "conversion tokenizes and retains a separate static string for each command");
        check(callbackSawReturnFlag && hooks->getReturnValueAsString() == (callbackResult ? "1" : "0"),
            "direct hook preserves return-flag and boolean formatting path");
        for (int index : {-1, 0, 2}) hooks->setArgFromString(index, "wrong-index");
        hooks->executeObjectMethod();
        check(observedArgument == expected, "conversion indices other than one leave stored argument unchanged");
    }
    currentBinding = bindings[0];
    auto* first = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
    first->setArgFromString(1, "\"quoted words\"");
    first->executeObjectMethod();
    check(observedArgument == "\"quoted", "conversion uses stream extraction without console quote parsing");
    first->setArgFromString(1, "");
    first->executeObjectMethod();
    check(observedArgument == "\"quoted", "empty conversion input preserves preceding token on sentry failure");
    first->setArgFromString(1, " \t\r\n");
    first->executeObjectMethod();
    check(observedArgument == "\"quoted", "whitespace-only conversion input preserves preceding token");
    currentBinding = bindings[1];
    static_cast<ConsoleObjectBaseImpl*>(currentBinding)->executeObjectMethod();
    check(observedArgument == "converted-1", "updating one command static leaves another command static intact");
}

static void numericBindings()
{
    const char* names[] = {"setPointerSize", "setCommMouseSensitivity", "setPointerMouseSensitivity",
        "setCommSize", "setCommPos", "setCommMousePos", "setSpottedMenuSize", "setSpottedMenuPos",
        "setSpottedMousePos", "setMaximumNrOfCPs"};
    const int arities[] = {2, 1, 1, 2, 2, 2, 2, 2, 2, 1};
    ConsoleObject* resolved[10] = {};
    for (int i = 0; i < 10; ++i) {
        currentBinding = resolved[i] = mainConsoleObjects->getConsoleObject("HUDMANAGER", names[i]);
        check(currentBinding != nullptr, "real registry resolves numeric HUD command");
        if (!currentBinding) continue;
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        const int arity = arities[i];
        const bool returnsBool = i != 9;
        check(currentBinding->getMinNoArgs() == arity && currentBinding->getMaxNoArgs() == arity,
            "numeric binding preserves exact retail arity");
        check(std::strcmp(currentBinding->getReturnType(), returnsBool ? "bool" : "void") == 0
            && currentBinding->isDynamic() == returnsBool, "numeric return and dynamic metadata match retail");
        check(!currentBinding->hasReturnValue() && hooks->getReturnValueAsString().empty(), "numeric fresh return flag and format");
        unsigned previousCalls = calls;
        check(currentBinding->execute(nullptr, 0).empty() && calls == previousCalls, "numeric initial wrong arity skips callback");
        for (int variant = 0; variant < 2; ++variant) {
            std::string args[3] = {variant ? "-2147483648" : " \t-123 trailing",
                variant ? "2147483647" : " +456 suffix", "789"};
            const int first = variant ? INT_MIN : -123;
            const int second = arity == 2 ? (variant ? INT_MAX : 456) : 0;
            callbackResult = variant == 0;
            previousCalls = calls;
            const std::string result = currentBinding->execute(args, arity);
            check(calls == previousCalls + 1 && observedMethod == i + 6, "numeric invoker selects exact HUD virtual once");
            check(observedFirst == first && observedSecond == second, "signed integer conversion preserves values and pair order");
            check(callbackSawReturnFlag == returnsBool, "numeric return flag has retail value before callback");
            check(result == (returnsBool ? (callbackResult ? "1" : "0") : ""), "numeric boolean or void formatter matches target result");
            check(currentBinding->hasReturnValue() == returnsBool && hooks->getReturnValueAsString() == result,
                "numeric result flag and cached formatting persist");
            previousCalls = calls;
            check(currentBinding->execute(args, arity + 1) == result, "numeric excess arity preserves cached return");
            if (arity == 2) check(currentBinding->execute(args, 1) == result, "numeric pair with one argument preserves cached return");
            check(currentBinding->execute(nullptr, 0) == result, "numeric missing arity preserves cached return");
            check(currentBinding->execute(nullptr, -1) == result, "numeric negative arity preserves cached return");
            check(calls == previousCalls, "numeric wrong arities never enter HUD callback");
        }
        std::string retained[] = {std::to_string(-600 - i), std::to_string(800 + i)};
        currentBinding->execute(retained, arity);
    }
    for (int i = 0; i < 10; ++i) {
        currentBinding = resolved[i];
        if (!currentBinding) continue;
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        for (int index : {-1, 0, arities[i] + 1}) hooks->setArgFromString(index, "91");
        hooks->executeObjectMethod();
        check(observedMethod == i + 6 && observedFirst == -600 - i
            && observedSecond == (arities[i] == 2 ? 800 + i : 0),
            "numeric storage is independent per command and per argument; invalid indices do nothing");
    }
}

static void splitNodeBinding()
{
    // This deliberately does not represent a constructed Bf2HudBuilder. Only
    // identity passes through the wrapper to the field-free target observer.
    alignas(Bf2HudBuilder) unsigned char identity[sizeof(void*)] = {};
    activeBuilder = reinterpret_cast<Bf2HudBuilder*>(identity);
    currentBinding = mainConsoleObjects->getConsoleObject("HUDBUILDER", "CREATESPLITNODE");
    check(currentBinding != nullptr, "real registry resolves active split-node binding");
    if (!currentBinding) return;
    auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
    check(currentBinding->getMinNoArgs() == 2 && currentBinding->getMaxNoArgs() == 2,
        "split-node metadata requires two arguments");
    check(!std::strcmp(currentBinding->getArgType(0), "std::string")
        && !std::strcmp(currentBinding->getArgType(1), "std::string"), "split-node advertises two string arguments");
    check(!currentBinding->hasReturnValue() && hooks->getReturnValueAsString().empty(), "split-node fresh result is empty");
    observeSplitGetter = true;
    splitOrder.clear();
    check(currentBinding->execute(nullptr, 0).empty() && splitOrder.empty(), "split-node initial wrong arity skips builder lookup");
    const std::string parents[] = {"", "parent with spaces", std::string("pa\0rent", 7), std::string(4096, 'p')};
    const std::string names[] = {"", "child with spaces", std::string("na\0me", 5), std::string(8192, 'n')};
    for (int i = 0; i < 4; ++i) {
        std::string args[] = {parents[i], names[i], "excess"};
        splitSources[0] = &args[0]; splitSources[1] = &args[1];
        callbackResult = i % 2 == 0;
        splitOrder.clear();
        const unsigned previousCalls = splitCalls;
        const std::string result = currentBinding->execute(args, 2);
        check(splitOrder == std::vector<std::string>{"builder", "target"} && splitCalls == previousCalls + 1,
            "split-node wrapper gets current builder once before exact target");
        check(observedBuilder == activeBuilder && observedParent == parents[i] && observedName == names[i],
            "split-node wrapper preserves builder identity and raw string ordering");
        check(splitCopied && args[0] == parents[i] && args[1] == names[i], "split-node by-value strings isolate target mutations");
        check(splitLookupFlag && splitTargetFlag, "split-node result flag precedes builder lookup and target");
        check(result == (callbackResult ? "1" : "0") && hooks->getReturnValueAsString() == result,
            "split-node formats and caches actual boolean result");
        splitOrder.clear();
        check(currentBinding->execute(args, 3) == result && currentBinding->execute(args, 1) == result
            && currentBinding->execute(nullptr, 0) == result && currentBinding->execute(nullptr, -1) == result,
            "split-node wrong arities preserve cached return");
        check(splitOrder.empty() && splitCalls == previousCalls + 1, "split-node wrong arities skip lookup and target");
    }
    std::string raw[] = {"prime-parent", "prime-name"};
    currentBinding->execute(raw, 2);
    hooks->setArgFromString(1, "  converted-parent parent-tail");
    hooks->setArgFromString(2, "\tconverted-name name-tail");
    splitSources[0] = splitSources[1] = nullptr;
    hooks->executeObjectMethod();
    check(observedParent == "converted-parent" && observedName == "converted-name", "split-node converter retains separate token storage for each argument");
    for (int index : {-1, 0, 3}) hooks->setArgFromString(index, "ignored");
    hooks->setArgFromString(1, "");
    hooks->setArgFromString(2, " \t");
    hooks->executeObjectMethod();
    check(observedParent == "converted-parent" && observedName == "converted-name",
        "split-node invalid indices and failed string sentries preserve stored tokens");
    observeSplitGetter = false;
    activeBuilder = nullptr;
    check(!mainConsoleObjects->getConsoleObject("HudBuilder", "createSplitNode"), "split-node lookup observes absent builder");
}

// Family R. The first invokers in this table that reach neither the builder nor
// the map: each asks the HUD singleton for one of its five HudRadio owners and
// hands it a comrose entry. Executors 76DFA0, 76E430, 76E75B, 76EB2E, 76EF01.
static void radioStringMapBindings()
{
    ObserverRadio owners[5];
    for (int i = 0; i < 5; ++i) { owners[i].id = i; radioOwners[i] = &owners[i]; }
    radioOwnersPresent = true;

    // The row names do NOT read off the getter names: the infantry radio is
    // the object called radioInterface and the vehicle radio is
    // radioVehicleInterface. Each pair below is taken from the row's own
    // Availability gate, so a crossed wire fails as a wrong observer id.
    const char* objects[] = {"SQUADLEADERINTERFACE", "SQUADINTERFACE",
        "COMMANDERINTERFACE", "RADIOINTERFACE", "RADIOVEHICLEINTERFACE"};
    ConsoleObject* resolved[5] = {};
    for (int i = 0; i < 5; ++i) {
        currentBinding = resolved[i] = mainConsoleObjects->getConsoleObject(objects[i], "setStringMap");
        check(currentBinding != nullptr, "real registry resolves active radio string-map command");
        if (!currentBinding) continue;
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        check(currentBinding->getMinNoArgs() == 3 && currentBinding->getMaxNoArgs() == 3,
            "radio string-map metadata requires exactly three arguments");
        check(!std::strcmp(currentBinding->getArgType(0), "std::string")
            && !std::strcmp(currentBinding->getArgType(1), "std::string")
            && !std::strcmp(currentBinding->getArgType(2), "std::string"),
            "radio string-map advertises three string arguments");
        check(!std::strcmp(currentBinding->getReturnType(), "void") && !currentBinding->isDynamic(),
            "radio string-map declares a void non-dynamic return");
        check(!currentBinding->hasReturnValue() && hooks->getReturnValueAsString().empty(),
            "fresh radio string-map has no formatted result");

        unsigned previousCalls = radioCalls;
        check(currentBinding->execute(nullptr, 0).empty() && radioCalls == previousCalls,
            "radio string-map initial wrong arity skips the radio");

        // Every argument is std::string, so m_argNeedsConversion is zero for all
        // three and the RAW pointers are used: a quoted console argument keeps its
        // spaces and its embedded NUL all the way to addEntry.
        const std::string keys[] = {"", "key with spaces", std::string("ke\0y", 4), std::string(3072, 'K')};
        const std::string ids[] = {"HUD_RADIO_ID", "", std::string("i\0d", 3), std::string(3073, 'I')};
        const std::string commands[] = {std::string("cm\0d", 4), "game.sayAll \"hold this\"", "", std::string(3074, 'C')};
        for (int v = 0; v < 4; ++v) {
            std::string args[4] = {keys[v], ids[v], commands[v], "excess"};
            for (int a = 0; a < 3; ++a) radioSources[a] = &args[a];
            observedRadio = -1;
            previousCalls = radioCalls;
            const std::string result = currentBinding->execute(args, 3);
            check(radioCalls == previousCalls + 1 && observedRadio == i,
                "radio string-map reaches its own radio exactly once");
            check(radioKey == keys[v] && radioLocalized == ids[v] && radioCommand == commands[v],
                "complete key, localized id and command payloads arrive in order");
            check(radioCopied && args[0] == keys[v] && args[1] == ids[v] && args[2] == commands[v],
                "by-value payload copies isolate radio mutation from the console arguments");
            check(!radioSawReturnFlag, "void row clears the result flag before dispatching the radio");
            check(result.empty() && !currentBinding->hasReturnValue()
                && hooks->getReturnValueAsString().empty(),
                "void row formats and caches nothing");

            previousCalls = radioCalls;
            check(currentBinding->execute(args, 4).empty() && currentBinding->execute(args, 2).empty()
                && currentBinding->execute(args, 1).empty() && currentBinding->execute(nullptr, 0).empty()
                && currentBinding->execute(nullptr, -1).empty(),
                "radio string-map wrong arities stay empty");
            check(radioCalls == previousCalls, "radio string-map wrong arities never enter the radio");
        }
    }

    // Each row is wired to its own getter: with only one owner present the other
    // four rows fall out of the registry through their own Availability gate.
    for (int i = 0; i < 5; ++i) radioOwners[i] = nullptr;
    for (int i = 0; i < 5; ++i) {
        radioOwners[i] = &owners[i];
        for (int j = 0; j < 5; ++j)
            check((mainConsoleObjects->getConsoleObject(objects[j], "setStringMap") != nullptr) == (i == j),
                "each radio row's availability gate reads its own getter");
        radioOwners[i] = nullptr;
    }
    radioOwnersPresent = false;
    check(!mainConsoleObjects->getConsoleObject(objects[0], "setStringMap"),
        "radio string-map lookup observes an absent radio");
    // The observers are stack-owned; leave no pointer behind for later lookups.
    for (int i = 0; i < 5; ++i) radioOwners[i] = nullptr;
}

static void commanderMenuBindings()
{
    const char* noArgNames[] = {"singleClick", "rightClick", "doubleClick", "deselect"};
    check(!mainConsoleObjects->getConsoleObject("COMMANDERMENU", noArgNames[0]),
        "CommanderMenu availability observes an absent owner");

    alignas(HudCommander) unsigned char ownerStorage[sizeof(HudCommander)] = {};
    commanderOwner = reinterpret_cast<HudCommander*>(ownerStorage);
    commanderOwnerPresent = true;

    for (int i = 0; i < 4; ++i) {
        currentBinding = mainConsoleObjects->getConsoleObject("COMMANDERMENU", noArgNames[i]);
        check(currentBinding != nullptr, "real registry resolves CommanderMenu no-argument row");
        if (!currentBinding) continue;
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        check(currentBinding->getMinNoArgs() == 0 && currentBinding->getMaxNoArgs() == 0
            && !std::strcmp(currentBinding->getReturnType(), "void") && !currentBinding->isDynamic(),
            "CommanderMenu action preserves void zero-argument metadata");
        const unsigned oldCalls = commanderCalls;
        commanderGetterCalls = 0;
        const std::string result = currentBinding->execute(nullptr, 0);
        check(commanderCalls == oldCalls + 1 && observedCommanderMethod == i
            && observedCommander == commanderOwner, "CommanderMenu row reaches its exact owner action");
        check(commanderGetterCalls == 1 && !commanderGetterSawReturnFlag && !commanderSawReturnFlag,
            "CommanderMenu void row clears result before its execution owner lookup");
        check(result.empty() && !currentBinding->hasReturnValue() && hooks->getReturnValueAsString().empty(),
            "CommanderMenu void row formats no value");
        std::string excess = "ignored";
        commanderGetterCalls = 0;
        check(currentBinding->execute(&excess, 1).empty() && commanderCalls == oldCalls + 1
            && commanderGetterCalls == 0 && !currentBinding->hasReturnValue(),
            "CommanderMenu no-argument wrong arity preserves state and skips owner lookup");
    }

    currentBinding = mainConsoleObjects->getConsoleObject("COMMANDERMENU", "sendRadioMessage");
    check(currentBinding != nullptr, "real registry resolves CommanderMenu radio row");
    if (currentBinding) {
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        check(currentBinding->getMinNoArgs() == 2 && currentBinding->getMaxNoArgs() == 2
            && !std::strcmp(currentBinding->getArgType(0), "std::string")
            && !std::strcmp(currentBinding->getArgType(1), "TargetChannel")
            && !std::strcmp(currentBinding->getReturnType(), "void") && !currentBinding->isDynamic(),
            "CommanderMenu radio row preserves exact metadata");
        struct ChannelCase { const char* token; TargetChannel value; };
        const ChannelCase cases[] = {
            {"TCGlobal", TCGlobal}, {"TCTeam", TCTeam},
            {"TCTeamAndLocalEnemies", TCTeamAndLocalEnemies}, {"TCCommander", TCCommander},
            {"TCSquad", TCSquad}, {"TCSquadId", TCSquadId},
            {"TCSquadLeader", TCSquadLeader}, {"TCSquadAndLocalEnemies", TCSquadAndLocalEnemies},
            {"TCLocal", TCLocal}, {"TCLocalTeamMedic", TCLocalTeamMedic},
            {"TCLocalRevive", TCLocalRevive}, {"TCAutoLocalRevive", TCAutoLocalRevive},
            {"TCLocalGroundVehicle", TCLocalGroundVehicle}, {"TCLocalAirVehicle", TCLocalAirVehicle},
            {"TCLocalTeam", TCLocalTeam}, {"TCVehicle", TCVehicle},
            {"TCServerMessage", TCServerMessage}, {"TCServerTeamMessage", TCServerTeamMessage},
            {"TCPlayer", TCPlayer}, {"TCInvalid", TCGlobal}, {"unknown", TCGlobal}
        };
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
            std::string args[] = {i == 1 ? std::string("message with spaces")
                                              : std::string("message\0payload", 15),
                                  std::string(cases[i].token)};
            const std::string expectedMessage = args[0];
            commanderMessageSource = &args[0];
            commanderGetterCalls = 0;
            const unsigned oldCalls = commanderCalls;
            const std::string result = currentBinding->execute(args, 2);
            check(commanderCalls == oldCalls + 1 && observedCommanderMethod == 4
                && observedCommander == commanderOwner, "CommanderMenu radio reaches owner once");
            check(commanderMessage == expectedMessage && commanderMessageCopied && args[0] == expectedMessage,
                "CommanderMenu radio preserves raw string and passes an isolated by-value copy");
            check(commanderChannel == cases[i].value,
                "CommanderMenu TargetChannel parser matches the PE token table");
            check(commanderGetterCalls == 1 && !commanderGetterSawReturnFlag && !commanderSawReturnFlag
                && result.empty() && !currentBinding->hasReturnValue(),
                "CommanderMenu radio clears result before owner lookup and formats void");
        }
        std::string wrong[] = {"one", "TCTeam", "excess"};
        const unsigned oldCalls = commanderCalls;
        commanderGetterCalls = 0;
        check(currentBinding->execute(wrong, 3).empty() && currentBinding->execute(wrong, 1).empty()
            && currentBinding->execute(nullptr, 0).empty(),
            "CommanderMenu radio wrong arities preserve empty result");
        check(commanderCalls == oldCalls && commanderGetterCalls == 0,
            "CommanderMenu radio wrong arities skip owner lookup and target");

        std::string prime[] = {"prime", "TCTeam"};
        currentBinding->execute(prime, 2);
        hooks->setArgFromString(1, "  converted-message tail");
        hooks->setArgFromString(2, "  TCPlayer tail");
        commanderMessageSource = nullptr;
        hooks->executeObjectMethod();
        check(commanderMessage == "converted-message" && commanderChannel == TCPlayer,
            "CommanderMenu explicit converters retain independent token storage");
        for (int index : {-1, 0, 3}) hooks->setArgFromString(index, "TCTeam");
        hooks->setArgFromString(1, "");
        hooks->executeObjectMethod();
        check(commanderMessage == "converted-message" && commanderChannel == TCPlayer,
            "CommanderMenu invalid converter indices and failed string extraction preserve storage");
    }

    currentBinding = mainConsoleObjects->getConsoleObject("COMMANDERMENU", "vehicleDropReloadTime");
    check(currentBinding != nullptr, "real registry resolves CommanderMenu duration property");
    if (currentBinding) {
        auto* hooks = static_cast<ConsoleObjectBaseImpl*>(currentBinding);
        check(currentBinding->isSetGet() && currentBinding->getMinNoArgs() == 0
            && currentBinding->getMaxNoArgs() == 1 && !std::strcmp(currentBinding->getReturnType(), "float")
            && !std::strcmp(currentBinding->getArgType(0), "float") && currentBinding->isDynamic(),
            "CommanderMenu duration preserves float property metadata");
        commanderOwner->m_vehicleDropDuration = 2.75f;
        commanderOwner->m_vehicleDropDeadline = 91.0f;
        commanderOwner->m_vehicleDropRemaining = 37.0f;
        commanderOwner->m_vehicleDropFraction = .625f;
        commanderOwner->m_vehicleDropReloading = true;
        commanderGetterCalls = 0;
        check(currentBinding->execute(nullptr, 0) == "2.75" && currentBinding->hasReturnValue(),
            "CommanderMenu duration getter formats and caches current float");
        check(commanderGetterCalls == 1 && commanderGetterSawReturnFlag,
            "CommanderMenu duration getter raises result flag before owner lookup");
        std::string set = "8.5 trailing";
        commanderGetterCalls = 0;
        check(currentBinding->execute(&set, 1).empty() && !currentBinding->hasReturnValue()
            && commanderOwner->m_vehicleDropDuration == 8.5f,
            "CommanderMenu duration setter converts and writes the exact field");
        check(commanderGetterCalls == 1 && !commanderGetterSawReturnFlag,
            "CommanderMenu duration setter clears result before owner lookup");
        check(commanderOwner->m_vehicleDropDeadline == 91.0f
            && commanderOwner->m_vehicleDropRemaining == 37.0f
            && commanderOwner->m_vehicleDropFraction == .625f
            && commanderOwner->m_vehicleDropReloading,
            "CommanderMenu duration setter leaves active cooldown state untouched");
        check(currentBinding->execute(nullptr, 0) == "8.5", "CommanderMenu duration getter sees updated field");
        std::string excess[] = {"44", "55"};
        commanderGetterCalls = 0;
        check(currentBinding->execute(excess, 2) == "8.5" && currentBinding->hasReturnValue()
            && commanderOwner->m_vehicleDropDuration == 8.5f && commanderGetterCalls == 0,
            "CommanderMenu duration excess arity preserves cached result and owner state");
        hooks->setArgFromString(1, "-3.25 suffix");
        hooks->executeObjectMethod();
        check(commanderOwner->m_vehicleDropDuration == 8.5f,
            "CommanderMenu direct method hook retains excess argument count");
        std::string prime = "0";
        currentBinding->execute(&prime, 1);
        hooks->setArgFromString(1, "-3.25 suffix");
        hooks->executeObjectMethod();
        check(commanderOwner->m_vehicleDropDuration == -3.25f && !currentBinding->hasReturnValue(),
            "CommanderMenu float conversion hook stores the parsed token");
    }

    commanderOwnerPresent = false;
    check(!mainConsoleObjects->getConsoleObject("COMMANDERMENU", noArgNames[0]),
        "CommanderMenu lookup observes owner removal");
    commanderOwner = nullptr;
}

int main()
{
    static_assert(sizeof(void*) == 4, "x86 only");
    sharedBaseDispatch();
    const std::size_t initialSize = mainConsoleObjects->getConsoleObjects().size();
    HudConsoleModule module;
    check(module.init(), "real HUD console module registers bindings");
    check(mainConsoleObjects->getConsoleObjects().size() == initialSize + 228, "real registry receives complete binding list");
    check(!mainConsoleObjects->getConsoleObject("HUDMANAGER", "SETMOUSETEXTUREEMPTY"), "null manager keeps binding inactive");
    void* storage = ::operator new(sizeof(ObserverHud));
    std::memset(storage, 0x5a, sizeof(ObserverHud));
    ObserverHud* observer = new(storage) ObserverHud;
    hudManager = observer;

    const char* names[] = {"setMouseTextureEmpty", "setMouseTextureFull", "setMouseTextureArtillery",
        "setMouseTextureUAV", "setMouseTextureSupply", "setMouseTextureCancel"};
    ConsoleObject* resolved[6] = {};
    for (int i = 0; i < 6; ++i) {
        ConsoleObject* binding = mainConsoleObjects->getConsoleObject("HUDMANAGER", names[i]);
        resolved[i] = binding;
        check(binding != nullptr, "real registry resolves active typed HUD command");
        if (binding) exercise(binding, i);
    }
    conversionHooks(resolved);
    numericBindings();
    splitNodeBinding();
    radioStringMapBindings();
    commanderMenuBindings();
    hudManager = nullptr;
    check(!mainConsoleObjects->getConsoleObject("hudManager", names[0]), "lookup observes manager removal after execution");
    check(module.close() && mainConsoleObjects->getConsoleObjects().size() == initialSize, "real module close restores registry");
    destroyConstructorOnly(observer);
    std::printf("HUD console invokers: %u/%u checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
