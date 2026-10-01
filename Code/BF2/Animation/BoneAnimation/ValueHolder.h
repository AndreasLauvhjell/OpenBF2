// [INFERRED] Source placement.

// Process-wide registry of animation value holders.


#ifndef ANIMATION_BONEANIMATION_VALUEHOLDER_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_VALUEHOLDER_H

#include <map>
#include <string>

#include "../../IO/File/IFileSystem.h"     // hfe::NoCaseStringCompare - the map's comparator

namespace dice {

namespace hfe { namespace io { class IStream; } }

namespace anim {


// Script-configured animation value and interpolation settings.

class ValueHolder
{
public:
    ValueHolder();

    // Writes creation, values, then optional message masks.
    void makeScript(hfe::io::IStream* stream);

    // Public for the same reason CameraSpring's fields are: the ELF's console executors store
    // straight into these offsets with no accessor call anywhere in the binary.
    /* +0x00 */ float min;                    // ConsoleClass176
    /* +0x04 */ float max;                    // ConsoleClass178  +0x04, not +0x08
    /* +0x08 */ float normal;                 // ConsoleClass177
    /* +0x0C */ unsigned int stopOnMessage;   // ConsoleClass180
    /* +0x10 */ unsigned int passOnMessage;   // ConsoleClass179
};

class ValueHolderManager
{
public:
    ValueHolderManager();
    ~ValueHolderManager();

    // once PER process, and the flag is SET before the read. Called by
    // AnimationSystemTemplate::loadScript at the top of every script load; the second and later
    // calls do nothing at all.
    void loadValueHolders();
    void saveValueHolders();

    // Returns the shared value-holder filename.
    static const std::string& getFilename();

    ValueHolder* createValueHolder(const std::string& name);
    ValueHolder* get(const std::string& name);
    void setActiveByName(const std::string& name);
    const std::string& getActiveByName();
    const std::string& findName(ValueHolder* holder);

    std::string listValueHolders() const;

    // Selects the active value holder. [CHOSEN] Accessor name.
    void setActive(ValueHolder* holder) { m_activeValueHolder = holder; }
    // Same reasoning: ModuleBoneAnimation's file-local getActiveValueHolder() reads +0x0C
    // directly from another TU, which is what an inlined getter compiles to.
    ValueHolder* getActive() const { return m_activeValueHolder; }

private:
    /* +0x00 */ std::map<std::string, ValueHolder*, hfe::NoCaseStringCompare> m_valueHolders;
    /* +0x0C */ ValueHolder* m_activeValueHolder;                 // ctor 0
    /* +0x10 */ bool m_valueHoldersLoaded;                        // ctor false. [CHOSEN] name
};

// Created directly by AnimModule::init.
extern ValueHolderManager* valueHolderManager;

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_VALUEHOLDER_H
