// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-VOIP-FIXTURE]
// Tests HudVoip layout, row storage, update gates, and painting with real registry/list data.
// Unexpected player/squad slots abort; the voice client remains null.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <functional>
#include <list>
#include <new>
#include <string>
#include <vector>
// HudVoip.h is included without the private-is-public trick on purpose: MSVC
// encodes access in the mangled name, so calling its private paintRow from a TU
// that saw it as public produces an unresolved symbol, which /force turns into
// RVA 0 and a silent crash. Its members are reached through the friend struct
// the header already declares.
#include "../BF2/Menu/HudVoip.h"
#define private public
#define protected public
#include "../BF2/Menu/BfListBoxData.h"
#include "../BF2/Menu/BF2HudManager.h"
#undef protected
#undef private
#include "../BF2/Menu/HudColors.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/StringUtil.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Game/GameLogic/SquadManager/ISquadManager.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"

[[noreturn]] static void fixtureUnsupported(const char* name) {
    std::fprintf(stderr, "[FRONTIER] VoIP fixture unexpected %s\n", name);
    std::fflush(stderr); std::abort();
}
[[noreturn]] static void actionUnsupported(const char* name) { fixtureUnsupported(name); }
#include "hud_kit_observers.h"

namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IBase* g_hudInformationLayer = nullptr;
IGameLogic* gameLogic = nullptr;
IBase* squadManager = nullptr;
hud::BF2HudManager* hudManager = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
// Constructing a PlayerManager constructs this second, unrelated singleton and
// stores it here (PlayerManager.cpp). Without the definition the /force link
// resolves the store to address 0 and the fixture dies with no output at all.
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }

using namespace dice::hfe;
using namespace dice::hfe::hud;
using dice::meme::BfListBoxData;
using dice::meme::ListBoxCell;

namespace dice { namespace hfe { namespace hud {
struct HudVoipFixture {
    static void paint(HudVoip& v, meme::BfListBoxData* l, world::IPlayer* p) { v.paintRow(l, p); }
    static meme::System*& system(HudVoip& v)      { return v.m_system; }
    static int& visibleRows(HudVoip& v)           { return v.m_visibleRows; }
    static unsigned& field008(HudVoip& v)         { return v.m_field008; }
    static unsigned& field010(HudVoip& v)         { return v.m_field010; }
    static bool& visible(HudVoip& v)              { return v.m_visible; }
    static bool& enabled(HudVoip& v)              { return v.m_enabled; }
    static int& rowHeight(HudVoip& v)             { return v.m_rowHeight; }
    static int& speakerWidth(HudVoip& v)          { return v.m_speakerColumnWidth; }
    static int& nameWidth(HudVoip& v)             { return v.m_nameColumnWidth; }
    static float& field024(HudVoip& v)            { return v.m_field024; }
    static int& emptyTexture(HudVoip& v)          { return v.m_emptyTexture; }
    static int& speakerTexture(HudVoip& v)        { return v.m_speakerTexture; }
    static int& commanderTexture(HudVoip& v)      { return v.m_commanderTexture; }
    static int* squadTextures(HudVoip& v)         { return v.m_squadTextures; }
    static meme::BfListBoxData*& data(HudVoip& v) { return v.m_data; }
};
} } }
using dice::hfe::hud::HudVoipFixture;

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

// the world
class PlainSystem : public dice::meme::System
{
public:
    explicit PlainSystem(dice::meme::NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    dice::meme::TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadTexture(const char* name, bool menuPath) override
    {
        textures.push_back(std::string(name));
        menuFlags.push_back(menuPath);
        return 500 + static_cast<int>(textures.size());
    }
    std::vector<std::string> textures;
    std::vector<bool> menuFlags;
};

struct VoipPlayer : world::PlayerBoundary {
    int id = 0, team = 1, squad = 0;
    bool alive = true, commander = false, squadLeader = false;
    bool voip = true, muted = false, talking = false;
    std::string name = "player";
    int getId() const override { return id; }
    bool getIsAlive() const override { return alive; }
    int getTeam() const override { return team; }
    int getSquadId() const override { return squad; }
    bool getIsCommander() const override { return commander; }
    bool getIsSquadLeader() const override { return squadLeader; }
    bool getVoipEnabled() override { return voip; }
    bool getIsMuted() override { return muted; }
    bool getIsTalking() override { return talking; }
    const std::string& getName() const override { return name; }
    // PlayerBoundary leaves these four to its user; nothing here calls them.
    int getKit() const override { fixtureUnsupported("getKit"); }
    int getChosenUnlockLevel(int, int) const override { fixtureUnsupported("getChosenUnlockLevel"); }
    void setChosenUnlockLevel(int, int, int) override { fixtureUnsupported("setChosenUnlockLevel"); }
    bool isUnlocked(int, int, int) override { fixtureUnsupported("isUnlocked"); }
};

struct VoipPlayers : world::PlayerManager {
    world::IPlayer* local = nullptr;
    world::IPlayer* byId = nullptr;
    int lastRequestedId = -12345;
    int lastSquadTeam = -1, lastSquadId = -1, lastLeaderTeam = -1;
    std::list<world::IPlayer*> squadMembers, squadLeaders;
    world::IPlayer* getLocalHumanPlayer() override { return local; }
    world::IPlayer* getPlayerFromId(int id) override
    { lastRequestedId = id; return byId; }
    const std::list<world::IPlayer*>& getPlayersInSquad(int team, int squad) override
    { lastSquadTeam = team; lastSquadId = squad; return squadMembers; }
    const std::list<world::IPlayer*>& getTeamSquadLeaders(int team) override
    { lastLeaderTeam = team; return squadLeaders; }
};

// The real 34-slot interface; only getCommander is exercised.
struct VoipSquads : ISquadManager {
    int commanderId = 7;
    int lastTeam = -1;
    int getCommander(int team) override { lastTeam = team; return commanderId; }
    void addRef() override { fixtureUnsupported("squad addRef"); }
    int getRef() override { fixtureUnsupported("squad getRef"); }
    int release() override { fixtureUnsupported("squad release"); }
    void* queryInterface(unsigned int) const override { fixtureUnsupported("squad queryInterface"); }
    void reset() override { fixtureUnsupported("reset"); }
    void reAddDynamicSpawnGroups() override { fixtureUnsupported("reAddDynamicSpawnGroups"); }
    void sendSquadManagerState(int) override { fixtureUnsupported("sendSquadManagerState"); }
    bool setSquadName(int, int, int, const std::string&, bool) override { fixtureUnsupported("setSquadName"); }
    void setIsPrivate(int, int, bool, bool) override { fixtureUnsupported("setIsPrivate"); }
    bool setIsAccepted(int, int, bool, OrderType, SquadOrderType, bool) override { fixtureUnsupported("setIsAccepted"); }
    ISquadInfo* getSquadInfo(int, int) override { fixtureUnsupported("getSquadInfo"); }
    void inviteToSquad(int, int, int, bool, bool) override { fixtureUnsupported("inviteToSquad"); }
    bool addToSquad(int, int, int) override { fixtureUnsupported("addToSquad"); }
    void removeFromSquad(int, int, int, bool) override { fixtureUnsupported("removeFromSquad"); }
    void setSquadLeader(int, int, int, bool) override { fixtureUnsupported("setSquadLeader"); }
    int getSquadLeader(int, int) override { fixtureUnsupported("getSquadLeader"); }
    int getFirstUnusedSquad(int) const override { fixtureUnsupported("getFirstUnusedSquad"); }
    void issueSquadOrder(int, int, SquadOrder&, bool) override { fixtureUnsupported("issueSquadOrder"); }
    void issueCommanderOrder(int, int, SquadOrder&, bool) override { fixtureUnsupported("issueCommanderOrder"); }
    void issueRequest(int, int, SquadOrder&, bool) override { fixtureUnsupported("issueRequest"); }
    void resetSquad(int, int) override { fixtureUnsupported("resetSquad"); }
    void findNewSquadLeader(int, int, int) override { fixtureUnsupported("findNewSquadLeader"); }
    void applyForCommander(int, int) override { fixtureUnsupported("applyForCommander"); }
    int popNextCommanderApplicant(int) override { fixtureUnsupported("popNextCommanderApplicant"); }
    void acceptCommander(int, int) override { fixtureUnsupported("acceptCommander"); }
    void declineCommander(int, int) override { fixtureUnsupported("declineCommander"); }
    void resignCommander(int) override { fixtureUnsupported("resignCommander"); }
    void setCommander(int, int) override { fixtureUnsupported("setCommander"); }
    void* getCommanderCam(int) override { fixtureUnsupported("getCommanderCam"); }
    void setCommanderToSoldierMode(int) override { fixtureUnsupported("setCommanderToSoldierMode"); }
    void setCommanderToOverheadCamMode(int) override { fixtureUnsupported("setCommanderToOverheadCamMode"); }
    void setCommanderOverheadCamZoomMode(int, bool) override { fixtureUnsupported("setCommanderOverheadCamZoomMode"); }
    void removeCommanderApplicant(int, int) override { fixtureUnsupported("removeCommanderApplicant"); }
    void addOrderMarker(int, SquadOrder&) override { fixtureUnsupported("addOrderMarker"); }
    void removeAllOrderMarkers() override { fixtureUnsupported("removeAllOrderMarkers"); }
    void removeOrderMarker(int) override { fixtureUnsupported("removeOrderMarker"); }
    void updateOrderMarkers(world::IPlayer*) override { fixtureUnsupported("updateOrderMarkers"); }
};

// paintRow dereferences the HUD singleton with NO null test, so the fixture
// has to supply a real one. Only the VO help slot is observed; the real
// constructor runs and the cleanup below is constructor-only, as in the other
// HUD component fixtures.
struct ObserverHud : BF2HudManager {
    int lastVoMessage = -1;
    int voCalls = 0;
    void enableVOHelpMessage(int id) override { lastVoMessage = id; ++voCalls; }
};
static ObserverHud* observerHud;

static void destroyConstructorOnly(ObserverHud* hud)
{
    delete hud->m_field2dc;
    using S = std::string; using W = std::wstring;
    using L = std::list<int>; using SL = std::list<std::string>;
    using V = std::vector<HudPendingRequest>;
    hud->m_field33c.~SL(); hud->m_field32c.~V(); hud->m_field304.~S();
    hud->m_field2e0.~SL(); hud->m_field0e8.~L(); hud->m_field0c0.~S();
    hud->m_field08c.~S(); hud->m_field060.~W(); hud->m_field038.~W(); hud->m_field010.~W();
    ::operator delete(hud);
}

// the class
static void layoutCases()
{
    CHECK(sizeof(HudVoip) == 0x60);
    dice::meme::NameTreeRoot registry;
    PlainSystem system(registry);
    {
        HudVoip voip(&system);
        CHECK(HudVoipFixture::system(voip) == &system);
        CHECK(HudVoipFixture::visibleRows(voip) == 9);
        CHECK(HudVoipFixture::speakerWidth(voip) == 15 && HudVoipFixture::nameWidth(voip) == 30);
        CHECK(HudVoipFixture::field024(voip) == -1.0f);
        CHECK(HudVoipFixture::rowHeight(voip) == 0 && HudVoipFixture::field008(voip) == 0 && HudVoipFixture::field010(voip) == 0);
        CHECK(!HudVoipFixture::visible(voip) && !HudVoipFixture::enabled(voip));
        CHECK(HudVoipFixture::data(voip) == nullptr);
        for (int i = 0; i < 10; ++i) CHECK(HudVoipFixture::squadTextures(voip)[i] == 0);
        // The three loads happen in the order commander, speaker, empty.
        CHECK(system.textures.size() == 3);
        CHECK(system.textures[0] == "Ingame/Kits/Icons/kit_Commander.tga");
        CHECK(system.textures[1] == "Ingame/Respawn/icon_speaker.tga");
        CHECK(system.textures[2] == "Ingame/GeneralIcons/empty.tga");
        CHECK(system.menuFlags[0] && system.menuFlags[1] && system.menuFlags[2]);
        CHECK(HudVoipFixture::commanderTexture(voip) == 501 && HudVoipFixture::speakerTexture(voip) == 502
              && HudVoipFixture::emptyTexture(voip) == 503);
        CHECK(voip.registerFunctions());
    }
    {   // No System: all three handles stay zero and nothing is loaded.
        const std::size_t before = system.textures.size();
        HudVoip voip(nullptr);
        CHECK(system.textures.size() == before);
        CHECK(HudVoipFixture::commanderTexture(voip) == 0 && HudVoipFixture::speakerTexture(voip) == 0
              && HudVoipFixture::emptyTexture(voip) == 0);
    }
}

static void registerDataCases()
{
    dice::meme::NameTreeRoot registry;
    PlainSystem system(registry);
    HudVoip voip(&system);
    CHECK(voip.registerData());
    BfListBoxData* list = HudVoipFixture::data(voip);
    CHECK(list != nullptr);
    CHECK(std::strcmp(list->getName(), "voipList") == 0);
    // three columns, not two: BfListBoxData's own constructor adds a
    // width-0 column at index 0 before registerData adds its two.
    CHECK(list->m_columns.size() == 3);
    CHECK(list->m_columnCount == 3);
    CHECK(list->m_columns[0] && list->m_columns[0]->width == 0);
    CHECK(list->m_columns[1] && list->m_columns[1]->width == 15);
    CHECK(list->m_columns[2] && list->m_columns[2]->width == 30);
    CHECK(list->m_visibleRows == 9);       // the default 20 is overwritten
    // The destructor is a vtable store and nothing else: the list leaks.
    BfListBoxData* survivor = HudVoipFixture::data(voip);
    voip.~HudVoip();
    CHECK(HudVoipFixture::data(voip) == survivor);
    ::operator delete(survivor);
    new(&voip) HudVoip(nullptr);            // leave the object destructible again
}

// the rows
struct Harness
{
    Harness() : system(registry), voip(&system)
    {
        voip.registerData();
        world::playerManager = &players;
        squadManager = &squads;
        players.local = &local;
        local.id = 1;
        local.team = 2;
    }
    ~Harness()
    {
        world::playerManager = nullptr;
        squadManager = nullptr;
        ::operator delete(HudVoipFixture::data(voip));
        HudVoipFixture::data(voip) = nullptr;
    }
    int rows(int column)
    {
        return column < static_cast<int>(HudVoipFixture::data(voip)->m_columns.size())
            ? static_cast<int>(HudVoipFixture::data(voip)->m_columns[column]->cells.size()) : -1;
    }
    ListBoxCell* cell(int column, int row)
    { return HudVoipFixture::data(voip)->m_columns[column]->cells[row]; }
    dice::meme::NameTreeRoot registry;
    PlainSystem system;
    HudVoip voip;
    VoipPlayers players;
    VoipSquads squads;
    VoipPlayer local;
};

static void updateGateCases()
{
    {   // A cleared enable flag returns without storing the visible flag.
        Harness h;
        HudVoipFixture::enabled(h.voip) = false;
        HudVoipFixture::visible(h.voip) = false;
        h.voip.update(true);
        CHECK(!HudVoipFixture::visible(h.voip));
        CHECK(h.rows(0) == 0);
    }
    {   // Not visible: the flag is stored and nothing is painted or cleared.
        Harness h;
        HudVoipFixture::enabled(h.voip) = true;
        h.voip.update(false);
        CHECK(!HudVoipFixture::visible(h.voip));
        CHECK(h.rows(0) == 0);
        h.voip.update(true);
        CHECK(HudVoipFixture::visible(h.voip));
    }
    {   // No local player: the flag is still stored.
        Harness h;
        HudVoipFixture::enabled(h.voip) = true;
        h.players.local = nullptr;
        h.voip.update(true);
        CHECK(HudVoipFixture::visible(h.voip));
        CHECK(h.rows(0) == 0);
    }
}

static void updateCases()
{
    {   // The commander row comes from squadManager->getCommander(localTeam).
        Harness h;
        HudVoipFixture::enabled(h.voip) = true;
        VoipPlayer commander;
        commander.id = 7; commander.talking = true; commander.commander = true;
        h.players.byId = &commander;
        h.squads.commanderId = 7;
        h.voip.update(true);
        CHECK(h.squads.lastTeam == 2);
        CHECK(h.players.lastRequestedId == 7);
        CHECK(h.rows(0) == 1);             // the hidden status-icon column
        CHECK(h.rows(1) == 1);             // the speaker column
        CHECK(h.rows(2) == 1);             // the name column
        CHECK(HudVoipFixture::data(h.voip)->m_columns.size() == 3);
        // A non-commander local player asks for its own squad's members.
        CHECK(h.players.lastSquadTeam == 2 && h.players.lastSquadId == 0);
        CHECK(h.players.lastLeaderTeam == -1);
    }
    {   // A commander local player asks for the team's squad leaders instead.
        Harness h;
        HudVoipFixture::enabled(h.voip) = true;
        h.local.commander = true;
        h.players.byId = nullptr;
        h.voip.update(true);
        CHECK(h.players.lastLeaderTeam == 2);
        CHECK(h.players.lastSquadTeam == -1);
    }
    {   // The nine-member cap is tested inside the loop against the
        // collection's size, which never changes - so a squad of ten paints
        // nothing, rather than the first nine.
        Harness h;
        HudVoipFixture::enabled(h.voip) = true;
        h.players.byId = nullptr;
        VoipPlayer members[10];
        for (int i = 0; i < 9; ++i) {
            members[i].id = 100 + i; members[i].team = 2; members[i].talking = true;
            h.players.squadMembers.push_back(&members[i]);
        }
        h.voip.update(true);
        CHECK(h.rows(0) == 9);
        members[9].id = 109; members[9].team = 2; members[9].talking = true;
        h.players.squadMembers.push_back(&members[9]);
        h.voip.update(true);               // clearRows, then paint nothing at all
        CHECK(h.rows(0) == 0);
    }
}

static void paintRowCases()
{
    {   // The row painter skips the local player, and refuses everything while
        // the local player is dead.
        Harness h;
        VoipPlayer other;
        other.id = 1; other.talking = true;      // same id as the local player
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.rows(0) == 0);
        other.id = 2;
        h.local.alive = false;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.rows(0) == 0);
        h.local.alive = true;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), nullptr);
        CHECK(h.rows(0) == 0);
    }
    {   // Not talking: three empty cells, of which only two can land.
        Harness h;
        VoipPlayer other; other.id = 2; other.talking = false;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.rows(0) == 1 && h.rows(1) == 1 && h.rows(2) == 1);
        CHECK(h.cell(0, 0) && h.cell(0, 0)->narrow.empty());
        CHECK(h.cell(0, 0) && !h.cell(0, 0)->isWide && !h.cell(0, 0)->isImage);
        CHECK(h.cell(0, 0) && h.cell(0, 0)->color.r == 1.0f && h.cell(0, 0)->color.a == 1.0f);
        // Muted and VoIP-disabled players are also "not talking" here.
        VoipPlayer muted; muted.id = 3; muted.talking = true; muted.muted = true;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &muted);
        CHECK(h.rows(0) == 2 && h.cell(0, 1)->narrow.empty());
        VoipPlayer off; off.id = 4; off.talking = true; off.voip = false;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &off);
        CHECK(h.rows(0) == 3 && h.cell(0, 2)->narrow.empty());
    }
    {   // A talking commander: the commander icon at 15x15, yellow.
        Harness h;
        VoipPlayer other; other.id = 2; other.talking = true; other.commander = true;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.rows(0) == 1 && h.rows(1) == 1 && h.rows(2) == 1);
        // The name lands in the 30-wide column 2 and carries the icon's colour.
        CHECK(h.cell(2, 0) && h.cell(2, 0)->narrow == "player");
        CHECK(h.cell(2, 0) && h.cell(2, 0)->color.g == 1.0f && h.cell(2, 0)->color.b == 0.0f);
        ListBoxCell* icon = h.cell(0, 0);
        CHECK(icon && icon->isImage && icon->texture == HudVoipFixture::commanderTexture(h.voip));
        CHECK(icon && icon->imageWidth == 15.0f && icon->imageHeight == 15.0f);
        CHECK(icon && icon->imageX == 0.0f && icon->imageY == -1.0f);
        CHECK(icon && icon->color.r == 1.0f && icon->color.g == 1.0f
              && icon->color.b == 0.0f && icon->color.a == 1.0f);
        ListBoxCell* speaker = h.cell(1, 0);
        CHECK(speaker && speaker->isImage && speaker->texture == HudVoipFixture::speakerTexture(h.voip));
        CHECK(speaker && speaker->imageWidth == 15.0f && speaker->imageY == -1.0f);
    }
    {   // A talking squad leader: the per-squad icon at 16x16, loaded once and
        // then cached, tinted with squadChatColors[2] and offset by -2.
        Harness h;
        VoipPlayer other;
        other.id = 2; other.talking = true; other.squadLeader = true; other.squad = 3;
        const std::size_t before = h.system.textures.size();
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.system.textures.size() == before + 1);
        CHECK(h.system.textures.back() == "Ingame/Player/Icons/Minimap/mini_Squad_3_small.tga");
        CHECK(HudVoipFixture::squadTextures(h.voip)[3] != 0);
        ListBoxCell* icon = h.cell(0, 0);
        // The squad's own chat colour reaches the name cell.
        CHECK(h.cell(2, 0) && h.cell(2, 0)->color.r == squadChatColors[3].r);
        CHECK(icon && icon->texture == HudVoipFixture::squadTextures(h.voip)[3]);
        CHECK(icon && icon->imageWidth == 16.0f && icon->imageHeight == 16.0f);
        CHECK(icon && icon->imageY == -2.0f);   // m_field024 - 1.0f
        CHECK(icon && icon->color.r == squadChatColors[2].r);
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        CHECK(h.system.textures.size() == before + 1);   // cached, not reloaded
    }
    {   // A talking ordinary player: the empty icon, still 16x16 and tinted.
        Harness h;
        VoipPlayer other; other.id = 2; other.talking = true; other.squad = 1;
        HudVoipFixture::paint(h.voip, HudVoipFixture::data(h.voip), &other);
        ListBoxCell* icon = h.cell(0, 0);
        CHECK(icon && icon->texture == HudVoipFixture::emptyTexture(h.voip));
        CHECK(icon && icon->imageWidth == 16.0f && icon->imageY == -2.0f);
    }
}

// the list-data entries
static void listDataCases()
{
    void* storage = ::operator new(sizeof(BfListBoxData));
    BfListBoxData* list = new(storage) BfListBoxData("cells");
    list->addColumn(10, true, false);
    // The narrow cell constructor: `narrow` assigned, `wide` left empty.
    CHECK(list->appendCell(0, std::string("text"), 0.25f, 0.5f, 0.75f, 1.0f, 6));
    ListBoxCell* cell = list->m_columns[0]->cells[0];
    CHECK(cell && cell->narrow == "text" && cell->wide.empty());
    CHECK(cell && !cell->isWide && !cell->isImage);
    CHECK(cell && cell->color.r == 0.25f && cell->color.b == 0.75f);
    CHECK(cell && cell->field064 == 6 && cell->field068 == 1);
    CHECK(cell && cell->resourceIndex == -1 && cell->texture == 0);
    CHECK(cell && cell->imageWidth == 0.0f && cell->field090 == 0);
    // A column index past the end is accepted and silently drops the cell.
    CHECK(list->appendCell(1, std::string("dropped"), 1, 1, 1, 1, 0));
    CHECK(list->appendCell(-1, std::string("dropped"), 1, 1, 1, 1, 0));
    CHECK(list->m_columns[0]->cells.size() == 1);
    // Both forwarders reach their targets unchanged.
    CHECK(list->appendCell0x7c81c0(0, std::string("forwarded"), 1, 1, 1, 1, 2));
    CHECK(list->m_columns[0]->cells[1]->narrow == "forwarded");
    CHECK(list->m_columns[0]->cells[1]->field064 == 2);
    CHECK(list->appendImageCell0x7c8300(0, 42, 3.0f, 4.0f, 5.0f, 6.0f, 1, 1, 1, 1));
    CHECK(list->m_columns[0]->cells[2]->isImage);
    CHECK(list->m_columns[0]->cells[2]->texture == 42);
    CHECK(list->m_columns[0]->cells[2]->imageWidth == 3.0f);
    ::operator delete(list);
}

int main()
{
    static_assert(sizeof(void*) == 4, "x86 only");
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    void* hudStorage = ::operator new(sizeof(ObserverHud));
    std::memset(hudStorage, 0x5a, sizeof(ObserverHud));
    observerHud = new(hudStorage) ObserverHud;
    hudManager = observerHud;
    layoutCases();
    registerDataCases();
    updateGateCases();
    updateCases();
    paintRowCases();
    listDataCases();
    hudManager = nullptr;
    destroyConstructorOnly(observerHud);
    std::printf("HUD VoIP %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
