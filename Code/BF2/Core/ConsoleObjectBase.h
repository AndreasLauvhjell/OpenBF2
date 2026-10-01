// Base for generated console bindings. Header path [INFERRED].
// Public accessors delegate type-specific work to virtual objectXxx hooks.
// Keep execute last among the ConsoleObject slots; the delta hook precedes the range hook,
// reversing the order of the public range/delta accessors.

#ifndef CORE_CONSOLEOBJECTBASE_H     // [INFERRED] guard style
#define CORE_CONSOLEOBJECTBASE_H

#include <string>

#include "ConsoleObject.h"

namespace dice {
namespace hfe {

class ConsoleObjectBaseImpl : public ConsoleObject
{
public:
    virtual ~ConsoleObjectBaseImpl();                        // +0x00 / +0x04

    // ConsoleObject overrides in ABI order. Accessors read binding metadata unless noted.
    virtual void*         getObject() const;                 // +0x08 0x83be770  base returns 0
    virtual const char*   getObjectName() const;             // +0x0C / +0x08
    virtual const char*   getFunctionName() const;           // +0x10 / +0x0C

    // Returns the binding kind: 0 method, 1 read/write, 2 read-only, 3 write-only.
    // Property reads use zero arguments; writes use one. ConsoleObjects' get/set prefix lookup
    // uses the inverse type-2/type-3 tests; preserve that distinction from command execution.
    virtual int           getType() const;                   // +0x14 / +0x04

    virtual int           getAccess() const;                 // +0x18 / +0x10
    virtual unsigned char getCategory() const;               // +0x1C 0x83be7c0  -> +0x14 (byte)

    // Returns borrowed type-name strings for the result and indexed argument.
    virtual const char*   getReturnType() const;             // +0x20 / +0x4C
    virtual const char*   getArgType(int index) const;       // +0x24 0x83be7e0  -> +0x50 + index*4

    virtual int           getMinNoArgs() const;              // +0x28 / +0x18
    virtual int           getMaxNoArgs() const;              // +0x2C / +0x1C

    // These seven dispatch to the objectXxx() hooks further down; the bodies are 24-44 bytes and do
    // nothing but forward. ConsoleObjects::findInMultiMap calls isActive() through slot +0x30 to
    // skip inactive bindings during lookup.
    // range signatures both return pairs by value (sret). The first real
    // ranged bindings prove that the first pair contains pointers to the generated object's
    // mutable range-bound members, not float values; see Core/ConsoleObject.h.
    virtual bool          isActive() const;                  // +0x30
    virtual bool          hasRange() const;                  // +0x34
    virtual std::pair<const void*, const void*> getRange() const; // +0x38
    virtual std::pair<std::string, std::string> getRangeAsString() const;  // +0x3C
    virtual std::string   getDeltaAsString() const;          // +0x40

    virtual bool          hasReturnValue() const;            // +0x44 0x83be990  -> +0xA0 (byte)
    virtual bool          isDynamic() const;                 // +0x48 0x83be9a0  -> +0xA1 (byte)

    // Returns whether getType() identifies a set/get property (value one).
    virtual bool          isSetGet() const;                  // +0x4C

    virtual bool          hasBundledType() const;            // +0x50
    virtual std::string   getTypeDescription() const;        // +0x54

    // Executes the binding: converts the arguments, calls the C++ function, formats the result.
    // last interface slot, not first - see the ordering warning in the header block.
    virtual std::string   execute(std::string* args, int argCount);   // +0x58
    // Generated bindings override these default hooks. Preserve their virtual declaration order.
    virtual bool        isObjectActive() const;               // +0x5C 0x83be810  stub -> 0
    virtual void        executeObjectMethod();                // +0x60 0x83bea60  stub -> 0
    virtual std::string getReturnValueAsString() const;       // +0x64
    virtual void        setArgFromString(int index,
                                         const std::string&); // +0x68 0x83beaa0  stub, empty (5 bytes)
    virtual bool        objectHasRange() const;               // +0x6C 0x83be840  stub -> 0
    virtual std::pair<const void*, const void*> getObjectRange() const; // +0x70
    virtual std::string getObjectDeltaAsString() const;       // +0x74
    virtual std::pair<std::string, std::string> getObjectRangeAsString() const; // +0x78
    virtual void        checkObjectRange(bool);               // +0x7C 0x83beab0  stub -> 0
    virtual bool        objectHasBundledType() const;         // +0x80 0x83be9d0  stub -> 0
    virtual std::string getObjectTypeDescription() const;     // +0x84

protected:
    // Offsets proven from the accessor bodies above.
    /* 0x00 */ // vptr - implicit, supplied by the ConsoleObject base
    /* 0x04 */ int           m_type;            // 1 == set/get property (see isSetGet)
    /* 0x08 */ const char*   m_objectName;
    /* 0x0C */ const char*   m_functionName;
    /* 0x10 */ int           m_access;          // vs the caller's AccessType
    /* 0x14 */ unsigned char m_category;
    /* 0x18 */ int           m_minNoArgs;
    /* 0x1C */ int           m_maxNoArgs;

    // The following four were recovered from execute() (0x83be630) rather than from accessors,
    // and they close the layout completely - every byte from 0x00 to 0xA1 is now accounted for.
    /* 0x20 */ int           m_argCount;        // set by execute() before conversion
    /* 0x24 */ std::string*  m_args[10];        // raw pointers, used when no conversion is needed

    // Borrowed C++ type-name strings used to describe the result and argument types.
    /* 0x4C */ const char*   m_returnType;      // e.g. "bool"
    /* 0x50 */ const char*   m_argTypes[10];    // e.g. "std::string", "float", "int"

    // One flag per argument, computed in the generated constructor as
    //     m_argNeedsConversion[i] = (m_argTypes[i] == "std::string") ? 0 : -1;
    // i.e. std::string arguments are handed to the target RAW, everything else is routed through
    // the generated setArgFromString override. That is exactly the branch execute() takes.
    /* 0x78 */ int           m_argNeedsConversion[10];
    /* 0xA0 */ bool          m_hasReturnValue;
    /* 0xA1 */ bool          m_isDynamic;
    // Ten entries in each array matches ObjectWrapper::executeBinary's fixed std::string args[10],
    // so the 10-argument ceiling is consistent on both sides of the binding.
};

// Sets defaults inherited by subsequent console bindings in this translation unit.
// Stores access and category and a lowercase-first spelling of objectName; names are limited to 31 characters.
class SetConsoleClassDefaults
{
public:
    SetConsoleClassDefaults(const char* objectName, unsigned int access, unsigned char category);
};

} // namespace hfe
} // namespace dice

#endif // CORE_CONSOLEOBJECTBASE_H
