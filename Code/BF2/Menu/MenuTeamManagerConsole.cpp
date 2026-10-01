// Client team-manager console bindings, registered once by ModuleGame.
// [INFERRED] Source placement and generated class names.
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "IMenuTeamManager.h"
#include "../Core/ConsoleObjectMacro.h"
#include "../Game/Main/BF2ClientSeams.h"

namespace dice { namespace hfe {
namespace {

ConsoleObjects::ConsoleObjectList& bindingStorage()
{
    static ConsoleObjects::ConsoleObjectList bindings;
    return bindings;
}

// 8432A3 calls SetConsoleClassDefaults("MenuTeamManager",2,0), which lowercases
// only the initial character. All four rows are non-dynamic void methods.
SetConsoleClassDefaults defaults("MenuTeamManager", 2, 0);

class MenuTeamConsoleBinding : public ConsoleObjectBaseImpl {
protected:
    MenuTeamConsoleBinding(const char* function, int arity)
    {
        m_objectName = consoleDefaultName();
        m_functionName = function;
        m_isDynamic = false;
        m_type = 0;
        m_access = consoleDefaultAccess();
        m_category = consoleDefaultCategory();
        m_minNoArgs = arity;
        m_maxNoArgs = arity;
        m_returnType = "void";
        m_hasReturnValue = false;
        bindingStorage().push_back(this); // Retail appends before argument metadata.
    }
    void argument(int index, const char* type)
    {
        m_argTypes[index] = type;
    }
    void conversionFlags(int arity)
    {
        for (int i = 0; i < arity; ++i)
            m_argNeedsConversion[i] = std::string(m_argTypes[i]) == "std::string" ? 0 : -1;
    }
public:
    bool isObjectActive() const override { return g_menuTeamManager != nullptr; }
    std::string getReturnValueAsString() const override { return std::string(); }
    std::string getObjectDeltaAsString() const override { return "No Delta"; }
    bool objectHasRange() const override { return false; }
    std::pair<const void*, const void*> getObjectRange() const override
    { return std::pair<const void*, const void*>(nullptr, nullptr); }
    std::pair<std::string, std::string> getObjectRangeAsString() const override
    { return std::pair<std::string, std::string>(); }
    void checkObjectRange(bool) override {} //586B53 RET4
    bool objectHasBundledType() const override { return false; }
    std::string getObjectTypeDescription() const override { return ""; }
};

class AddTeamBinding : public MenuTeamConsoleBinding {
public:
    AddTeamBinding() : MenuTeamConsoleBinding("addTeam", 2)
    { argument(0, "std::string"); argument(1, "std::string"); conversionFlags(2); }
    void executeObjectMethod() override //copies argument0 before argument1
    {
        if (m_argCount != 2) return;
        const std::string name(*m_args[0]);
        const std::string path(*m_args[1]);
        m_hasReturnValue = false;
        static_cast<IMenuTeamManager*>(g_menuTeamManager)->addTeam(name, path);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(std::string, 1)
        BF2_CC_CONVERT(std::string, 2)
    }
};

class SetTeamIdBinding : public MenuTeamConsoleBinding {
public:
    SetTeamIdBinding() : MenuTeamConsoleBinding("setTeamId", 1)
    { argument(0, "int"); conversionFlags(1); }
    void executeObjectMethod() override
    {
        if (m_argCount != 1) return;
        const int id = BF2_CC_ARG(int, 0);
        m_hasReturnValue = false;
        static_cast<IMenuTeamManager*>(g_menuTeamManager)->setTeamId(id);
    }
    void setArgFromString(int index, const std::string& value) override
    { BF2_CC_CONVERT(int, 1) }
};

class AddKitBinding : public MenuTeamConsoleBinding {
public:
    AddKitBinding() : MenuTeamConsoleBinding("addKit", 1)
    { argument(0, "std::string"); conversionFlags(1); }
    void executeObjectMethod() override
    {
        if (m_argCount != 1) return;
        const std::string file(*m_args[0]);
        m_hasReturnValue = false;
        static_cast<IMenuTeamManager*>(g_menuTeamManager)->addKit(file);
    }
    void setArgFromString(int index, const std::string& value) override
    { BF2_CC_CONVERT(std::string, 1) }
};

class AddWeaponBinding : public MenuTeamConsoleBinding {
public:
    AddWeaponBinding() : MenuTeamConsoleBinding("addWeapon", 4)
    {
        argument(0, "std::string"); argument(1, "int");
        argument(2, "int"); argument(3, "int");
        conversionFlags(4);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 4) return;
        const std::string name(*m_args[0]);
        const int accuracy = BF2_CC_ARG(int, 2);
        const int rateOfFire = BF2_CC_ARG(int, 3);
        const int damageType = BF2_CC_ARG(int, 1);
        m_hasReturnValue = false;
        static_cast<IMenuTeamManager*>(g_menuTeamManager)->addWeapon(name, damageType, accuracy, rateOfFire);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(std::string, 1)
        BF2_CC_CONVERT(int, 2)
        BF2_CC_CONVERT(int, 3)
        BF2_CC_CONVERT(int, 4)
    }
};

// Static initialization order8432B7,8432CD,8432E3,8432F9. Do not add a lowercase
// addweapon alias: the one retail spelling already resolves case-insensitively.
AddTeamBinding addTeamBinding;
SetTeamIdBinding setTeamIdBinding;
AddKitBinding addKitBinding;
AddWeaponBinding addWeaponBinding;
} // namespace

ConsoleObjects::ConsoleObjectList& menuTeamConsoleObjects() { return bindingStorage(); }
void registerMenuTeamConsoleObjects()
{ mainConsoleObjects->registerConsoleObjects(menuTeamConsoleObjects()); }
void unregisterMenuTeamConsoleObjects()
{ mainConsoleObjects->unregisterConsoleObjects(menuTeamConsoleObjects()); }

} } // namespace dice::hfe
#endif // !dedicated
