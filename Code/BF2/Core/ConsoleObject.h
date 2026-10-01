// Console-command interface shared by generated bindings and console::Wrapper. Header path [INFERRED].
// Keep all 23 virtual declarations in order; ConsoleObjectBaseImpl appends its own hooks.

#ifndef CORE_CONSOLEOBJECT_H     // [INFERRED] guard style
#define CORE_CONSOLEOBJECT_H

#include <string>
#include <utility>

namespace dice {
namespace hfe {

class ConsoleObject
{
public:
    virtual ~ConsoleObject();                                 // +0x00 / +0x04

    // identity
    // getObjectName() and getFunctionName() are the two slots ConsoleObjects keys its registry on:
    // the map key is their plain concatenation, with no separator. See Core/ConsoleObjects.cpp.
    virtual void*         getObject() const = 0;              // +0x08
    virtual const char*   getObjectName() const = 0;          // +0x0C
    virtual const char*   getFunctionName() const = 0;        // +0x10

    // Metadata
    // getType() encodes the access direction; the value set is documented on
    // ConsoleObjectBaseImpl::m_type in Core/ConsoleObjectBase.h, together with the polarity
    // contradiction that ConsoleObjects::getConsoleObject exposes. Read that note before relying
    // on the meaning of 2 versus 3.
    virtual int           getType() const = 0;                // +0x14
    virtual int           getAccess() const = 0;              // +0x18
    virtual unsigned char getCategory() const = 0;            // +0x1C

    // Returns borrowed type-name strings for the result and indexed argument.
    virtual const char*   getReturnType() const = 0;          // +0x20
    virtual const char*   getArgType(int index) const = 0;    // +0x24

    virtual int           getMinNoArgs() const = 0;           // +0x28
    virtual int           getMaxNoArgs() const = 0;           // +0x2C

    // State
    // isActive() is the filter ConsoleObjects::findInMultiMap applies: a lookup walks every binding
    // sharing a key and returns the first active one, so an inactive binding is invisible to the
    // console even though it is still registered.
    virtual bool          isActive() const = 0;               // +0x30

    virtual bool          hasRange() const = 0;               // +0x34

    // Range returns two pointers by value; formatted range returns two strings, while delta is one string.
    // std::pair and erased const void* are [CHOSEN] spellings of the observed return layout.
    virtual std::pair<const void*, const void*> getRange() const = 0;         // +0x38
    virtual std::pair<std::string, std::string> getRangeAsString() const = 0; // +0x3C
    virtual std::string   getDeltaAsString() const = 0;       // +0x40
    virtual bool          hasReturnValue() const = 0;         // +0x44
    virtual bool          isDynamic() const = 0;              // +0x48
    virtual bool          isSetGet() const = 0;               // +0x4C
    virtual bool          hasBundledType() const = 0;         // +0x50
    virtual std::string   getTypeDescription() const = 0;     // +0x54

    // The call
    // Mangled _ZN4dice3hfe21ConsoleObjectBaseImpl7executeEPSsi - std::string* and int, i.e. an
    // array of arguments plus its count, not a vector.
    virtual std::string   execute(std::string* args, int argCount) = 0;   // +0x58
};

} // namespace hfe
} // namespace dice

#endif // CORE_CONSOLEOBJECT_H
