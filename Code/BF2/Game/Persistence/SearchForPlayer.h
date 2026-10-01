// Path: [INFERRED] beside attested SearchForPlayer.cpp.
// ELF primary 0x88C8688; PE 0x7E04E8. The interface has no destructor slot.
#ifndef GAME_PERSISTENCE_SEARCHFORPLAYER_H
#define GAME_PERSISTENCE_SEARCHFORPLAYER_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
#include <vector>
namespace dice { namespace hfe {
std::string encodeString(const std::string&);
class ISearchForPlayer : public IBase
{
public:
    struct Player { int profileId; std::string nick; int score; }; // [INFERRED] field names
    virtual bool search(const std::string&, bool, bool) = 0;
    virtual void tick() = 0;
    virtual bool getPlayers(std::vector<Player>&) = 0;
protected:
    ~ISearchForPlayer() {}
};
class SearchForPlayer : public ISearchForPlayer, public HTTPRequest
{
public:
    SearchForPlayer();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool search(const std::string&, bool, bool) override;
    void tick() override;
    bool getPlayers(std::vector<Player>&) override;
    ~SearchForPlayer() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
private:
    int m_refCount;
    KeyParser m_parser;
};
} }
#endif
