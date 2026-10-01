// Loads and manages server logos for menu display.
#ifndef MENU_SERVERLOGOMANAGER_H
#define MENU_SERVERLOGOMANAGER_H
#include "../Core/IBase.h"
#include "../Game/Persistence/HTTPRequest.h"
#include <string>
#if !DEDICATED
namespace dice { namespace hfe {

// Every slot of the primary table 8A9230 after IBase's four. Slot names are [INFERRED] from the
// bodies and their two callers (SwiffRender's http arm and the SwiffPlayer row).
class IServerLogoManager : public IBase {
public:
    virtual bool init() = 0;                                                            //+10 5A5E20  the shared constant-true stub
    virtual void shutdown() = 0;                                                        //+14 83D270  nullsub
    virtual bool parseUrl(const std::string& url, std::string& directory, std::string& file) = 0;
    virtual bool getCachedPath(const std::string& url, std::string& out) = 0;           //+1C 4C33D0
    virtual bool download(const std::string& url) = 0;
};

class ServerLogoManager final : public IServerLogoManager, public HTTPRequest {
public:
    ServerLogoManager();                                       //the HTTPRequest base first (59C390), then a refcount of 1
    ~ServerLogoManager();                                      //the two tables reset, then the base's 59C3C0
    static IBase* create(unsigned int iid, IBase* outer);      //the settings-factory shape, 32 retail bytes

    void  addRef() override;
    int   getRef() override;
    int   release() override;
    void* queryInterface(unsigned int iid) const override;     //IBase (1), then IServerLogoManager (121067)

    bool init() override;
    void shutdown() override;
    bool parseUrl(const std::string& url, std::string& directory, std::string& file) override;
    bool getCachedPath(const std::string& url, std::string& out) override;
    bool download(const std::string& url) override;

private:
    int m_refCount;                                            // +0x1C
};

} }
#endif // !dedicated
#endif // MENU_SERVERLOGOMANAGER_H
