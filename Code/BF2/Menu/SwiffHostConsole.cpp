// [CHOSEN] Console adapters for the menu host methods.
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "ISwiffHost.h"
#include "../Core/ConsoleObjectMacro.h"
#include "../Game/Main/BF2ClientSeams.h"

namespace dice { namespace hfe {
namespace {
ConsoleObjects::ConsoleObjectList& bindingStorage()
{
    static ConsoleObjects::ConsoleObjectList bindings;
    return bindings;
}

SetConsoleClassDefaults defaults("SwiffHost", 2, 0);

// Every row is a non-dynamic void method with a fixed arity; the constructors store the same fields
// in the same order (4111CB and its siblings) and append before the argument metadata.
class SwiffHostConsoleBinding : public ConsoleObjectBaseImpl {
protected:
    SwiffHostConsoleBinding(const char* function, int arity)
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
        bindingStorage().push_back(this);
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
    bool isObjectActive() const override { return g_swiffHost != nullptr; }
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

// ranks.con: `swiffHost.addRank <id> <a> <b> <c> RANK_<id>`. The executor reads the four ints in
// order, copies the name, clears the return flag and calls +0x29C with the copy by reference.
class AddRankBinding : public SwiffHostConsoleBinding {
public:
    AddRankBinding() : SwiffHostConsoleBinding("addRank", 5)
    {
        argument(0, "int"); argument(1, "int"); argument(2, "int"); argument(3, "int");
        argument(4, "std::string");
        conversionFlags(5);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 5) return;
        const int id = BF2_CC_ARG(int, 0);
        const int a = BF2_CC_ARG(int, 1);
        const int b = BF2_CC_ARG(int, 2);
        const int c = BF2_CC_ARG(int, 3);
        const std::string name(*m_args[4]);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->addRank(id, a, b, c, name);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(int, 1)
        BF2_CC_CONVERT(int, 2)
        BF2_CC_CONVERT(int, 3)
        BF2_CC_CONVERT(int, 4)
        BF2_CC_CONVERT(std::string, 5)
    }
};

// awards.con: `swiffHost.addAward 0 <id> <level> <priority> "bf2"`. The same reads as addRank, then
// +0x2A0 takes the string by value - the executor copies its local copy a second time into the
// argument area (4115A4).
class AddAwardBinding : public SwiffHostConsoleBinding {
public:
    AddAwardBinding() : SwiffHostConsoleBinding("addAward", 5)
    {
        argument(0, "int"); argument(1, "int"); argument(2, "int"); argument(3, "int");
        argument(4, "std::string");
        conversionFlags(5);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 5) return;
        const int first = BF2_CC_ARG(int, 0);
        const int id = BF2_CC_ARG(int, 1);
        const int level = BF2_CC_ARG(int, 2);
        const int priority = BF2_CC_ARG(int, 3);
        const std::string mod(*m_args[4]);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->slot2A0(first, id, level, priority, mod);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(int, 1)
        BF2_CC_CONVERT(int, 2)
        BF2_CC_CONVERT(int, 3)
        BF2_CC_CONVERT(int, 4)
        BF2_CC_CONVERT(std::string, 5)
    }
};

class AddGameModeBinding : public SwiffHostConsoleBinding {
public:
    AddGameModeBinding() : SwiffHostConsoleBinding("addGameMode", 2)
    {
        argument(0, "std::string"); argument(1, "std::string");
        conversionFlags(2);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 2) return;
        const std::string mode(*m_args[0]);
        const std::string name(*m_args[1]);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->addGameMode(mode, name);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(std::string, 1)
        BF2_CC_CONVERT(std::string, 2)
    }
};

// Binds addCredit(string, int), appending a copied text line and its category to the host's credits.
class AddCreditBinding : public SwiffHostConsoleBinding {
public:
    AddCreditBinding() : SwiffHostConsoleBinding("addCredit", 2)
    {
        argument(0, "std::string"); argument(1, "int");
        conversionFlags(2);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 2) return;
        const std::string text(*m_args[0]);
        const int value = BF2_CC_ARG(int, 1);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->addEntryEC8(text, value);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(std::string, 1)
        BF2_CC_CONVERT(int, 2)
    }
};

// credits.con's scroller setup: `setScrollFadeSpeed 200` -> +0x318 (one store, 0x488DF0).
class SetScrollFadeSpeedBinding : public SwiffHostConsoleBinding {
public:
    SetScrollFadeSpeedBinding() : SwiffHostConsoleBinding("setScrollFadeSpeed", 1)
    {
        argument(0, "int");
        conversionFlags(1);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 1) return;
        const int speed = BF2_CC_ARG(int, 0);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->setIntF08(speed);
    }
    void setArgFromString(int index, const std::string& value) override
    { BF2_CC_CONVERT(int, 1) }
};

// `setScrollSpeed 1200` -> +0x32C (clamped to 500..4000 by the host, 0x488F20).
class SetScrollSpeedBinding : public SwiffHostConsoleBinding {
public:
    SetScrollSpeedBinding() : SwiffHostConsoleBinding("setScrollSpeed", 1)
    {
        argument(0, "float");
        conversionFlags(1);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 1) return;
        const float speed = BF2_CC_ARG(float, 0);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->setFloatF0C(speed);
    }
    void setArgFromString(int index, const std::string& value) override
    { BF2_CC_CONVERT(float, 1) }
};

// `setScrollerRect 0 118 1024 668` -> +0x34C (four stores, 0x489020).
class SetScrollerRectBinding : public SwiffHostConsoleBinding {
public:
    SetScrollerRectBinding() : SwiffHostConsoleBinding("setScrollerRect", 4)
    {
        argument(0, "int"); argument(1, "int"); argument(2, "int"); argument(3, "int");
        conversionFlags(4);
    }
    void executeObjectMethod() override
    {
        if (m_argCount != 4) return;
        const int x = BF2_CC_ARG(int, 0);
        const int y = BF2_CC_ARG(int, 1);
        const int width = BF2_CC_ARG(int, 2);
        const int height = BF2_CC_ARG(int, 3);
        m_hasReturnValue = false;
        static_cast<ui::ISwiffHost*>(g_swiffHost)->setRect(x, y, width, height);
    }
    void setArgFromString(int index, const std::string& value) override
    {
        BF2_CC_CONVERT(int, 1)
        BF2_CC_CONVERT(int, 2)
        BF2_CC_CONVERT(int, 3)
        BF2_CC_CONVERT(int, 4)
    }
};

// Static initialization order 84184D, 841863, 841879, 84188F, 8418BB, 8418D1, 8418FD (the three
// unbound rows' slots, 84183C / 8418A5 / 8418E7, are skipped).
AddRankBinding addRankBinding;
AddAwardBinding addAwardBinding;
AddGameModeBinding addGameModeBinding;
AddCreditBinding addCreditBinding;
SetScrollFadeSpeedBinding setScrollFadeSpeedBinding;
SetScrollSpeedBinding setScrollSpeedBinding;
SetScrollerRectBinding setScrollerRectBinding;
} // namespace

ConsoleObjects::ConsoleObjectList& swiffHostConsoleObjects() { return bindingStorage(); }
void registerSwiffHostConsoleObjects()
{ mainConsoleObjects->registerConsoleObjects(swiffHostConsoleObjects()); }
void unregisterSwiffHostConsoleObjects()
{ mainConsoleObjects->unregisterConsoleObjects(swiffHostConsoleObjects()); }

} }
#endif // !dedicated
