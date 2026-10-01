// [INFERRED] names/path. Metadata/lifetime are recovered; command execution is
// explicitly diagnostic scaffolding.
#include "ProfileConsoleBindings.h"
#if !DEDICATED
#include "../Core/ConsoleObjectBase.h"
#include "../Core/ConsoleObjectMacro.h"
#include <cstdio>
#include <cstdlib>

namespace dice { namespace hfe { namespace hud {

ProfileConsoleOwner* profileConsoleOwner = nullptr;

ProfileConsoleOwner::ProfileConsoleOwner()
    : m_emptyName(""), m_selectedProfile(nullptr), m_activeProfile(nullptr)
{
    // a second owner does not replace the published first one.
    if (!profileConsoleOwner) profileConsoleOwner = this;
}

ProfileConsoleOwner::~ProfileConsoleOwner()
{
    // clear the singleton before native map/string destruction. The tree
    // erase7BFE80/7BFC10 destroys keys/nodes only, never mapped PlayerProfile*s.
    if (profileConsoleOwner == this) profileConsoleOwner = nullptr;
}

namespace {
ConsoleObjects::ConsoleObjectList profileBindings; // static initializer864291
SetConsoleClassDefaults profileDefaults("Profile", 2, 0);

enum class Activation { Owner, SelectedProfile, ActiveProfile, SelectedCharacter, ActiveCharacter };

PlayerProfile* selectedProfile()
{
    return profileConsoleOwner ? profileConsoleOwner->getSelectedProfile() : nullptr;
}
PlayerProfile* activeProfile()
{
    return profileConsoleOwner ? profileConsoleOwner->getActiveProfile() : nullptr;
}
CharacterProfile* selectedCharacter()
{
    PlayerProfile* profile = selectedProfile();
    return profile ? profile->getSelectedCharacter() : nullptr;
}
CharacterProfile* activeCharacter()
{
    PlayerProfile* profile = activeProfile();
    return profile ? profile->getActiveCharacter() : nullptr;
}

// [CHOSEN, not recovered] Shared metadata for 17 profile console bindings.
// Unimplemented conversion, invocation and formatting methods stop with a diagnostic.
class ProfileConsoleBinding : public ConsoleObjectBaseImpl {
public:
    ProfileConsoleBinding(const char* name, int type, int minArgs, int maxArgs,
        const char* returnType, Activation activation, bool dynamic,
        const char* arg0 = nullptr, const char* arg1 = nullptr, const char* arg2 = nullptr)
        : m_activation(activation)
    {
        m_objectName = consoleDefaultName();
        m_access = consoleDefaultAccess();
        m_category = consoleDefaultCategory();
        m_functionName = name;
        m_type = type;
        m_minNoArgs = minArgs;
        m_maxNoArgs = maxArgs;
        m_returnType = returnType;
        m_hasReturnValue = false;
        m_isDynamic = dynamic;
        // The PE constructors append before initializing argument metadata.
        profileBindings.push_back(this);
        const char* arguments[] = {arg0, arg1, arg2};
        for (int i = 0; i < maxArgs; ++i) {
            m_argTypes[i] = arguments[i];
            m_argNeedsConversion[i] = std::string(arguments[i]) == "std::string" ? 0 : -1;
        }
        // Unused argument slots and execution state remain untouched.
    }
    bool isObjectActive() const override
    {
        switch (m_activation) {
        case Activation::Owner: return profileConsoleOwner != nullptr;
        case Activation::SelectedProfile: return selectedProfile() != nullptr;
        case Activation::ActiveProfile: return activeProfile() != nullptr;
        case Activation::SelectedCharacter: return selectedCharacter() != nullptr;
        case Activation::ActiveCharacter: return activeCharacter() != nullptr;
        }
        std::abort(); // impossible private enum value, not a retail availability branch
    }
    std::string getObjectDeltaAsString() const override { return "No Delta"; }

    // [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: PROFILE-CONSOLE-INVOKERS]
    // These known interface signatures deliberately stop before any execution.
    std::string execute(std::string*, int) override
    {
        std::fprintf(stderr, "[FRONTIER] dice::hfe::hud::ProfileConsoleBinding::execute: profile.%s\n", m_functionName);
        std::fflush(stderr);
        std::abort();
    }
    void executeObjectMethod() override
    {
        std::fprintf(stderr, "[FRONTIER] dice::hfe::hud::ProfileConsoleBinding::executeObjectMethod: profile.%s\n", m_functionName);
        std::fflush(stderr);
        std::abort();
    }
    void setArgFromString(int, const std::string&) override
    {
        std::fprintf(stderr, "[FRONTIER] dice::hfe::hud::ProfileConsoleBinding::setArgFromString: profile.%s\n", m_functionName);
        std::fflush(stderr);
        std::abort();
    }
    std::string getReturnValueAsString() const override
    {
        std::fprintf(stderr, "[FRONTIER] dice::hfe::hud::ProfileConsoleBinding::getReturnValueAsString: profile.%s\n", m_functionName);
        std::fflush(stderr);
        std::abort();
    }
private:
    Activation m_activation; // chosen metadata, not a claimed retail generated-class suffix
};

// CRT pointers96D74C..96D78C prove this exact order. Constructors7702E5..771819.
ProfileConsoleBinding bindings[] = {
    {"createPlayerProfile", 0, 1, 1, "bool", Activation::Owner, true, "std::string"},
    {"activePlayerProfile", 1, 0, 1, "std::string", Activation::Owner, true, "std::string"},
    {"activatePlayerProfile", 0, 1, 1, "bool", Activation::Owner, true, "std::string"},
    {"rank", 1, 0, 1, "int", Activation::SelectedProfile, true, "int"},
    {"addCharacterProfile", 0, 1, 1, "bool", Activation::SelectedProfile, true, "std::string"},
    {"activeCharacterProfile", 1, 0, 1, "std::string", Activation::SelectedProfile, true, "std::string"},
    {"activateCharacterProfile", 0, 1, 1, "bool", Activation::ActiveProfile, true, "std::string"},
    {"addModSetting", 0, 1, 1, "bool", Activation::SelectedCharacter, true, "std::string"},
    {"activeMod", 1, 0, 1, "std::string", Activation::SelectedCharacter, true, "std::string"},
    {"playerName", 1, 0, 1, "std::string", Activation::SelectedCharacter, true, "std::string"},
    {"visualUnlockLevel", 1, 0, 1, "int", Activation::SelectedCharacter, true, "int"},
    {"defaultKit", 1, 0, 1, "int", Activation::SelectedCharacter, true, "int"},
    {"tagIcon", 1, 0, 1, "std::string", Activation::SelectedCharacter, true, "std::string"},
    {"tagText", 1, 0, 1, "std::string", Activation::SelectedCharacter, true, "std::string"},
    {"setIndexForKit", 0, 3, 3, "void", Activation::SelectedCharacter, false, "std::string", "int", "int"},
    {"getIndexForKit", 0, 2, 2, "void", Activation::SelectedCharacter, false, "std::string", "int"},
    {"applySettings", 0, 0, 0, "void", Activation::ActiveCharacter, false}
};
} // namespace

ConsoleObjects::ConsoleObjectList& profileConsoleBindingList() { return profileBindings; }

} } }
#endif
