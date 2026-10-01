// GameSpy-backed server browser.
#ifndef MENU_SERVERBROWSER_H
#define MENU_SERVERBROWSER_H
#include "IServerBrowser.h"
#if !DEDICATED

struct _ServerBrowser;   // the SDK handle (sb_serverbrowsing.h: `typedef struct _ServerBrowser *ServerBrowser;`)

namespace dice { namespace hfe {

class ServerBrowser : public ui::IServerBrowser
{
public:
    ServerBrowser();
    virtual ~ServerBrowser();                                               // +0x64  0x485880 (deleting), body 0x4852A0
    static IBase* create(unsigned int iid, IBase* outer);

    void  addRef() override;                                                // +0x00
    int   getRef() override;                                                // +0x04
    int   release() override;                                               // +0x08  0x68AB00: at zero, the +0x64 destructor with 1
    void* queryInterface(unsigned int iid) const override;                  // +0x0C  0x485270: 1 and 121034

    bool  init(bool internet) override;                                     // +0x10
    void  shutdown() override;                                              // +0x14
    void  update() override;                                                // +0x18
    bool  isBrowsing() override;                                            // +0x1C
    ui::ServerBrowserRecordList* getRecords() override;                     // +0x20
    void  reset() override;                                                 // +0x24
    bool  startUpdate(bool async) override;                                 // +0x28
    bool  requeueRecords() override;                                        // +0x2C
    void  stopUpdate() override;                                            // +0x30
    bool  isInternet() override;                                            // +0x34
    void  sort(const std::string& key, bool ascending, int stringCompare) override;             // +0x38
    ui::ServerBrowserRecord* getRecord(const std::string& address, int port, bool full, bool useCached) override;   // +0x3C
    bool  fillRecord(SBServer server, ui::ServerBrowserRecord* out, bool full, bool useCached) override;            // +0x40
    bool  fillLocalRecord(ui::ServerBrowserRecord* out) override;           // +0x44
    bool  queryAddress(const std::string& address, int port, bool viaMaster) override;         // +0x48
    bool  removeAddress(const std::string& address, int port) override;    // +0x4C
    std::string& getFilter() override;                                      // +0x50
    void  setFilter(const std::string& filter) override;                    // +0x54
    SBServer getServer(const std::string& address, int port) override;     // +0x58
    void  setMaxPing(int maxPing) override;                                 // +0x5C
    void  setKeepUnansweredServers(bool keep) override;                     // +0x60

    // The SDK callback (0x485BC0, a free function in ServerBrowser.cpp) dispatches here by
    // reason; the four handlers are 0x485B30, 0x4859C0, 0x4855C0 and 0x482580. Not a slot.
    static void dispatchCallback(_ServerBrowser* browser, int reason, SBServer server, void* instance);

private:
    friend struct ServerBrowserFixture;   // [TEMP] Code/Tests/client_startup_runtime.cpp reads the fields

    void  onServerAdded(SBServer server);
    void  onServerUpdated(SBServer server);
    void  onServerDeleted(SBServer server);
    void  onUpdateComplete();

    int                          m_refCount;               // +0x04
    ui::ServerBrowserRecordList  m_results;                // +0x08  the listed servers; reserve(4000) at construction
    ui::ServerBrowserRecordList  m_pending;                // +0x20  servers queued for a query; reserve(4000)
    _ServerBrowser*              m_browser;                // +0x38  the GameSpy handle, null until init
    bool                         m_internet;               // +0x3C  init's argument: internet (master list) or LAN
    bool                         m_suppressCallbacks;      // +0x3D  raised around the synchronous queries and lookups
    std::string                  m_sortKey;                // +0x40  the last sort key (empty: none yet)
    int                          m_sortStringCompare;      // +0x5C  the last sort's compare mode argument
    bool                         m_sortAscending;          // +0x60
    std::string                  m_filter;                 // +0x64  the master-list filter startUpdate sends
    bool                         m_updateStarted;          // +0x80  raised by startUpdate, cleared by the update-complete / query-error callbacks
    int                          m_maxPing;                // +0x84  mirrors m_results.maxPing
    bool                         m_keepUnansweredServers;  // +0x88  list servers without basic keys (favourites, history)
};

} }
#endif // !dedicated
#endif
