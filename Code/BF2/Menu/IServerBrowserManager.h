// Manages named server-browser lists.
#ifndef MENU_ISERVERBROWSERMANAGER_H
#define MENU_ISERVERBROWSERMANAGER_H
#include "../Core/IBase.h"
#include <string>
#if !DEDICATED
namespace dice { namespace hfe { namespace ui {

struct IServerBrowser;

struct IServerBrowserManager : IBase
{
    virtual unsigned int getListCount() = 0;                                    // +0x10  0x68ABC0  the map's size
    virtual IServerBrowser* getList(const std::string& name) = 0;               // +0x14  0x486110  by name, null when absent
    virtual bool createList(const std::string& name, bool internet) = 0;        // +0x18  0x4866C0  a dice.hfe.ServerBrowser, init(internet), stored by name
    virtual void clearLists() = 0;                                              // +0x1C  0x4865B0  every list freed (no destructor), the map emptied
    virtual void update() = 0;                                                  // +0x20  0x486140  update() on every list that isBrowsing()
    virtual bool isAnyBrowsing() = 0;                                           // +0x24
};

} // namespace ui
} }
#endif // !dedicated
#endif
