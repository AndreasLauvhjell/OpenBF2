// HTTP request interface for persistence services.
#ifndef GAME_PERSISTENCE_HTTPREQUEST_H
#define GAME_PERSISTENCE_HTTPREQUEST_H
#include <string>
namespace dice { namespace hfe {
class HTTPRequest {
public:
    HTTPRequest();
    virtual ~HTTPRequest();
    // ELF dtor pair +0/+4, PE single dtor +0. getFile completion precedes post.
    virtual void onGetComplete(const std::string&);
    virtual void onGetFailure(const std::string&);
    virtual void onGetFileComplete();
    virtual void onPostComplete(const std::string&);
    virtual void onPostFailure(const std::string&);
    virtual void onProgress(float);
    bool get(const std::string& url, bool asynchronous);
    bool post(const std::string& url, const std::string& body, bool asynchronous);
    bool getFile(const std::string& url, const std::string& file, bool asynchronous);
    void update();
    bool isUpdating();
    bool cancel();
    void setProxy(const std::string& proxy);
    std::string getGameSpyUrl() const;
    bool validateUrlNotSpoofed(std::string host) const;
    int getRevision();
    /* +0x04 */ bool m_getPending;
    /* +0x05 */ bool m_postPending;
    /* +0x08 */ int m_ghttpHandleA;
    /* +0x0C */ void* m_postData;
    /* +0x10 */ int m_ghttpHandleB;
    /* +0x14 */ int m_revision;
};
} }
#endif
