// [TEMP] [CHOSEN, not recovered] bounded DROP-retrieval fixtures, never linked into production.
// Real constructors/layouts, typed virtual calls, production search and destruction forwards.
// Grid/ray results, player state, weapon magazines and handleMessage are fixture observations.
// Fixture objects intentionally live until process exit: this is not a lifecycle/boot test.
#include <algorithm>
#include <array>
#include <bitset>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

// Access only for seeding recovered object fields without introducing production test setters.
#define private public
#define protected public
#include "../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../BF2/Game/GameLogic/Player.h"
#include "../BF2/Game/Objects/Game/Soldier.h"
#include "../BF2/Game/Objects/Game/SoldierTemplate.h"
#include "../BF2/Game/Objects/Game/Zipline.h"
#include "../BF2/Game/Objects/Game/RopeLink.h"
#include "../BF2/Game/Objects/Game/GrapplingHookRope.h"
#include "../BF2/Game/Objects/TemplateLeaves.h"
#include "../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../BF2/Game/Objects/GenericFireArmTemplate.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../BF2/Animation/BoneAnimation/Skeleton.h"
#undef protected
#undef private
#include "../BF2/Animation/BoneAnimation/AnimationSystem.h"
#include "../BF2/Scene/Object/ObjectPredicator.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Physics/ResponsePhysicsManager.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"

namespace dice { namespace hfe {
extern ClassManager* classManager;
namespace world { extern ResponsePhysicsManager* responsePhysicsManager; }
} }
using namespace dice::hfe;
using namespace dice::hfe::world;

namespace {
int checks = 0, failures = 0;
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                  fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset, symbol->Name, displacement);
        else
            std::fprintf(stderr, "  %08llx\n", frame.AddrPC.Offset);
        IMAGEHLP_LINE64 line = {}; line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
            std::fprintf(stderr, "    %s:%lu\n", line.FileName, line.LineNumber);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                         0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}

class FixtureClasses : public ClassManager
{
public:
    IBase* createInstance(unsigned int cid, unsigned int iid, IBase* outer) override
    {
        if (cid == CID_BinTreeCuller) return BinTreeCuller::create(iid, outer);
        if (cid == CID_QuadTreeCuller) return QuadTreeCuller::create(iid, outer);
        std::fprintf(stderr, "Unexpected fixture factory %u\n", cid);
        std::abort();
    }
};

// Only the real GameLogic constructor/PCO registry is needed by Soldier construction. Every
// abstract server operation fails loudly if setup or the bounded test ever escapes that scope.
class FixtureLogic : public GameLogic
{
    [[noreturn]] static void unused()
    { std::fputs("Unexpected fixture GameLogic operation\n", stderr); std::abort(); }
public:
#define UNUSED_METHOD(signature) signature override { unused(); }
    UNUSED_METHOD(void addRef())
    UNUSED_METHOD(int getRef())
    UNUSED_METHOD(int release())
    UNUSED_METHOD(void* queryInterface(unsigned int) const)
    UNUSED_METHOD(void runNextLevel())
    UNUSED_METHOD(void setNextLevel(int))
    UNUSED_METHOD(bool selectKitAndUnlockLevel(IPlayer*, int, int))
    UNUSED_METHOD(void setTicketState(unsigned int, int))
    UNUSED_METHOD(int getTicketState(unsigned int))
    UNUSED_METHOD(void setDefaultNumberOfTickets(unsigned int, unsigned int))
    UNUSED_METHOD(unsigned int getDefaultNumberOfTickets(unsigned int))
    UNUSED_METHOD(void setDefaultNumberOfTicketsEx(unsigned int, unsigned int, unsigned int))
    UNUSED_METHOD(unsigned int getDefaultNumberOfTicketsEx(unsigned int, unsigned int))
    UNUSED_METHOD(void setDefaultTimeToNextAIWave(float))
    UNUSED_METHOD(float getDefaultTimeToNextAIWave())
    UNUSED_METHOD(void setTicketLossAtEndPerMin(float))
    UNUSED_METHOD(float getTicketLossAtEndPerMin())
    UNUSED_METHOD(void setTicketLossPerMin(unsigned int, float))
    UNUSED_METHOD(float getTicketLossPerMin(unsigned int))
    UNUSED_METHOD(void playEffect(Vec3, const IObjectTemplate*, Vec3))
    UNUSED_METHOD(bool handleCollisionForProjectile(IObject*, IObject*, const Vec3&, const Vec3&,
                  const Vec3&, int, int, CollisionFaceInfo*, CollisionFaceInfo*))
    UNUSED_METHOD(bool handleCollisionForReplenisher(bool, IObject*, IObject*, const Vec3&,
                  const Vec3&, const Vec3&, int, int))
    UNUSED_METHOD(bool handleCollisionForResurrecter(IObject*, IObject*, float, float,
                  const Vec3&, const Vec3&, const Vec3&, int, int))
    UNUSED_METHOD(void handleCollision(IObject*, IObject*, const Vec3&, const Vec3&,
                  const Vec3&, int, int, CollisionFaceInfo*, CollisionFaceInfo*))
    UNUSED_METHOD(void spawnMultipleObjects(const IObjectTemplate*, int))
    UNUSED_METHOD(IObject* spawnObject(const IObjectTemplate*, Vec3, Vec3, int))
    UNUSED_METHOD(IObject* spawnObject(const std::string&, Vec3, Vec3, int))
    UNUSED_METHOD(void checkPlayerTriggers(IPlayer*, IObject*, io::PlayerInput, bool))
    UNUSED_METHOD(void toggleLocalFreeCamera())
    UNUSED_METHOD(TicketHistory& getTicketHistory())
    UNUSED_METHOD(void setTicketHistory(const TicketHistory&))
#undef UNUSED_METHOD
};

class FixtureObjects : public ObjectManager
{
public:
    std::vector<IObject*> scratch, first, second, destroyed;
    std::vector<bool> blocked;
    std::vector<float> radii;
    std::vector<Vec3> origins, directions;
    std::vector<std::size_t> incomingSizes;
    int rays = 0;
    bool queryContract = true, rayContract = true;
    std::vector<IObject*>& getObjectTempVector() override { return scratch; }
    void getObjectsFromGrid(const Vec3& centre, float radius, std::vector<IObject*>& out,
                             ObjectPredicator* pred) override
    {
        queryContract = queryContract && &out == &scratch && pred && pred->getFlags() == 0x200 &&
                        centre.x == 0 && centre.y == 0 && centre.z == 0;
        incomingSizes.push_back(out.size());
        out = radii.empty() ? first : second; // real radius query replaces its output
        radii.push_back(radius);
    }
    bool intersectLine(IObject*& object, Vec3& point, Vec3& normal, int& part,
                        const Vec3& from, const Vec3& delta, ObjectPredicator* pred,
                        bool a, bool b, bool c, bool d, unsigned int ignored) override
    {
        rayContract = rayContract && !object && point.x == 0 && point.y == 0 && point.z == 0 &&
                      normal.x == 0 && normal.y == 0 && normal.z == 0 &&
                      pred && pred->getFlags() == 0x100 && a && !b && !c && !d &&
                      ignored == CID_SoldierTemplate;
        origins.push_back(from);
        directions.push_back(delta);
        part = 0;
        const bool result = static_cast<std::size_t>(rays) < blocked.size() && blocked[rays];
        ++rays;
        return result;
    }
    void destroyObject(IObject* object) override { destroyed.push_back(object); }
    void reset()
    {
        scratch.assign(1, 0); // verifies explicit initial erase before the first query
        first.clear(); second.clear(); destroyed.clear(); blocked.clear(); radii.clear();
        origins.clear(); directions.clear(); incomingSizes.clear(); rays = 0;
        queryContract = rayContract = true;
    }
};

class FixtureTemplates : public ObjectTemplateManager
{
public:
    ZiplineTemplate* zipline = 0;
    mutable int lookups = 0;
    IObjectTemplate* getTemplate(const std::string& name) const override
    {
        if (name == "Zipline") { ++lookups; return zipline; }
        if (name.empty() || name == "fallingWeapon" || name == "MultiPlayerFreeCamera")
            return 0; // absent optional content fixtures; no free-camera action is under test
        std::fprintf(stderr, "Unexpected fixture template %s\n", name.c_str());
        std::abort();
    }
};

class FixtureSoldier : public Soldier
{
public:
    explicit FixtureSoldier(const SoldierTemplate* t) : Soldier(t) {}
    Vec3 position = {0, 0, 0};
    mutable int positionReads = 0, heightReads = 0;
    int kit = 13, messages = 0;
    IPlayer* messagePlayer = 0;
    bool messageContract = true;
    std::vector<IObject*> weapons;
    const Vec3& getAbsolutePosition() const override { ++positionReads; return position; }
    float getStandingHeight() const override { ++heightReads; return 2.0f; }
    int getKitType() const override { return kit; }
    void* getWeapons() override { return &weapons; }
    void handleMessage(TemplateMessage message, IPlayer* player, bool flag) override
    {
        ++messages; messagePlayer = player;
        messageContract = messageContract && static_cast<int>(message) == 26 && !flag;
    }
};

class FixturePlayer : public Player
{
public:
    explicit FixturePlayer(IObject* v) : Player(0, 0), vehicle(v) {}
    IObject* vehicle;
    bool alive = true, manDown = false;
    mutable int vehicleReads = 0;
    IObject* getVehicle() const override { ++vehicleReads; return vehicle; }
    bool getIsAlive() const override { return alive; }
    bool getIsManDown() override { return manDown; }
};

class FixtureZipline : public Zipline
{
public:
    explicit FixtureZipline(const ZiplineTemplate* t) : Zipline(t) { _dword180 = 13; }
    Vec3 position = {1, 0, 0};
    mutable int positionReads = 0;
    const Vec3& getAbsolutePosition() const override { ++positionReads; return position; }
};

class FixtureWeapon : public GenericFireArm
{
public:
    explicit FixtureWeapon(const GenericFireArmTemplate* t) : GenericFireArm(t) {}
    std::vector<int> magazines;
    int capacity = 2, writes = 0;
    IObjectTemplate* primary = 0;
    mutable int primaryReads = 0;
    int getCurrentNrOfMags() const override { return static_cast<int>(magazines.size()); }
    int getAmmoInMag(unsigned int i) const override { return magazines.at(i); }
    int getMagSize() override { return capacity; }
    void setAmmoInMag(unsigned int i, int value) override { magazines.at(i) = value; ++writes; }
    IObjectTemplate* getProjectileTemplate() const override { ++primaryReads; return primary; }
};
} // namespace

int main()
{
    static_assert(sizeof(void*) == 4, "This fixture must be x86");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    SetUnhandledExceptionFilter(reportFault);
    classManager = new FixtureClasses;
    eventManager = new EventManager;
    FixtureObjects* objects = new FixtureObjects;
    objectManager = objects;
    responsePhysicsManager = new ResponsePhysicsManager;
    FixtureTemplates* templates = new FixtureTemplates;
    objectTemplateManager = templates;
    gameLogic = new FixtureLogic; // Soldier's real constructor removes itself from the empty PCO list
    SoldierTemplate* soldierTemplate = new SoldierTemplate;
    soldierTemplate->m_skeleton1P = new dice::anim::Skeleton;
    soldierTemplate->m_skeleton3P = new dice::anim::Skeleton;
    // Empty loaded-rig fixture: the retail default constructor leaves boneCount unwritten.
    soldierTemplate->m_skeleton1P->m_boneCount = 0;
    soldierTemplate->m_skeleton3P->m_boneCount = 0;
    soldierTemplate->m_animationSystem1P = new dice::anim::AnimationSystemTemplate;
    soldierTemplate->m_animationSystem3P = new dice::anim::AnimationSystemTemplate;
    FixtureSoldier* soldier = new FixtureSoldier(soldierTemplate);
    FixturePlayer* player = new FixturePlayer(soldier);
    ZiplineTemplate* zipTemplate = new ZiplineTemplate;
    zipTemplate->setName("deployed_zip");
    zipTemplate->m_searchRadius = 50;
    FixtureZipline* zip = new FixtureZipline(zipTemplate);
    FixtureZipline* otherZip = new FixtureZipline(zipTemplate);
    GrapplingHookRopeTemplate* ropeTemplate = new GrapplingHookRopeTemplate;
    ropeTemplate->setName("deployed_rope");
    GrapplingHookRope* rope = new GrapplingHookRope(ropeTemplate);
    GrapplingHookRope* otherRope = new GrapplingHookRope(ropeTemplate);
    RopeLinkTemplate* linkTemplate = new RopeLinkTemplate;
    RopeLink* link = static_cast<RopeLink*>(linkTemplate->createObject());
    RopeLink* otherLink = static_cast<RopeLink*>(linkTemplate->createObject());
    link->m_rope = rope; link->_dword14C = 13;
    otherLink->m_rope = otherRope; otherLink->_dword14C = 13;
    link->setAbsolutePosition(Vec3{1, 0, 0});
    otherLink->setAbsolutePosition(Vec3{2, 0, 0});
    HookLinkTemplate* hookTemplate = new HookLinkTemplate;
    RopeLink* hook = static_cast<RopeLink*>(hookTemplate->createObject());
    hook->m_rope = rope; hook->_dword14C = 13;
    hook->setAbsolutePosition(Vec3{1, 0, 0});
    GenericFireArmTemplate* gunTemplate = new GenericFireArmTemplate;
    gunTemplate->m_secondaryProjectileTemplate = zipTemplate;
    FixtureWeapon* gun = new FixtureWeapon(gunTemplate);
    FixtureWeapon* otherGun = new FixtureWeapon(gunTemplate);
    io::PlayerInput input = {};
    const auto reset = [&]() {
        objects->reset(); templates->zipline = 0; templates->lookups = 0;
        player->alive = true; player->manDown = false; player->vehicleReads = 0;
        soldier->messages = soldier->heightReads = soldier->positionReads = 0;
        soldier->m_readyToClimbTime = 0; soldier->kit = 13;
        soldier->weapons = {gun};
        zip->position = {1, 0, 0}; otherZip->position = {2, 0, 0}; zip->_dword180 = 13;
        zip->positionReads = otherZip->positionReads = 0;
        zip->m_endPoint = {10, 0, 0};
        rope->_float1C0 = otherRope->_float1C0 = 0;
        link->_dword14C = 13;
        gun->magazines = {2, 0, 0}; otherGun->magazines = {0};
        gun->writes = otherGun->writes = gun->primaryReads = otherGun->primaryReads = 0;
        gun->primary = otherGun->primary = ropeTemplate;
        gunTemplate->m_secondaryProjectileTemplate = zipTemplate;
        input = io::PlayerInput();
    };
    const auto run = [&]() { ServerGameLogic::checkSpecialForcesTriggers(player, soldier, input); };
    const auto destroyed = [&](IObject* value) {
        return objects->destroyed == std::vector<IObject*>(1, value);
    };

    reset(); run();
    check(soldier->messages == 1 && soldier->messagePlayer == player && soldier->messageContract,
          "empty search reaches handleMessage(26, player, false)");
    check(objects->radii == std::vector<float>{4} && objects->incomingSizes[0] == 0 &&
          objects->queryContract && templates->lookups == 1 && player->vehicleReads == 4,
          "initial erase, 4m predicate query, repeated vehicle reads, absent named template");
    reset(); templates->zipline = zipTemplate; run();
    check(objects->radii == std::vector<float>({4, 50}) && soldier->messages == 1,
          "empty fallback uses the named template search radius");

    reset(); objects->first = {otherZip, zip}; run();
    check(destroyed(zip) && gun->magazines == std::vector<int>({2, 1, 0}) && !gun->primaryReads,
          "nearest zipline refunds first nonfull magazine through secondary template");
    check(objects->radii.size() == 1 && templates->lookups == 0 && soldier->heightReads == 2 &&
          zip->positionReads == 2 && player->vehicleReads == 4, "near zipline skips fallback query");
    reset(); objects->first = {zip, otherZip}; otherZip->position = zip->position; run();
    check(destroyed(zip), "equal zipline distances retain first candidate");
    reset(); objects->first = {zip}; zip->position = {0, 2, 0}; run();
    check(destroyed(zip), "standing-height boundary is inclusive");
    reset(); objects->first = {zip}; zip->position = {0, 2.001f, 0}; run();
    check(objects->destroyed.empty() && soldier->messages == 1, "above standing height is rejected");
    reset(); objects->first = {zip}; zip->_dword180 = 12; run();
    check(objects->destroyed.empty() && !soldier->messages && !gun->writes,
          "wrong kit suppresses refund and ordinary DROP fallback");
    reset(); objects->first = {zip}; player->manDown = true; run();
    check(objects->destroyed.empty() && !soldier->messages, "man-down zipline is selected but not refunded");
    reset(); objects->first = {zip}; player->alive = false; run();
    check(objects->destroyed.empty() && !soldier->messages, "dead player does not retrieve zipline");
    reset(); objects->first = {zip}; gun->magazines = {2, 2}; run();
    check(objects->destroyed.empty() && !soldier->messages, "full magazines preserve deployed zipline");
    reset(); objects->first = {zip}; soldier->weapons = {0, link, gun, otherGun}; run();
    check(objects->destroyed == std::vector<IObject*>({zip, zip}) && gun->writes == 1 &&
          otherGun->writes == 1, "null/nonweapon entries skipped; refund continues to next weapon");
    reset(); objects->first = {zip}; gunTemplate->m_secondaryProjectileTemplate = ropeTemplate; run();
    check(objects->destroyed.empty(), "zipline primary match cannot replace secondary-name match");
    reset(); objects->first = {zip}; gunTemplate->m_secondaryProjectileTemplate = 0; run();
    check(objects->destroyed.empty(), "absent secondary projectile is skipped");

    reset(); templates->zipline = zipTemplate; objects->second = {zip};
    zip->position = {-10, 0, 0}; zip->m_endPoint = {10, 0, 0}; run();
    check(destroyed(zip), "fallback selects interior of a long segment");
    reset(); templates->zipline = zipTemplate; objects->first = {zip};
    zip->position = {-10, 3, 0}; zip->m_endPoint = {10, 3, 0}; run();
    check(destroyed(zip) && objects->incomingSizes[1] == 1 && objects->scratch.empty(),
          "fallback retains first snapshot and accepts exactly 3m from segment");
    reset(); templates->zipline = zipTemplate; objects->second = {zip};
    zip->position = {-10, 3.001f, 0}; zip->m_endPoint = {10, 3.001f, 0}; run();
    check(objects->destroyed.empty() && soldier->messages == 1, "segment outside 3m is rejected");
    reset(); templates->zipline = zipTemplate; objects->second = {zip};
    zip->position = {-10, 0, 0}; zip->m_endPoint = {-3, 0, 0}; run();
    check(objects->destroyed.empty() && soldier->messages == 1, "endpoint at exactly 3m is rejected");
    reset(); templates->zipline = zipTemplate; objects->second = {zip};
    zip->position = {-10, 0, 0}; zip->m_endPoint = {-2.999f, 0, 0}; run();
    check(destroyed(zip), "endpoint just inside 3m is accepted");
    reset(); templates->zipline = zipTemplate; objects->second = {zip};
    zip->position = zip->m_endPoint = {-1, 0, 0}; run();
    check(destroyed(zip), "zero-length segment still allows endpoint retrieval");

    reset(); objects->first = {link, otherLink}; run();
    check(destroyed(otherRope) && gun->magazines == std::vector<int>({2, 1, 0}) && gun->primaryReads == 1,
          "last eligible rope wins and refunds through primary projectile template");
    check(objects->rays == 2 && objects->rayContract && objects->origins[1].x == 2 &&
          objects->directions[1].x == -2, "rope ray uses link origin, soldier delta and retail flags");
    reset(); objects->first = {link}; objects->blocked = {true}; run();
    check(objects->destroyed.empty() && soldier->messages == 1, "obstructed rope falls back to DROP");
    reset(); objects->first = {link}; rope->_float1C0 = 0.001f; run();
    check(objects->destroyed.empty() && objects->rays == 1 && soldier->messages == 1,
          "rope cooldown is checked after raycast");
    reset(); objects->first = {link}; soldier->m_readyToClimbTime = 0.001f; run();
    check(objects->destroyed.empty() && !objects->rays, "soldier climb cooldown prevents raycast");
    reset(); objects->first = {link};
    input.mappingFlags = 1ULL << io::PIAltFire; input.inputs[io::PIAltFire] = 1; run();
    check(objects->destroyed.empty() && !objects->rays && soldier->messages == 1,
          "mapped alternate fire prevents rope retrieval");
    reset(); objects->first = {link}; input.inputs[io::PIAltFire] = 1; run();
    check(destroyed(rope), "unmapped alternate-fire value does not suppress retrieval");
    reset(); objects->first = {link};
    input.mappingFlags = 1ULL << io::PIAltFire; input.inputs[io::PIAltFire] = 0.5f; run();
    check(destroyed(rope), "alternate-fire threshold is strictly greater than one half");
    reset(); objects->first = {hook}; run();
    check(destroyed(rope), "HookLink template follows the rope retrieval branch");
    reset(); objects->first = {link}; link->_dword14C = 12; run();
    check(objects->destroyed.empty() && !soldier->messages, "rope kit mismatch suppresses fallback");
    reset(); objects->first = {link}; gun->primary = zipTemplate; run();
    check(objects->destroyed.empty(), "rope retrieval requires matching primary template name");
    reset(); objects->first = {zip, link}; run();
    check(objects->destroyed == std::vector<IObject*>({zip, rope}) && gun->magazines[1] == 2,
          "zipline and rope refund independently in shipped order");

    std::printf("special-forces DROP runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
