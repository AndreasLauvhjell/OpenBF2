// Control-point template configuration. Original method placement is Game/Objects/Game/ControlPoint.cpp.

#include "ControlPointTemplate.h"

#include "../../Core/Ids.h"
#include "../../Core/Debug.h"
#include "../../IO/Stream/IStream.h"
#include <cmath>
#include <sstream>

// preCache's three collaborators.
#include "IObject.h"                                          // init() / forceDestroy()
#include "Game/ControlPoint.h"                                // what createObject news
#include "../IGameLogic.h"                                    // gameLogic, getTeamFlag
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"   // objectTemplateManager

namespace dice {
namespace hfe {
namespace world {

// The constructor - 0x816F770, 938 bytes, read end to end.
// It writes every slot from +0x180 to +0x21F with no gaps. Each store lands on a field the console
// -binding route had already named independently, and the two derivations agree everywhere, which
// is what makes this layout trustworthy rather than merely plausible.
ControlPointTemplate::ControlPointTemplate()
{
    m_refCount = 1;                              // +0x180
    radius = 1.0f;                               // +0x184
    startTeam = 0;                               // +0x188
    // controlPointName (+0x18C): default-constructed empty (the _S_empty_rep construction)
    controlPointId = -1;                         // +0x190
    timeToGetControl = 10.0f;                    // +0x194
    timeToLoseControl = 10.0f;                   // +0x198

    disableIfEnemyInsideRadius = false;          // +0x19C
    disableWhenLosingControl = false;            // +0x19D
    loseControlWhenEnemyClose = true;            // +0x19E the only true one of the four
    loseControlWhenNotClose = false;             // +0x19F
    unableToChangeTeam = false;                  // +0x1A0
    minNrToTakeControl = 1;                      // +0x1A4
    isUnstrategicControlPoint = false;           // +0x1A8
    isHemisphere = false;                        // +0x1A9
    onlyTakeableByTeam = 0;                      // +0x1AC 0 = any team
    areaValueTeam1 = 0;                          // +0x1B0
    areaValueTeam2 = 0;                          // +0x1B4
    enemyTicketLossWhenCaptured = 0;             // +0x1B8
    showOnMinimap = true;                        // +0x1BC
    groupId = -1;                                // +0x1C0

    // m_supplyGroupsNeeded (+0x1C4, +0x1D0): the ELF zeroes both triples in a two-iteration loop -
    // two empty vectors, which is what default construction gives.
    m_supplyValueNeeded[0] = 0;                  // +0x1DC
    m_supplyValueNeeded[1] = 0;                  // +0x1E0
    m_supplyValue[0] = 0;                        // +0x1E4
    m_supplyValue[1] = 0;                        // +0x1E8

    hoistFlag = true;                            // +0x1EC
    // Empty team flag names fall back to GameLogic::getTeamFlag during precaching.

    hoistMinMax.x = 0.2f;                        // +0x1FC
    hoistMinMax.y = 0.9f;                        // +0x200
    signalTriggerId = -1;                        // +0x204
    radiusOffset.x = 0.0f;                       // +0x208
    radiusOffset.y = 0.0f;                       // +0x20C
    radiusOffset.z = 0.0f;                       // +0x210
    forSoldierOnly = false;                      // +0x214
    hasGreyCapturingState = false;               // +0x215
    minimapNameOffset.x = 0.0f;                  // +0x218
    minimapNameOffset.y = 0.0f;                  // +0x21C

    // ObjectTemplate's +0x10 string, assigned from the literal via std::string::assign(char*, len)
    // with the length coming from a live strlen call. Hex-Rays names that field `networkableInfo`
    // in preCache's decompile, which matches this tree's own ObjectTemplate.h.
    m_networkableInfo.assign("ControlPointInfo");

    updateFlags(0x50, 0);                        // 0x50, not the 0x40 the rest of this family
                                                 // uses - bit 4 as well as bit 6

    // Ensure two material mappings and set both explicitly. mapMaterial cannot override these defaults.
    if (m_materialVector.size() < 2)
        m_materialVector.resize(2);
    m_materialVector[0] = std::make_pair(std::string("Rock_or_concrete"), 78u);
    m_materialVector[1] = std::make_pair(std::string("Metal_solid"), 21u);
}

// The ELF destructor is 304 B (D1) / 315 B (D0) only because GCC inlines the release of four
// strings and two vectors into it. All of that is implicit in C++.
ControlPointTemplate::~ControlPointTemplate() {}

unsigned int ControlPointTemplate::getClassID() const { return CID_ControlPointTemplate; }

void* ControlPointTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ControlPointTemplate)
        return this;
    return BundleTemplate::queryImplementation(classId);
}

// Exposes the control-point interface in addition to the inherited template interfaces.
void* ControlPointTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)                    return const_cast<ControlPointTemplate*>(this);
    if (iid == IID_IObjectTemplate)          return const_cast<ControlPointTemplate*>(this);
    // Cast to the interface, not the concrete base: only that applies the +0x30 / +0x164
    // sub-object adjustment the ELF's `lea edx, [eax+30h]` / `lea edx, [eax+164h]` perform.
    if (iid == IID_ISimpleObjectTemplate)
        return static_cast<ISimpleObjectTemplate*>(const_cast<ControlPointTemplate*>(this));
    if (iid == IID_IBundleTemplate)
        return static_cast<IBundleTemplate*>(const_cast<ControlPointTemplate*>(this));
    if (iid == IID_IControlPointTemplate)
        return static_cast<IControlPointTemplate*>(const_cast<ControlPointTemplate*>(this));
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<ControlPointTemplate*>(this));
    return 0;
}

void ControlPointTemplate::addRef()  { ++m_refCount; }
int  ControlPointTemplate::getRef()  { return m_refCount; }
int  ControlPointTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// Intentionally ignores mapping requests to preserve the constructor's two material entries.
void ControlPointTemplate::mapMaterial(unsigned int, const std::string&, unsigned int) {}

// THE trivial accessors - 17-byte setters (`[this+off] = arg; ret`) and 14/15-byte getters
// (`return [this+off]`), verified against setStartTeam/getStartTeam and uniform across the family.
void  ControlPointTemplate::setStartTeam(int v)                 { startTeam = v; }
int   ControlPointTemplate::getStartTeam() const                { return startTeam; }
void  ControlPointTemplate::setRadius(float v)                  { radius = v; }
float ControlPointTemplate::getRadius() const                   { return radius; }

// BY value, then assigned - the mangled name is `Ss`, not `RKSs`, and the body tail-jumps into
// std::string::assign after rewriting the `this` slot to &controlPointName. Same for the three
// flag-template setters below.
void ControlPointTemplate::setControlPointName(std::string v)   { controlPointName.assign(v); }
const std::string& ControlPointTemplate::getControlPointName()  { return controlPointName; }

void ControlPointTemplate::setControlPointId(int v)             { controlPointId = v; }
int  ControlPointTemplate::getControlPointId()                  { return controlPointId; }

void ControlPointTemplate::setDisableIfEnemyInsideRadius(bool v){ disableIfEnemyInsideRadius = v; }
bool ControlPointTemplate::getDisableIfEnemyInsideRadius()      { return disableIfEnemyInsideRadius; }
void ControlPointTemplate::setDisableWhenLosingControl(bool v)  { disableWhenLosingControl = v; }
bool ControlPointTemplate::getDisableWhenLosingControl()        { return disableWhenLosingControl; }
void ControlPointTemplate::setLoseControlWhenEnemyClose(bool v) { loseControlWhenEnemyClose = v; }
bool ControlPointTemplate::getLoseControlWhenEnemyClose()       { return loseControlWhenEnemyClose; }
void ControlPointTemplate::setLoseControlWhenNotClose(bool v)   { loseControlWhenNotClose = v; }
bool ControlPointTemplate::getLoseControlWhenNotClose()         { return loseControlWhenNotClose; }

void  ControlPointTemplate::setTimeToGetControl(float v)        { timeToGetControl = v; }
float ControlPointTemplate::getTimeToGetControl()               { return timeToGetControl; }
void  ControlPointTemplate::setTimeToLoseControl(float v)       { timeToLoseControl = v; }
float ControlPointTemplate::getTimeToLoseControl()              { return timeToLoseControl; }
void  ControlPointTemplate::setUnableToChangeTeam(bool v)       { unableToChangeTeam = v; }
bool  ControlPointTemplate::getUnableToChangeTeam() const       { return unableToChangeTeam; }

void ControlPointTemplate::setOnlyTakeableByTeam(unsigned int v){ onlyTakeableByTeam = v; }
unsigned int ControlPointTemplate::getOnlyTakeableByTeam()      { return onlyTakeableByTeam; }
void ControlPointTemplate::setMinNrToTakeControl(unsigned int v){ minNrToTakeControl = v; }
unsigned int ControlPointTemplate::getMinNrToTakeControl()      { return minNrToTakeControl; }

void ControlPointTemplate::setIsUnstrategicControlPoint(bool v) { isUnstrategicControlPoint = v; }
bool ControlPointTemplate::getIsUnstrategicControlPoint()       { return isUnstrategicControlPoint; }

void ControlPointTemplate::setAreaValueTeam1(unsigned int v)    { areaValueTeam1 = v; }
unsigned int ControlPointTemplate::getAreaValueTeam1()          { return areaValueTeam1; }
void ControlPointTemplate::setAreaValueTeam2(unsigned int v)    { areaValueTeam2 = v; }
unsigned int ControlPointTemplate::getAreaValueTeam2()          { return areaValueTeam2; }

void ControlPointTemplate::setEnemyTicketLossWhenCaptured(int v){ enemyTicketLossWhenCaptured = v; }
int  ControlPointTemplate::getEnemyTicketLossWhenCaptured()     { return enemyTicketLossWhenCaptured; }
void ControlPointTemplate::setShowOnMinimap(bool v)             { showOnMinimap = v; }
bool ControlPointTemplate::getShowOnMinimap()                   { return showOnMinimap; }
void ControlPointTemplate::setSignalTriggerId(int v)            { signalTriggerId = v; }
int  ControlPointTemplate::getSignalTriggerId() const           { return signalTriggerId; }

// The pair the old header flagged. Same field on both sides - see the sret note at the top.
void ControlPointTemplate::setRadiusOffset(const Vec3& v)       { radiusOffset = v; }
Vec3 ControlPointTemplate::getRadiusOffset()                    { return radiusOffset; }

void ControlPointTemplate::setGroupId(int v)                    { groupId = v; }
int  ControlPointTemplate::getGroupId()                         { return groupId; }

void ControlPointTemplate::setHoistFlag(bool v)                 { hoistFlag = v; }
bool ControlPointTemplate::getHoistFlag()                       { return hoistFlag; }

void ControlPointTemplate::setFlagTemplateTeam0(std::string v)  { m_flagTemplateTeam[0].assign(v); }
const std::string& ControlPointTemplate::getFlagTemplateTeam0() { return m_flagTemplateTeam[0]; }
void ControlPointTemplate::setFlagTemplateTeam1(std::string v)  { m_flagTemplateTeam[1].assign(v); }
const std::string& ControlPointTemplate::getFlagTemplateTeam1() { return m_flagTemplateTeam[1]; }
void ControlPointTemplate::setFlagTemplateTeam2(std::string v)  { m_flagTemplateTeam[2].assign(v); }
const std::string& ControlPointTemplate::getFlagTemplateTeam2() { return m_flagTemplateTeam[2]; }

void ControlPointTemplate::setHoistMinMax(Vec2 v)               { hoistMinMax = v; }
Vec2 ControlPointTemplate::getHoistMinMax()                     { return hoistMinMax; }

void ControlPointTemplate::setIsHemisphere(bool v)              { isHemisphere = v; }
bool ControlPointTemplate::getIsHemisphere()                    { return isHemisphere; }
void ControlPointTemplate::setForSoldierOnly(bool v)            { forSoldierOnly = v; }
bool ControlPointTemplate::getForSoldierOnly()                  { return forSoldierOnly; }
void ControlPointTemplate::setHasGreyCapturingState(bool v)     { hasGreyCapturingState = v; }
bool ControlPointTemplate::getHasGreyCapturingState()           { return hasGreyCapturingState; }

void ControlPointTemplate::setMinimapNameOffset(Vec2 v)         { minimapNameOffset = v; }
Vec2 ControlPointTemplate::getMinimapNameOffset()               { return minimapNameOffset; }

// Supply arrays use team - 1. Add and remove accept only teams 1 and 2.
// getSupplyGroupNeeded does not validate the index; callers must supply a valid team.
int ControlPointTemplate::getSupplyValueNeeded(int team)
{
    if (static_cast<unsigned int>(team - 1) > 1)
        return -1;
    return m_supplyValueNeeded[team - 1];
}

int ControlPointTemplate::getSupplyValue(int team)
{
    if (static_cast<unsigned int>(team - 1) > 1)
        return -1;
    return m_supplyValue[team - 1];
}

void ControlPointTemplate::removeSupplyGroupNeeded(int team, int group)
{
    if (static_cast<unsigned int>(team - 1) > 1)
        return;

    std::vector<SupplyGroup>& v = m_supplyGroupsNeeded[team - 1];
    for (std::vector<SupplyGroup>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->group == group)
        {
            v.erase(i);
            return;                              // first match only, and it stops there
        }
}

// NO guard ON `team` - see the block above. Returns the ids as a list, built front to back.
std::list<int> ControlPointTemplate::getSupplyGroupNeeded(int team)
{
    std::list<int> out;
    std::vector<SupplyGroup>& v = m_supplyGroupsNeeded[team - 1];
    for (std::vector<SupplyGroup>::iterator i = v.begin(); i != v.end(); ++i)
        out.insert(out.end(), i->group);
    return out;
}

// A duplicate IS process-fatal, AS shipped. The diagnostic is Debug type 6, which this tree has
// already measured as the unignorable kind: it routes to displayErrorWindow and exits(1). So a
// .con file that lists the same supply group twice on one control point kills the server rather
// than warning. Preserved deliberately - the fidelity rule covers dice's bugs too.
// The Debug call is also where this file's real path came from: it passes
// "Game/Objects/Game/ControlPoint.cpp" as __FILE__ with line 616.
void ControlPointTemplate::addSupplyGroupNeeded(int team, int group, bool needed)
{
    if (static_cast<unsigned int>(team - 1) > 1)
        return;

    std::vector<SupplyGroup>& v = m_supplyGroupsNeeded[team - 1];
    for (std::vector<SupplyGroup>::iterator i = v.begin(); i != v.end(); ++i)
        if (i->group == group)
        {
            Debug(DEBUGTYPE_6, "Game/Objects/Game/ControlPoint.cpp", 616, __FUNCTION__)
                << "Supply group " << group << " already added.";
            return;
        }

    SupplyGroup g;
    g.group = group;
    g.needed = needed;
    v.push_back(g);
}

// Precaches this object through ObjectTemplate, then the flags for teams 0 through 2.
// Empty authored names use GameLogic's team flags. Supply arrays use a different team index range.
void ControlPointTemplate::preCache() const
{
    ObjectTemplate::preCache();                  // the grandparent - skips BundleTemplate's

    for (int team = 0; team <= 2; ++team)
    {
        const std::string& name = m_flagTemplateTeam[team].empty()
                                ? gameLogic->getTeamFlag(team)       // ds:0x8ACBFA0, slot +0x204
                                : m_flagTemplateTeam[team];

        IObjectTemplate* t = objectTemplateManager->getTemplate(name);
        IObject* obj = t ? t->createObject() : 0;                    // template slot +0x54
        if (obj)
        {
            obj->init();                                             // IObject slot +0x20
            obj->forceDestroy();                                     // IObject slot +0x38 (tail)
        }
    }
}


// Creates a control point from this template.
IObject* ControlPointTemplate::createObject() const
{
    return new ControlPoint(this);
}

// . Retail emits the base template name, not
// controlPointName, and deliberately ignores the final stream write count.
bool ControlPointTemplate::makeScript(io::IStream* stream)
{
    if (!BundleTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    if (!m_templateName.empty()) out << "ObjectTemplate.setControlPointName " << m_templateName << std::endl;
    if (radius != 1.0f) out << "ObjectTemplate.radius " << radius << std::endl;
    const float eps = 1.19209290e-7f;
    if (!(std::fabs(radiusOffset.x) <= eps && std::fabs(radiusOffset.y) <= eps && std::fabs(radiusOffset.z) <= eps))
        out << "ObjectTemplate.radiusOffset " << radiusOffset.x << '/' << radiusOffset.y << '/' << radiusOffset.z << std::endl;
    if (startTeam) out << "ObjectTemplate.team " << startTeam << std::endl;
    if (controlPointId != -1) out << "ObjectTemplate.controlPointId " << controlPointId << std::endl;
    if (areaValueTeam1) out << "ObjectTemplate.areaValueTeam1 " << areaValueTeam1 << std::endl;
    if (areaValueTeam2) out << "ObjectTemplate.areaValueTeam2 " << areaValueTeam2 << std::endl;
    if (enemyTicketLossWhenCaptured) out << "ObjectTemplate.enemyTicketLossWhenCaptured " << enemyTicketLossWhenCaptured << std::endl;
    if (timeToGetControl != 10.0f) out << "ObjectTemplate.timeToGetControl " << timeToGetControl << std::endl;
    if (timeToLoseControl != 10.0f) out << "ObjectTemplate.timeToLoseControl " << timeToLoseControl << std::endl;
    if (disableIfEnemyInsideRadius) out << "ObjectTemplate.disableIfEnemyInsideRadius " << disableIfEnemyInsideRadius << std::endl;
    if (disableWhenLosingControl) out << "ObjectTemplate.disableWhenLosingControl " << disableWhenLosingControl << std::endl;
    if (!loseControlWhenEnemyClose) out << "ObjectTemplate.loseControlWhenEnemyClose " << loseControlWhenEnemyClose << std::endl;
    if (loseControlWhenNotClose) out << "ObjectTemplate.loseControlWhenNotClose " << loseControlWhenNotClose << std::endl;
    if (minNrToTakeControl != 1) out << "ObjectTemplate.minNrToTakeControl " << minNrToTakeControl << std::endl;
    if (unableToChangeTeam) out << "ObjectTemplate.unableToChangeTeam " << unableToChangeTeam << std::endl;
    if (!showOnMinimap) out << "ObjectTemplate.showOnMinimap " << showOnMinimap << std::endl;
    if (isUnstrategicControlPoint) out << "ObjectTemplate.isUnstrategicControlPoint " << isUnstrategicControlPoint << std::endl;
    if (isHemisphere) out << "ObjectTemplate.isHemisphere " << isHemisphere << std::endl;
    if (forSoldierOnly) out << "ObjectTemplate.ForSoldierOnly " << forSoldierOnly << std::endl;
    if (hasGreyCapturingState) out << "ObjectTemplate.HasGreyCapturingState 1" << std::endl;
    // test/parity and ELF 0x81707CF fucomip/jb emit unordered too.
    if (!(std::fabs(minimapNameOffset.x) + std::fabs(minimapNameOffset.y) <= eps))
        out << "ObjectTemplate.MinimapNameOffset " << minimapNameOffset.x << '/' << minimapNameOffset.y << std::endl;
    if (groupId != -1) out << "ObjectTemplate.supplyGroupId " << groupId << std::endl;
    if (signalTriggerId != -1) out << "ObjectTemplate.signalTriggerId " << signalTriggerId << std::endl;
    for (int team = 0; team < 2; ++team)
    {
        for (const SupplyGroup& group : m_supplyGroupsNeeded[team])
            out << "ObjectTemplate.supplyGroupNeeded " << team + 1 << ' ' << group.group << ' ' << group.needed << std::endl;
        if (m_supplyValueNeeded[team]) out << "ObjectTemplate.supplyValueNeededTeam" << team + 1 << ' ' << m_supplyValueNeeded[team] << std::endl;
        if (m_supplyValue[team]) out << "ObjectTemplate.supplyValueTeam" << team + 1 << ' ' << m_supplyValue[team] << std::endl;
    }
    if (!hoistFlag) out << "ObjectTemplate.hoistFlag " << hoistFlag << std::endl;
    out << "ObjectTemplate.hoistMinMax " << hoistMinMax.x << '/' << hoistMinMax.y << std::endl;
    for (int team = 0; team < 3; ++team)
        if (!m_flagTemplateTeam[team].empty()) out << "ObjectTemplate.flagTemplateTeam" << team << ' ' << m_flagTemplateTeam[team] << std::endl;
    const std::string text = out.str();
    stream->write(text.c_str(), static_cast<unsigned int>(text.size()));
    return true;
}

} // namespace world
} // namespace hfe
} // namespace dice
