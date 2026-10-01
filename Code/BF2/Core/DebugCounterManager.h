// Debug counters and graph presentation. Header path [INFERRED].
// The x86 ELF layout is 80 bytes: IDebugCounterManager is primary and IEventListener is at +4.
// Keep the virtual destructor after graphMinMax; IEventListener has no virtual destructor.
// Use typed calls: ELF adds handleEvent to the primary table, while MSVC uses the secondary table.

#ifndef CORE_DEBUGCOUNTERMANAGER_H     // [INFERRED] guard style
#define CORE_DEBUGCOUNTERMANAGER_H

#include <string>
#include <vector>

#include "IBase.h"
#include "../Game/IEventListener.h"     // EventCategory, EventNode, IEventListener
#include "../Math/Vec3.h"               // Vec3 is a typedef (Math::Vec3), not a class - see the
                                        // note in Core/SettingsRepostitory.h before forward-
                                        // declaring it

namespace dice {
namespace hfe {

// Opaque debug-counter interface. [UNRESOLVED] original method names and full layout.
// Observed slots supply a tag, two reset phases, pre-format update, buffer formatting, indexed
// float samples and sample count. Keep the bounded slot adapters until the real interface is known.
class IDebugCounter;

// Counter-management interface with the concrete manager's fixed slot order.
// Return types except getGraphFunction remain [UNRESOLVED]; void is the current choice and affects MSVC linkage.
// Parameter names/color scale are [UNRESOLVED]; setColor truncates and packs RGB integer components.
class IDebugCounterManager : public IBase
{
public:
    virtual void reset()                            = 0;   // +0x10
    virtual void update()                           = 0;   // +0x14
    virtual void draw()                             = 0;   // +0x18
    virtual void enable(bool on)                    = 0;   // +0x1C
    virtual void setColor(const Vec3& color)        = 0;   // +0x20
    virtual void setPosition(int x, int y)          = 0;   // +0x24
    virtual void setYSpacing(int spacing)           = 0;   // +0x28
    virtual void graphEnable(const std::string& counterName) = 0;   // +0x2C
    virtual void graphDisable()                     = 0;   // +0x30
    virtual void setGraphFunction(const std::string& fn)     = 0;   // +0x34
    virtual const std::string& getGraphFunction() const      = 0;   // +0x38
    virtual void graphPos(int x, int y)             = 0;   // +0x3C
    virtual void graphSize(int width, int height)   = 0;   // +0x40
    virtual void graphMinMax(int min, int max)      = 0;   // +0x44

    // last. Slots +0x48 / +0x4C. Moving this declaration up moves eighteen slots.
    virtual ~IDebugCounterManager() {}
};

class DebugCounterManager : public IDebugCounterManager, public IEventListener
{
public:
    DebugCounterManager();

    // Primary vtable, in slot order. Do not reorder.
    virtual void addRef();                                          // +0x00
    virtual int  getRef();                                          // +0x04
    virtual int  release();                                         // +0x08
    virtual void* queryInterface(unsigned int iid) const;           // +0x0C

    virtual void reset();                                           // +0x10
    virtual void update();                                          // +0x14
    virtual void draw();                                            // +0x18
    virtual void enable(bool on);                                   // +0x1C
    virtual void setColor(const Vec3& color);                       // +0x20
    virtual void setPosition(int x, int y);                         // +0x24
    virtual void setYSpacing(int spacing);                          // +0x28
    virtual void graphEnable(const std::string& counterName);       // +0x2C
    virtual void graphDisable();                                    // +0x30
    virtual void setGraphFunction(const std::string& fn);           // +0x34
    virtual const std::string& getGraphFunction() const;            // +0x38
    virtual void graphPos(int x, int y);                            // +0x3C
    virtual void graphSize(int width, int height);                  // +0x40
    virtual void graphMinMax(int min, int max);                     // +0x44

    virtual ~DebugCounterManager();                                 // +0x48 / +0x4C


    // The IEventListener half. Secondary vtable slot 0; also +0x50 of the primary under Itanium.
    virtual void handleEvent(EventCategory category, unsigned int id,
                             EventNode* node, void* userData);

    // Static operations over the shared counter vector; no instance pointer is passed.
    // [UNRESOLVED] Original return types: pointer results and remove's bool follow observed values.
    static IDebugCounter* add(IDebugCounter* counter);
    static bool           remove(const char* name);
    static IDebugCounter* get(int index);
    static IDebugCounter* getCounterByName(const char* name);

    // Non-virtual, non-static. Reads the counter named by m_graphCounterName and reduces it to the
    // single number the line graph plots.
    float getValueFromFunction();

    // Creates the requested interface for module registration. Aggregation is unsupported.
    static IBase* create(unsigned int iid, IBase* outer);

    // Shared across all instances; destruction deletes every counter in this vector.
    static std::vector<IDebugCounter*> m_counterVec;

private:
    // Reference layout offsets are for the x86 ELF. Field names are [CHOSEN] except m_counterVec.
    /* +0x08 */ int         m_refCount;          // = 1 at construction (0x83C17C2 `mov [esi+8], 1`).
                                                 //     The one-reference rule holds here.
    /* +0x0C */ bool        m_enabled;           // = false; a byte (`mov byte ptr [esi+0Ch], 0`), and
                                                 //     draw() returns immediately when it is clear
    /* +0x10 */ int         m_color;             // = 0;  packed x<<16 | y<<8 | z, see setColor
    /* +0x14 */ int         m_posX;              // = 0
    /* +0x18 */ int         m_posY;              // = 0
    /* +0x1C */ int         m_ySpacing;          // = 0
    /* +0x20 */ std::string m_graphCounterName;  // = ""; empty means "no graph" - draw() tests it
    /* +0x24 */ void*       m_graph;             // = 0;  made by the DebugRenderer singleton
    /* +0x28 */ void*       m_graphRenderer;     // = 0;  the DebugLineGraphRenderer singleton
    /* +0x2C */ std::string m_graphFunction;     // = "0" - the literal string "0", not the number.
                                                 //     getValueFromFunction reads its first
                                                 //     character as a sample index
    /* +0x30 */ int         m_graphYOffset;      // = 0;  cache - draw only re-lays the graph out
                                                 //     when the computed offset changes
    /* +0x34 */ int         m_graphPosX;         // = 0
    /* +0x38 */ int         m_graphPosY;         // = 0
    /* +0x3C */ int         m_graphWidth;        // = 400  (0x190)
    /* +0x40 */ int         m_graphHeight;       // = 200  (0xC8)
    /* +0x44 */ int         m_graphMin;
    /* +0x48 */ int         m_graphMax;          // = 100  (0x64)
    /* +0x4C */ char*       m_textBuffer;        // = 0, then `new char[1024]` at the very end of the
                                                 //     constructor. One row of formatted text; the
                                                 //     counters write into it through their +0x10.
};

} // namespace hfe
} // namespace dice

#endif // CORE_DEBUGCOUNTERMANAGER_H
