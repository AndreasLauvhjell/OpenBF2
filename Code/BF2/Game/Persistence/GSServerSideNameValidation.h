// Path: [INFERRED] beside the attested persistence request sources.
// ELF primary 0x88C6608 / HTTP secondary 0x88C6644; PE 0x7CD954 / 0x7CD938.
// Native STL layout is used. Retail sizes are ELF 0x4C, PE 0xB0, NOT the old 0x44.
#ifndef GAME_PERSISTENCE_GSSERVERSIDENAMEVALIDATION_H
#define GAME_PERSISTENCE_GSSERVERSIDENAMEVALIDATION_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
namespace dice { namespace hfe {
class GameServer;
class IGSServerSideNameValidation : public IBase
{
public:
    virtual bool validateSoldierName(const std::string&, const std::string&, int,
                                    const std::string&, int, GameServer*) = 0;
    virtual int getNumKeyMaps() = 0;
    virtual int getNumKeys() = 0;
    virtual bool getKey(unsigned int, int, std::string&, std::string&) = 0;
    virtual std::string getKeyValue(unsigned int, const std::string&) = 0;
protected:
    ~IGSServerSideNameValidation() {}
};
class GSServerSideNameValidation : public IGSServerSideNameValidation, public HTTPRequest
{
public:
    GSServerSideNameValidation();
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool validateSoldierName(const std::string&, const std::string&, int,
                            const std::string&, int, GameServer*) override;
    int getNumKeyMaps() override;
    int getNumKeys() override;
    bool getKey(unsigned int, int, std::string&, std::string&) override;
    std::string getKeyValue(unsigned int, const std::string&) override;
    ~GSServerSideNameValidation() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    KeyMap* getKeyMap(unsigned int);
    static int outstandingRequests;
private:
    int m_refCount;
    KeyParser m_parser;
    int m_connectionId;
    std::string m_soldierNick;
    std::string m_clanTag;
    std::string m_scratch;
    int m_playerId;
    std::string m_gsAuth;
    double m_requestTime; // [INFERRED] name. ELF +44 / PE +A8, written before GET.
};
} }
#endif
