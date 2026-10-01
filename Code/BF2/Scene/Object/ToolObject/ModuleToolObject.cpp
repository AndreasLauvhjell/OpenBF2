// Object and template console bindings used by level scripts and editor controls.
// Dispatches through objectTemplateAdm. Preserve registered command spelling and argument conversion.
// [FRONTIER] Unimplemented editor bindings remain marked at their declarations/bodies.
#include "../../../Core/ConsoleObjectMacro.h"
#include "../ObjectTemplateUtil.h"                        // the two _ptrproxy types
#include "../ObjectTypes/Base/ObjectTemplateManager.h"    // the argc-0 arm's active template
#include "../ObjectManager.h"                             // the message rows' argc-0 active object
#include "ObjectTemplateAdm.h"

#include <sstream>
#include <string>

namespace dice {
namespace hfe {
namespace tool {

namespace {
// Stands in for the original's file-scope list (ELF 0x8B1EF24).
const char* const kToolObjectConsoleTag = "tool.moduleToolObject";
} // namespace

BF2_CONSOLE_OBJECT("Object", 2, 0)

// Creates from the active template or explicit template/position/rotation arguments (0..3).
// Handled calls return an object proxy; other counts leave return state untouched.
// Arguments use guarded static proxy/Vec3 storage and one-based conversion indices.
namespace {
// Written out rather than through BF2_CC_HEAD/tail (which key the class name to __LINE__ and only
// hold together inside a single-line macro): the same field set, the same conversion-flag loop,
// the same self-registration - the generator's shape for a 0..3-argument, proxy-returning method.
class ConsoleClass125 : public ::dice::hfe::ConsoleObjectBaseImpl
{
public:
    ConsoleClass125()
    {
        m_objectName     = ::dice::hfe::consoleDefaultName();
        m_access         = ::dice::hfe::consoleDefaultAccess();
        m_category       = ::dice::hfe::consoleDefaultCategory();
        m_functionName   = "create";
        m_type           = 0;
        m_minNoArgs      = 0;
        m_maxNoArgs      = 3;
        m_returnType     = "world::IObject_ptrproxy";
        m_hasReturnValue = false;
        m_isDynamic      = false;                                    // +0xA1 = 0 in this TU
        m_argTypes[0]    = "world::IObjectTemplate_ptrproxy";
        m_argTypes[1]    = "Vec3";
        m_argTypes[2]    = "Vec3";
        for (int i = 0; i < 3; ++i)
            m_argNeedsConversion[i] = (std::string(m_argTypes[i]) == "std::string") ? 0 : -1;
        ::dice::hfe::consoleBindingList(kToolObjectConsoleTag).push_back(this);
    }
    virtual bool isObjectActive() const { return true; }
    virtual void executeObjectMethod()
    {
        world::IObject* object;
        switch (m_argCount)
        {
        case 0:
            m_hasReturnValue = true;
            object = objectTemplateAdm.createObject(world::objectTemplateManager->getActiveObjectTemplate());
            break;
        case 1:
            m_hasReturnValue = true;
            object = objectTemplateAdm.createObject(BF2_CC_ARG(world::IObjectTemplate_ptrproxy, 0).ptr);
            break;
        case 2:
            m_hasReturnValue = true;
            object = objectTemplateAdm.createObject(BF2_CC_ARG(world::IObjectTemplate_ptrproxy, 0).ptr,
                                                    BF2_CC_ARG(Vec3, 1));
            break;
        case 3:
            m_hasReturnValue = true;
            object = objectTemplateAdm.createObject(BF2_CC_ARG(world::IObjectTemplate_ptrproxy, 0).ptr,
                                                    BF2_CC_ARG(Vec3, 1), BF2_CC_ARG(Vec3, 2));
            break;
        default:
            return;
        }
        m_returnValue.ptr = object;
    }
    virtual void setArgFromString(int index, const std::string& value)
    {
        BF2_CC_CONVERT(world::IObjectTemplate_ptrproxy, 1)
        BF2_CC_CONVERT(Vec3, 2)
        BF2_CC_CONVERT(Vec3, 3)
    }
    BF2_CC_RETURN_AS_STRING
    BF2_CC_COMMON_HOOKS
private:
    world::IObject_ptrproxy m_returnValue;               // +0xA4
};
static ConsoleClass125 s_consoleClass125;
} // namespace

// Starts/stops the active object or an explicitly parsed object proxy; publishes no return value.
// Parse directly into the shared static proxy: failed extraction still replaces its pointer.
// A temporary-and-commit conversion would incorrectly retain the previous target.
// Explicit ConsoleClass138..141 names share the macro-generated namespace; avoid line-number
// collisions with BF2_CC_NAME registrations when editing this file.
#define BF2_TOOL_OBJECT_MESSAGE_ROW(ClassName, fnLiteral, admTarget)                               \
namespace {                                                                                        \
class ClassName : public ::dice::hfe::ConsoleObjectBaseImpl                                        \
{                                                                                                  \
public:                                                                                            \
    ClassName()                                                                                    \
    {                                                                                              \
        m_objectName     = ::dice::hfe::consoleDefaultName();                                      \
        m_access         = ::dice::hfe::consoleDefaultAccess();                                    \
        m_category       = ::dice::hfe::consoleDefaultCategory();                                  \
        m_functionName   = (fnLiteral);                                                            \
        m_type           = 0;                                                                      \
        m_minNoArgs      = 0;                                                                      \
        m_maxNoArgs      = 1;                                                                      \
        m_returnType     = "void";                                                                 \
        m_hasReturnValue = false;                                                                  \
        m_isDynamic      = false;                                  /* +0xA1 = 0 in this TU */      \
        m_argTypes[0]    = "world::IObject_ptrproxy";                                              \
        m_argNeedsConversion[0] = (std::string(m_argTypes[0]) == "std::string") ? 0 : -1;         \
        ::dice::hfe::consoleBindingList(kToolObjectConsoleTag).push_back(this);                    \
    }                                                                                              \
    virtual bool isObjectActive() const { return true; }                                           \
    virtual void executeObjectMethod()                                                             \
    {                                                                                              \
        if (m_argCount == 0)                                                                       \
        {                                                                                          \
            m_hasReturnValue = false;                                                              \
            objectTemplateAdm.admTarget(world::objectManager->getActiveObject());                  \
        }                                                                                          \
        else if (m_argCount == 1)                                                                  \
        {                                                                                          \
            m_hasReturnValue = false;                                                              \
            objectTemplateAdm.admTarget(BF2_CC_ARG(world::IObject_ptrproxy, 0).ptr);               \
        }                                                                                          \
    }                                                                                              \
    virtual void setArgFromString(int index, const std::string& value)                             \
    {                                                                                              \
        if (index == 1)                                                                            \
        {                                                                                          \
            static world::IObject_ptrproxy s;                                                      \
            std::istringstream in(value);                                                          \
            in >> s;                                                                               \
            m_args[0] = reinterpret_cast<std::string*>(&s);                                        \
        }                                                                                          \
    }                                                                                              \
    BF2_CC_NO_RETURN_AS_STRING                                                                     \
    BF2_CC_COMMON_HOOKS                                                                            \
};                                                                                                 \
static ClassName s_##ClassName;                                                                    \
}

BF2_TOOL_OBJECT_MESSAGE_ROW(ConsoleClass138, "start",      start)
BF2_TOOL_OBJECT_MESSAGE_ROW(ConsoleClass139, "stop",       stop)
BF2_TOOL_OBJECT_MESSAGE_ROW(ConsoleClass140, "forceStart", forceStart)
BF2_TOOL_OBJECT_MESSAGE_ROW(ConsoleClass141, "forceStop",  forceStop)
#undef BF2_TOOL_OBJECT_MESSAGE_ROW

// Selects an active template from class name and template arguments. The class lookups
// are performed but their results are discarded; activeSafe does not enforce class equality.
BF2_CONSOLE_OBJECT("ObjectTemplate", 2, 0)

// Gets or selects the active template through a guarded proxy.
// No argument returns its name or NoObjectTemplate; an unknown name clears the active cursor.
// Uses the one-argument administrator setter and ignores its result; extraction writes the proxy directly.
namespace {
class ConsoleClass150 : public ::dice::hfe::ConsoleObjectBaseImpl
{
public:
    ConsoleClass150()
    {
        m_objectName     = ::dice::hfe::consoleDefaultName();
        m_access         = ::dice::hfe::consoleDefaultAccess();
        m_category       = ::dice::hfe::consoleDefaultCategory();
        m_functionName   = "active";
        m_type           = 1;
        m_minNoArgs      = 0;
        m_maxNoArgs      = 1;
        m_returnType     = "world::IObjectTemplate_ptrproxy";
        m_hasReturnValue = false;
        m_isDynamic      = false;                                    // +0xA1 = 0 in this TU
        m_argTypes[0]    = "world::IObjectTemplate_ptrproxy";
        m_argNeedsConversion[0] = (std::string(m_argTypes[0]) == "std::string") ? 0 : -1;
        ::dice::hfe::consoleBindingList(kToolObjectConsoleTag).push_back(this);
    }
    virtual bool isObjectActive() const { return true; }
    virtual void executeObjectMethod()
    {
        if (m_argCount == 0)
        {
            m_hasReturnValue = true;
            m_returnValue.ptr = world::objectTemplateManager->getActiveObjectTemplate();
        }
        else if (m_argCount == 1)
        {
            m_hasReturnValue = false;
            objectTemplateAdm.setActiveObjectTemplate(BF2_CC_ARG(world::IObjectTemplate_ptrproxy, 0).ptr);
        }
    }
    virtual void setArgFromString(int index, const std::string& value)
    {
        if (index == 1)
        {
            static world::IObjectTemplate_ptrproxy s;
            std::istringstream in(value);
            in >> s;
            m_args[0] = reinterpret_cast<std::string*>(&s);
        }
    }
    BF2_CC_RETURN_AS_STRING
    BF2_CC_COMMON_HOOKS
private:
    world::IObjectTemplate_ptrproxy m_returnValue;       // +0xA4
};
static ConsoleClass150 s_consoleClass150;
} // namespace

namespace {
void consoleObjectTemplateActiveSafe(const std::string& className, world::IObjectTemplate_ptrproxy tmpl)
{
    objectTemplateAdm.setActiveObjectTemplate(className, tmpl.ptr);
}
} // namespace
BF2_CONSOLE_METHOD_2(activeSafe, consoleObjectTemplateActiveSafe, std::string, world::IObjectTemplate_ptrproxy, kToolObjectConsoleTag)

// Handed to the console registry by tool::ModuleToolObject::init (Core/ModuleRegistry.cpp).
ConsoleObjectList& toolObjectConsoleObjects()
{
    return consoleBindingList(kToolObjectConsoleTag);
}

} // namespace tool
} // namespace hfe
} // namespace dice
