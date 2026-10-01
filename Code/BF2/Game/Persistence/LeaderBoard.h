// Path: [INFERRED] persistence cluster. ELF 0x88C6D08 / PE 0x7E0588 primary tables.
#ifndef GAME_PERSISTENCE_LEADERBOARD_H
#define GAME_PERSISTENCE_LEADERBOARD_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
namespace dice { namespace hfe {
class ILeaderBoard : public IBase
{
public:
    virtual bool getLeaderBoard(const std::string&, bool) = 0;
    virtual bool getLeaderBoardPlayer(int, const std::string&, bool) = 0;
    virtual int getUpdated() = 0;
    virtual int getNumPlayers() = 0;
protected:
    ~ILeaderBoard() {}
};
class LeaderBoard : public ILeaderBoard, public HTTPRequest
{
public:
    LeaderBoard();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool getLeaderBoard(const std::string&, bool) override;
    bool getLeaderBoardPlayer(int, const std::string&, bool) override;
    int getUpdated() override;
    int getNumPlayers() override;
    ~LeaderBoard() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
    int getNumKeyMaps();
    KeyMap* getKeyMap(unsigned int);
private:
    int m_refCount;
    KeyParser m_parser;
    int m_updated;
    int m_numPlayers;
};
} }
#endif
