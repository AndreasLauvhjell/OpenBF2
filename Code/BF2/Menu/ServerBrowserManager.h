// Client server-browser manager singleton. [INFERRED] Header placement.
#ifndef MENU_SERVERBROWSERMANAGER_H
#define MENU_SERVERBROWSERMANAGER_H
#include "IServerBrowserManager.h"
#include <map>
#if !DEDICATED
namespace dice { namespace hfe {

class ServerBrowserManager : public ui::IServerBrowserManager
{
public:
    ServerBrowserManager();
    virtual ~ServerBrowserManager();                                        // +0x28  0x4867D0: clearLists, then the map
    static IBase* create(unsigned int iid, IBase* outer);

    void  addRef() override;                                                // +0x00
    int   getRef() override;                                                // +0x04
    int   release() override;                                               // +0x08
    void* queryInterface(unsigned int iid) const override;                  // +0x0C  0x486740: 1 and 121063

    unsigned int getListCount() override;                                   // +0x10
    ui::IServerBrowser* getList(const std::string& name) override;          // +0x14
    bool  createList(const std::string& name, bool internet) override;      // +0x18
    void  clearLists() override;                                            // +0x1C
    void  update() override;                                                // +0x20
    bool  isAnyBrowsing() override;                                         // +0x24

private:
    int                                          m_refCount;   // +0x04
    std::map<std::string, ui::IServerBrowser*>   m_lists;      // +0x08  by name, in name order
};

} }
#endif // !dedicated
#endif
