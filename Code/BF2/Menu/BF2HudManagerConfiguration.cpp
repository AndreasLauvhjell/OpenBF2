// HUD configuration methods. [INFERRED] Split from the original manager translation unit.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Ref2System.h"
#include "HudInformationAdapter.h"
#include "HudRadio.h"

namespace dice { namespace hfe { namespace hud {

bool BF2HudManager::setPointerSize(int width, int height)
{
    m_field154 = static_cast<float>(width);
    m_field158 = static_cast<float>(height);
    return true;
}

// Each shipped setter owns its string argument, conditionally loads a texture,
// and returns true even when the UI system is absent. No old-handle release.
bool BF2HudManager::setMouseTextureEmpty(std::string name)
{
    if (ref2System) m_field164 = ref2System->loadTexture(name.c_str(), true);
    return true;
}
bool BF2HudManager::setMouseTextureFull(std::string name)
{
    if (ref2System) m_field168 = ref2System->loadTexture(name.c_str(), true);
    return true;
}
bool BF2HudManager::setMouseTextureArtillery(std::string name)
{
    if (ref2System) m_field16c = ref2System->loadTexture(name.c_str(), true);
    return true;
}
bool BF2HudManager::setMouseTextureUAV(std::string name)
{
    if (ref2System) m_field170 = ref2System->loadTexture(name.c_str(), true);
    return true;
}
bool BF2HudManager::setMouseTextureSupply(std::string name)
{
    if (ref2System) m_field174 = ref2System->loadTexture(name.c_str(), true);
    return true;
}
bool BF2HudManager::setMouseTextureCancel(std::string name)
{
    if (ref2System) m_field178 = ref2System->loadTexture(name.c_str(), true);
    return true;
}

bool BF2HudManager::setCommMouseSensitivity(int value)
{
    m_field17c = value;
    return true;
}
bool BF2HudManager::setPointerMouseSensitivity(int value)
{
    m_field180 = value;
    return true;
}
bool BF2HudManager::setCommPos(int x, int y)
{
    m_field208 = static_cast<float>(x);
    m_field20c = static_cast<float>(y);
    return true;
}
bool BF2HudManager::setCommSize(int width, int height)
{
    m_field200 = static_cast<float>(width);
    m_field204 = static_cast<float>(height);
    return true;
}
bool BF2HudManager::setCommMousePos(int x, int y)
{
    m_field1f8 = static_cast<float>(x);
    m_field1fc = static_cast<float>(y);
    return true;
}
bool BF2HudManager::setSpottedMenuPos(int x, int y)
{
    m_field220 = static_cast<float>(x);
    m_field224 = static_cast<float>(y);
    return true;
}
bool BF2HudManager::setSpottedMenuSize(int width, int height)
{
    m_field218 = static_cast<float>(width);
    m_field21c = static_cast<float>(height);
    return true;
}
bool BF2HudManager::setSpottedMousePos(int x, int y)
{
    m_field210 = static_cast<float>(x);
    m_field214 = static_cast<float>(y);
    return true;
}
void BF2HudManager::setMaximumNrOfCPs(int value) // 7519A0; console return "void"
{
    m_field150 = value;
}
int BF2HudManager::getMaximumNrOfCPs() // 5FFEE0; [INFERRED] name
{
    return m_field150;
}

bool BF2HudManager::getPaint() { return m_field248; }
bool BF2HudManager::getUpdate() { return m_field249; }

meme::BfEditData* BF2HudManager::getEditData(int selector)
{
    switch (selector) {
    case 1: return m_field0b4[0];
    case 2: return m_field0e0;
    case 3: return m_field0b4[1];
    default: return 0;
    }
}
meme::Data* BF2HudManager::getStringData(int selector)
{
    switch (selector) {
    case 1: return m_field0a8[0];
    case 2: return m_field0dc;
    case 3: return m_field0a8[1];
    default: return 0;
    }
}

void BF2HudManager::setMode(int mode, bool savePrevious)
{
    if (mode == getMode()) return;
    // getMode may mutate the stored state: retail reads the field after calling.
    if (savePrevious) m_field254 = m_field250;
    m_field250 = mode;
    if (m_field104) m_field104->setHudMode(mode);
}

// Keep virtual dispatch and the repeated mode queries used by these wrappers.
void BF2HudManager::toggleMode(int first, int second)
{
    if (getMode() == first) setMode(second, true);
    else setMode(first, true);
}
int BF2HudManager::getMode() { return m_field250; }
int BF2HudManager::getPreviousMode() { return m_field254; }
bool BF2HudManager::unknown1E8()
{
    getMode(); // Shipped discarded virtual call precedes unconditional true.
    return true;
}
bool BF2HudManager::isCommanderShown() // 74FDD0, no adapter null guard
{
    return m_field104->getBool("CommanderShow");
}
bool BF2HudManager::wasCommanderMode()
{
    const int mode = getPreviousMode();
    return mode == 15 || mode == 21 || mode == 16 || mode == 22 ||
           mode == 24 || mode == 25 || mode == 23;
}
bool BF2HudManager::isCommanderMode(int mode)
{
    return mode == 15 || mode == 21 || mode == 16 || mode == 22 ||
           mode == 24 || mode == 23 || mode == 25 ||
           ((mode == 27 || isChatActive() || mode == 9 || mode == 12) &&
            wasCommanderMode());
}
bool BF2HudManager::isScoreboardMode()
{
    const int mode = getMode();
    return mode == 9 || mode == 12;
}
bool BF2HudManager::isOrderMode()
{
    const int mode = getMode();
    return mode == 27 || mode == 21 || mode == 16 || mode == 20 || mode == 19;
}
bool BF2HudManager::activateRadio(HudRadio* radio)
{
    if (!radio) return false;
    radio->reset();
    radio->setActive(true);
    radio->setEnabled(false);
    return true;
}
void BF2HudManager::resetRadio(HudRadio* radio) // 750230, no radio null guard
{
    if (getMode() != 15) setMode(0, true);
    radio->setEnabled(true);
    radio->setActive(false);
}
void BF2HudManager::setSpottedMenuActive()
{
    setMode(6, false);
}

bool BF2HudManager::updateRadioMode()
{
    // The first disabled/inactive radio terminates the search even when no mode
    // changes. Keep repeated getMode/getPreviousMode dispatches and exact order.
    if (m_field108 && !m_field108->isEnabled() && !m_field108->isActive()) {
        if (getMode() == 4 && getPreviousMode() != 4)
            setMode(getPreviousMode(), true);
        if (getMode() == 31 && getPreviousMode() != 31)
            setMode(getPreviousMode(), true);
        if (getMode() == 30 && getPreviousMode() != 30)
            setMode(getPreviousMode(), true);
        return true;
    }
    if (m_field10c && !m_field10c->isEnabled() && !m_field10c->isActive()) {
        if (getMode() == 5 && getPreviousMode() != 5)
            setMode(getPreviousMode(), true);
        return true;
    }
    if (m_field110 && !m_field110->isEnabled() && !m_field110->isActive()) {
        if (getMode() == 3 && getPreviousMode() != 3)
            setMode(getPreviousMode(), true);
        return true;
    }
    if (m_field114 && !m_field114->isEnabled() && !m_field114->isActive()) {
        if (getMode() == 7 && getPreviousMode() != 7)
            setMode(getPreviousMode(), true);
        return true;
    }
    if (m_field118 && !m_field118->isEnabled() && !m_field118->isActive()) {
        if (getMode() == 8 && getPreviousMode() != 8)
            setMode(getPreviousMode(), true);
        return true;
    }
    return false;
}

HudQuestionManager* BF2HudManager::getQuestionManager() { return m_field100; }
Bf2HudBuilder* BF2HudManager::getBuilder() { return m_field0fc; }
HudScoreboard* BF2HudManager::getScoreboard() { return m_field120; }
HudVoip* BF2HudManager::getVoip() { return m_field124; }
HudVoteMap* BF2HudManager::getVoteMap() { return m_field128; }
HudRadio* BF2HudManager::getSquadRadio() { return m_field110; }
HudRadio* BF2HudManager::getVehicleRadio() { return m_field10c; }
HudRadio* BF2HudManager::getInfantryRadio() { return m_field108; }
HudRadio* BF2HudManager::getSquadLeaderRadio() { return m_field114; }
HudRadio* BF2HudManager::getCommanderRadio() { return m_field118; }
HudKitSelection* BF2HudManager::getKitSelection() { return m_field11c; }
HudInformationAdapter* BF2HudManager::getInformationAdapter() { return m_field104; }

} } }
#endif
