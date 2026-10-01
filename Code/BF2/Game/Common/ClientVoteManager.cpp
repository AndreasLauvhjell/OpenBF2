// Client voting state, requests, and HUD messages. [INFERRED] source path.

#include "ClientVoteManager.h"
#include "VoteEvent.h"
#include "../IGame.h"
#include "../IGameLogic.h"
#include "../GameLogic/SquadManager/ISquadManager.h"
#include "../GameServer/ServerInfo.h"          // teamVoteOnly - votePertainsToLocalClient (both images);
                                               // m_block.votingEnabled - update(), the PE's serverInfo +0x53
#include "../Main/Game.h"                      // isDedicated() - BF2.exe 440510; a constant `true` under dedicated
#include "../Objects/IPlayer.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../../Core/EventManager.h"
#if !DEDICATED
#include <cmath>                               // displayMessage's std::ceil (463930)
#include <string>
#include "../GeneralSettings.h"                // m_outOfVoting, through the +0x80 slot bridge
#include "../MapList.h"                        // displayMessage's map arms - getEntry (4488C0)
#include "../Main/BF2ClientSeams.h"            // hudManager (ds:0xA10890) - in dice::hfe, typed hud::BF2HudManager*
#include "../../Core/ClassManager.h"           // getSingleton("GeneralSettings")
#include "../../Core/StringUtil.h"             // formatString (4310D0) - "HUD_LEVELNAME_%s"
#include "../../Menu/BF2HudManager.h"          // refreshChatCategory (+0x140), getQuestionManager (+0x218),
                                               // getLocalizedString (+0x38), the three replace slots
                                               // (+0x3C/+0x40/+0x44), addServerChat (+0x130),
                                               // addServerTeamChat (+0x134)
#include "../../Menu/HudQuestionManager.h"     // removeQuestions (789140), addQuestion (7892F0)
#endif

namespace dice {
namespace hfe {
extern IBase* squadManager;
#if !DEDICATED
extern ClassManager* classManager;             // Core/Globals.cpp
#endif

ClientVoteManager* ClientVoteManager::s_instance = 0;     // ds:0x8AACA20
// Counts affirmative ballots through the player manager.
int ClientVoteManager::getNumYesVotesCasted()
{
    int count = 0;
    if (!world::playerManager) return count;
    const auto& players = world::playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it) {
        if (static_cast<int>((*it)->getVotingStatus()) == 2
            || static_cast<int>((*it)->getVotingStatus()) == 1) ++count;
    }
    return count;
}
int ClientVoteManager::getNumNoVotesCasted()
{
    int count = 0;
    if (!world::playerManager) return count;
    const auto& players = world::playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it) {
        if (static_cast<int>((*it)->getVotingStatus()) == 3) ++count;
    }
    return count;
}


// Initializes two strings, nine scalar words, and two bools, leaving padding untouched.
ClientVoteManager::ClientVoteManager()
{
    m_dword04 = 0;
#if DEDICATED
    m_dword08 = 254;
#endif
    m_votesNeeded = 0;
    m_voteTimeRemaining = 0;
    m_disallowedTime = 0;
    m_voteCaster = 0;
    m_reminderTimer = 0;
    m_promptTimer = 0;
    m_localTeam = 0;
#if DEDICATED
    m_messagesEnabled = true;
    m_messageDisplayed = false;
#else
    m_messageDisplayed = false;
    m_dword08 = 254;
    m_messagesEnabled = true;
#endif
    // m_string0C / m_string14 default-construct themselves.
}

// . lazy AND never freed - no atexit, and nothing on the server path calls the
// release() that exists beside it.
ClientVoteManager* ClientVoteManager::getInstance()
{
    if (!s_instance)
        s_instance = new ClientVoteManager();     // operator new(0x30)
    return s_instance;
}

// Deletes and clears the singleton. GameLogic teardown may create it immediately before releasing it.
void ClientVoteManager::release()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = 0;                               // ds:0x8AACA20
    }
}

// Requests the current ballot for a joining or team-changing player. Dedicated
// images do nothing; listen servers post locally and clients send using the passed id.
void ClientVoteManager::requestStateUpdate(int playerId)
{
    if (isDedicated())
        return;
#if !DEDICATED
    VoteEvent* const event = new VoteEvent;               // : operator new(0x24)
    event->m_voteType   = 0;                              // : +0x18
    event->m_voteStatus = 9;                              // : +0x14 - VSClientState
    event->m_voteCaster = 254;                            // : +0x21, raw
    event->setVoteItem(playerId);

    if (gameLogic->isServer())                            // : +0x13C
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);   // : +0x18
    else
        game->sendGameEvent(playerId, event);             // : +0x8C = ELF +0x90
#else
    (void)playerId;
#endif
}

// Apply vote broadcasts and update client prompts; dedicated servers do nothing.
// Requires the local human for issue/discard paths. Issue notifications activate
// the vote before resolving its issuer and display the prompt before marking
// the issuer's ballot. [UNRESOLVED] PlayerVotingStatus names remain numeric.
void ClientVoteManager::receiveVote(const VoteEvent& event)
{
    if (isDedicated())
        return;
#if !DEDICATED
    if (!world::playerManager)                                            // : ds:0xA08F60
        return;

    switch (event.m_voteStatus)                                           // : +0x14
    {
    case Vote::VSIssue:
    {
        if (!votePertainsToLocalClient(event))
            return;
        resetActiveVote();
        m_dword04 = event.m_voteType;                                     // : +0x04 <- +0x18
        m_dword08 = event.m_voteItem;                                     // : +0x08 <- +0x20
        m_string0C.clear();                                               // : msvcp71 clear()
        m_voteTimeRemaining = event.m_time;                               // : +0x54 <- +0x1C
        m_voteCaster = event.m_voteCaster;                                // : +0x28 <- +0x21
        m_votesNeeded = event.m_votesNeeded;                              // : +0x50 <- +0x12

        if (voteTypeRequiresTargetPlayer(static_cast<Vote::VoteType>(m_dword04)))
        {
            world::IPlayer* const target =
                world::playerManager->getPlayerFromId(event.m_voteItem);  // : +0x20
            if (target)
            {
                target->setTargetOfVote(true);                            // : PE +0x2A0 = ELF +0x2A4
                m_string0C = target->getName();                           // : PE +0x20, operator=
            }
        }

        // The issuer's own client starts the 300 s cooldown sendKickIssue / sendMapIssue /
        // sendMutinyIssue test through getDisallowedForTimePeriod. getLocalHumanPlayer() unguarded.
        if (world::playerManager->getLocalHumanPlayer()->getId() == event.m_voteCaster)
            m_disallowedTime = 300.0f;

        world::IPlayer* const caster =
            world::playerManager->getPlayerFromId(event.m_voteCaster);    // : +0x20
        if (!caster)
            return;
        m_string14 = caster->getName();                                   // : +0x2C (PE), operator=
        displayMessage(static_cast<ClientVoteMessage>(7), true);
        caster->setVotingStatus(static_cast<world::PlayerVotingStatus>(1));   // : PE +0x290
        return;
    }

    case Vote::VSFailed:
        displayMessage(static_cast<ClientVoteMessage>(10), true);
        resetActiveVote();
        return;

    case Vote::VSPrematureEnd:
    {
        world::IPlayer* const target =
            world::playerManager->getPlayerFromId(event.m_voteItem);      // : +0x20
        if (m_dword04 == Vote::VTMutiny && target)
        {
            target->setCommanderReapplyWaitTime(5.0);                     // : PE +0x12C, ds:89C1A0
            displayMessage(static_cast<ClientVoteMessage>(13), true);
        }
        else
        {
            displayMessage(static_cast<ClientVoteMessage>(12), true);
        }
        resetActiveVote();
        return;
    }

    case Vote::VSPassed:
        if (m_dword04 == Vote::VTMutiny)                                  // : +0x04
        {
            world::IPlayer* const target =
                world::playerManager->getPlayerFromId(event.m_voteItem);  // : +0x20
            if (target)
                target->setCommanderReapplyWaitTime(5.0);                 // : PE +0x12C
        }
        displayMessage(static_cast<ClientVoteMessage>(9), true);
        resetActiveVote();
        return;

    case Vote::VSUpdate:
    {
        world::IPlayer* const caster =
            world::playerManager->getPlayerFromId(event.m_voteCaster);    // : +0x20
        if (!caster)
            return;
        const int option = event.m_voteOption;                            // : +0x0C
        if (option == Vote::VOFor)
            caster->setVotingStatus(static_cast<world::PlayerVotingStatus>(2));
        else if (option == Vote::VOAgainst)
            caster->setVotingStatus(static_cast<world::PlayerVotingStatus>(3));
        else if (option == Vote::VONotVoted)
            return;
        if (caster == world::playerManager->getLocalHumanPlayer())        // : +0x30
            displayMessage(static_cast<ClientVoteMessage>(11), false);
        return;
    }

    case Vote::VSDiscardTarget:
        if (world::playerManager->getLocalHumanPlayer()->getId() == event.m_voteCaster)
            displayMessage(static_cast<ClientVoteMessage>(3), false);
        return;

    case Vote::VSDiscardConflict:
        if (world::playerManager->getLocalHumanPlayer()->getId() == event.m_voteCaster)
            displayMessage(static_cast<ClientVoteMessage>(1), false);
        return;

    case Vote::VSTooFewPlayers:
        if (world::playerManager->getLocalHumanPlayer()->getId() == event.m_voteCaster)
            displayMessage(static_cast<ClientVoteMessage>(6), false);
        return;

    case Vote::VSParticipation:
    {
        world::IPlayer* const player =
            world::playerManager->getPlayerFromId(event.m_voteCaster);    // : +0x20
        if (player)
            player->setParticipateInVoting(event.m_voteOption == 0);      // : PE +0x298
        return;
    }

    default:                                                              // : status > 8
        return;
    }
#else
    (void)event;
#endif
}

// Kick and mutiny votes require a player target; map votes do not.
bool ClientVoteManager::voteTypeRequiresTargetPlayer(Vote::VoteType type)
{
    return type == Vote::VTKickPlayer || type == Vote::VTMutiny;
}

// Tests whether a vote concerns the local player: all map votes, unrestricted kick
// votes, and team-restricted kick/mutiny votes targeting a teammate. Requires the
// player manager and local player. [PLATFORM] Uses the ELF's local-team-first read order.
bool ClientVoteManager::votePertainsToLocalClient(const VoteEvent& event)
{
    switch (event.m_voteType)                                              // +0x18
    {
    case Vote::VTMap:
        return true;
    case Vote::VTKickPlayer:
        if (serverInfo && !serverInfo->m_block.teamVoteOnly)               // !getTeamVoteOnly()
            return true;
        // fall through - a team-only kick vote concerns the target's team only
    case Vote::VTMutiny:
    {
        world::IPlayer* const target = world::playerManager->getPlayerFromId(event.m_voteItem);
        if (!target)
            return false;
        return world::playerManager->getLocalHumanPlayer()->getTeam() == target->getTeam();
    }
    default:
        return false;
    }
}

// Full ELF8128D10 and client599460's inlined first gate. The local-player
// result is deliberately unguarded; only the manager itself is checked.
bool ClientVoteManager::participatesInVoting()
{
    return world::playerManager
        && world::playerManager->getLocalHumanPlayer()->getParticipateInVoting();
}

bool ClientVoteManager::getDisallowedForTimePeriod() // ELF8128320/client5977B0
{
    return m_disallowedTime > 0.0f;
}

// Displays vote messages and ballots when enabled and the player is participating, unless overridden.
// Dedicated mode is inert. Client paths require HUD and local-player services.
// Result/reminder messages remove existing vote questions; repeated token substitutions
// replace only the first match each time. Names stop at embedded null characters.
void ClientVoteManager::displayMessage(ClientVoteMessage message, bool evenIfNotParticipating)
{
    if (isDedicated())
        return;
#if !DEDICATED
    if (!m_messagesEnabled)                                               // : +0x00
        return;
    if (!participatesInVoting() && !evenIfNotParticipating)
        return;

    switch (static_cast<int>(message))                                    // : `cmp eax, 0Dh; ja`
    {
    // Compute cooldown minutes with float multiplication before ceil; rounding can
    // raise exact minute boundaries such as 180 seconds to four minutes.
    case 0:
        hudManager->addServerChat(
            hudManager->replaceWideTokenFromInt(
                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_UNABLE_TO_ISSUE"),
                "#MINUTES#",
                static_cast<int>(std::ceil(m_disallowedTime * (1.0f / 60.0f)))),
            0);
        break;

    // 1, 2, 3, 4, 6: one fixed line each
    case 1:
        hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_IN_PROGRESS"), 0);
        break;
    case 2:
        hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_CANNOT_KICK_SELF"), 0);
        break;
    case 3:
        hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_NOT_IN"), 0);
        break;
    case 4:
        hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_CANNOT_KICK_HOST"), 0);
        break;
    // 5: no arm - its jump-table slot (599268) is the function's exit, the same as `default`.
    case 6:
        hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_UNABLE_TO_START_VOTE"), 0);
        break;

    // Issuer and target receive a local notification; other players receive a ballot.
    // Matches the local player by name, testing issuer before target.
    case 7:
    {
        const int minutes = static_cast<int>(std::ceil(m_voteTimeRemaining * (1.0f / 60.0f)));
        if (m_dword04 == Vote::VTKickPlayer)
        {
            if (m_string14 == world::playerManager->getLocalHumanPlayer()->getName()
                || m_string0C == world::playerManager->getLocalHumanPlayer()->getName())
            {
                if (hudManager->getQuestionManager())
                    hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
                hudManager->refreshChatCategory(8);
                hudManager->addServerTeamChat(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromString(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_START_LOCAL"),
                                "#PLAYERNAME1#", m_string14.c_str()),
                            "#PLAYERNAME2#", m_string0C.c_str()),
                        "#MINUTES#", minutes),
                    0);
                m_messageDisplayed = true;
            }
            else if (hudManager->getQuestionManager())
            {
                hudManager->getQuestionManager()->addQuestion(6, m_dword04, true);
                m_messageDisplayed = false;
            }
        }
        else if (m_dword04 == Vote::VTMutiny)
        {
            if (m_string14 == world::playerManager->getLocalHumanPlayer()->getName()
                || m_string0C == world::playerManager->getLocalHumanPlayer()->getName())
            {
                if (hudManager->getQuestionManager())
                    hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
                hudManager->refreshChatCategory(8);
                hudManager->addServerTeamChat(                            //  (shared tail)
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromString(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_COMMANDER_START_LOCAL"),
                                "#PLAYERNAME1#", m_string14.c_str()),
                            "#PLAYERNAME2#", m_string0C.c_str()),
                        "#MINUTES#", minutes),
                    0);
                m_messageDisplayed = true;
            }
            else if (hudManager->getQuestionManager())                    //  (shared)
            {
                hudManager->getQuestionManager()->addQuestion(6, m_dword04, true);
                m_messageDisplayed = false;
            }
        }
        else if (m_dword04 == Vote::VTMap)
        {
            const MapList::Entry* const entry = mapList->getEntry(m_dword08);
            std::string mapName;
            if (entry)
                mapName = entry->name;                                    // : operator=
            if (m_string14 == world::playerManager->getLocalHumanPlayer()->getName()
                || m_string0C == world::playerManager->getLocalHumanPlayer()->getName())
            {
                if (hudManager->getQuestionManager())
                    hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
                hudManager->refreshChatCategory(8);
                const std::string levelKey =
                    formatString("HUD_LEVELNAME_%s", mapName.c_str());
                hudManager->addServerTeamChat(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideToken(
                            hudManager->replaceWideTokenFromString(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_MAP_START_LOCAL"),
                                "#PLAYERNAME#", m_string14.c_str()),
                            "#MAPNAME#", hudManager->getLocalizedString(levelKey.c_str())),
                        "#MINUTES#", minutes),
                    0);
                m_messageDisplayed = true;
            }
            else if (hudManager->getQuestionManager())
            {
                hudManager->getQuestionManager()->addQuestion(6, m_dword04, true);
                m_messageDisplayed = false;
            }
        }
        break;
    }

    // 8: the 30 s reminder - the running tally
    case 8:
    {
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
        hudManager->refreshChatCategory(8);
        const int yesVotes = getNumYesVotesCasted();
        if (m_dword04 == Vote::VTKickPlayer)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromInt(
                    hudManager->replaceWideTokenFromString(
                        hudManager->replaceWideTokenFromInt(
                            hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_STAT"),
                            "#YESVOTES#", yesVotes),
                        "#PLAYERNAME#", m_string0C.c_str()),
                    "#NEEDED#", m_votesNeeded),
                0);
        }
        else if (m_dword04 == Vote::VTMutiny)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromInt(
                    hudManager->replaceWideTokenFromString(
                        hudManager->replaceWideTokenFromInt(
                            hudManager->getLocalizedString("HUD_HUD_KICKVOTE_COMMANDER_STAT"),
                            "#YESVOTES#", yesVotes),
                        "#PLAYERNAME#", m_string0C.c_str()),
                    "#NEEDED#", m_votesNeeded),
                0);
        }
        else if (m_dword04 == Vote::VTMap)
        {
            const MapList::Entry* const entry = mapList->getEntry(m_dword08);
            std::string mapName;
            if (entry)
                mapName = entry->name;
            const std::string levelKey = formatString("HUD_LEVELNAME_%s", mapName.c_str());
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromInt(
                    hudManager->replaceWideToken(
                        hudManager->replaceWideTokenFromInt(
                            hudManager->getLocalizedString("HUD_HUD_KICKVOTE_MAP_STAT"),
                            "#YESVOTES#", yesVotes),
                        "#MAPNAME#", hudManager->getLocalizedString(levelKey.c_str())),
                    "#NEEDED#", m_votesNeeded),
                0);
        }
        break;
    }

    // 9: the vote passed
    case 9:
    {
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
        hudManager->refreshChatCategory(8);
        const int yesVotes = getNumYesVotesCasted();
        if (m_dword04 == Vote::VTKickPlayer)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromString(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromInt(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_KICKED"),
                                "#YESVOTES#", yesVotes),
                            "#PLAYERNAME#", m_string0C.c_str()),
                        "#NEEDED#", m_votesNeeded),
                    "#PLAYERNAME#", m_string0C.c_str()),
                0);
        }
        else if (m_dword04 == Vote::VTMutiny)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromString(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromInt(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_COMMANDER_KICKED"),
                                "#YESVOTES#", yesVotes),
                            "#PLAYERNAME#", m_string0C.c_str()),
                        "#NEEDED#", m_votesNeeded),
                    "#PLAYERNAME#", m_string0C.c_str()),
                0);
        }
        else if (m_dword04 == Vote::VTMap)
        {
            const MapList::Entry* const entry = mapList->getEntry(m_dword08);
            std::string mapName;
            if (entry)
                mapName = entry->name;
            const std::string levelKey = formatString("HUD_LEVELNAME_%s", mapName.c_str());
            // the map name is localized twice (598D52, 598DA4) - once per #mapname#.
            hudManager->addServerChat(
                hudManager->replaceWideToken(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideToken(
                            hudManager->replaceWideTokenFromInt(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_MAP_CHANGE"),
                                "#YESVOTES#", yesVotes),
                            "#MAPNAME#", hudManager->getLocalizedString(levelKey.c_str())),
                        "#NEEDED#", m_votesNeeded),
                    "#MAPNAME#", hudManager->getLocalizedString(levelKey.c_str())),
                0);
        }
        break;
    }

    // 10: the vote failed
    case 10:
    {
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
        hudManager->refreshChatCategory(8);
        const int yesVotes = getNumYesVotesCasted();
        if (m_dword04 == Vote::VTKickPlayer)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromString(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromInt(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_STAYS"),
                                "#YESVOTES#", yesVotes),
                            "#PLAYERNAME#", m_string0C.c_str()),
                        "#NEEDED#", m_votesNeeded),
                    "#PLAYERNAME#", m_string0C.c_str()),
                0);
        }
        else if (m_dword04 == Vote::VTMutiny)
        {
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromString(
                    hudManager->replaceWideTokenFromInt(
                        hudManager->replaceWideTokenFromString(
                            hudManager->replaceWideTokenFromInt(
                                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_COMMANDER_STAYS"),
                                "#YESVOTES#", yesVotes),
                            "#PLAYERNAME#", m_string0C.c_str()),
                        "#NEEDED#", m_votesNeeded),
                    "#PLAYERNAME#", m_string0C.c_str()),
                0);
        }
        else if (m_dword04 == Vote::VTMap)
        {
            const MapList::Entry* const entry = mapList->getEntry(m_dword08);
            std::string mapName;
            if (entry)
                mapName = entry->name;
            const std::string levelKey = formatString("HUD_LEVELNAME_%s", mapName.c_str());
            // ONE #mapname# here, unlike case 9's two.
            hudManager->addServerChat(
                hudManager->replaceWideTokenFromInt(
                    hudManager->replaceWideToken(
                        hudManager->replaceWideTokenFromInt(
                            hudManager->getLocalizedString("HUD_HUD_KICKVOTE_MAP_STAYS"),
                            "#YESVOTES#", yesVotes),
                        "#MAPNAME#", hudManager->getLocalizedString(levelKey.c_str())),
                    "#NEEDED#", m_votesNeeded),
                0);
        }
        break;
    }

    // 11: our own ballot, echoed
    // world::playerManager is NOT re-tested: 597D9E reloads it after the participation test, and a
    //   caller passing true with no playerManager faults here. receiveVote passes false, after its
    //   own test.
    case 11:
    {
        const int status = static_cast<int>(
            world::playerManager->getLocalHumanPlayer()->getVotingStatus());   // : PE +0x294
        if (status == 2 || status == 1)
            hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_VOTE_YES"), 0);
        else if (status == 3)
            hudManager->addServerChat(hudManager->getLocalizedString("HUD_HUD_KICKVOTE_VOTE_NO"), 0);
        break;
    }

    // 12 / 13: the vote ended early (the target left / the commander resigned)
    case 12:
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
        hudManager->refreshChatCategory(8);
        getNumYesVotesCasted();                                           //  answer dropped
        hudManager->addServerChat(                                        //  (shared tail)
            hudManager->replaceWideTokenFromString(
                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_PLAYER_NOT_IN_GAME"),
                "#PLAYERNAME#", m_string0C.c_str()),
            0);
        break;
    case 13:
        if (hudManager->getQuestionManager())
            hudManager->getQuestionManager()->removeQuestions(6, -1, true, 10.0f);
        hudManager->refreshChatCategory(8);
        hudManager->addServerChat(                                        //  (shared tail)
            hudManager->replaceWideTokenFromString(
                hudManager->getLocalizedString("HUD_HUD_KICKVOTE_COMMANDER_NOT_IN_GAME"),
                "#PLAYERNAME#", m_string0C.c_str()),
            0);
        break;

    default:                                                              // : message > 13
        break;
    }
#else
    (void)message;
    (void)evenIfNotParticipating;
#endif
}

// ELF81285A0/client599460. Preserve all local-player/global reloads, the
// missing player/result guards, and the uninitialized fields in the new event.
bool ClientVoteManager::sendMutinyIssue()
{
    if (!participatesInVoting() || !world::playerManager || m_dword04) return false;
    if (getDisallowedForTimePeriod()) {
        displayMessage(static_cast<ClientVoteMessage>(0), false);
        return false;
    }
    const int team = world::playerManager->getLocalHumanPlayer()->getTeam();
    const int commander = static_cast<ISquadManager*>(squadManager)->getCommander(team);
    VoteEvent* event = new VoteEvent;
    event->m_voteType = 3;
    event->m_voteStatus = 0;
    event->m_voteCaster = 254;
    event->setVoteItem(commander);
    event->setVoteCaster(world::playerManager->getLocalHumanPlayer()->getId());
    if (gameLogic->isServer()) {
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);
    } else {
        const int playerId = world::playerManager->getLocalHumanPlayer()->getId();
        game->sendGameEvent(playerId, event);
    }
    return true;
}

// Requests a kick vote when participating and no vote is active. Rejects self,
// host IDs, bots, and cooldown; bot refusal is silent. Requires a local player.
// Success clears m_messageDisplayed so the new prompt can appear.
bool ClientVoteManager::sendKickIssue(int playerId)
{
    if (!participatesInVoting() || !world::playerManager || m_dword04) return false;
    if (playerId == world::playerManager->getLocalHumanPlayer()->getId()) {
        displayMessage(static_cast<ClientVoteMessage>(2), false);
        return false;
    }
    if (playerId == 255 || playerId == -1) {
        displayMessage(static_cast<ClientVoteMessage>(4), false);
        return false;
    }
    if (world::playerManager) {
        world::IPlayer* const target = world::playerManager->getPlayerFromId(playerId);
        if (target && target->getIsAIPlayer()) return false;
    }
    if (getDisallowedForTimePeriod()) {
        displayMessage(static_cast<ClientVoteMessage>(0), false);
        return false;
    }
    VoteEvent* event = new VoteEvent;
    event->m_voteType = 2;
    event->m_voteStatus = 0;
    event->m_voteCaster = 254;
    event->setVoteItem(playerId);
    event->setVoteCaster(world::playerManager->getLocalHumanPlayer()->getId());
    if (gameLogic->isServer()) {
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);
    } else {
        const int localId = world::playerManager->getLocalHumanPlayer()->getId();
        game->sendGameEvent(localId, event);
    }
    m_messageDisplayed = false;
    return true;
}

// Requests a map vote when participating, with no active vote and after cooldown.
// Cooldown refusal displays a message. Requires a local player; mapIndex is not
// validated and is truncated by setVoteItem. Success leaves m_messageDisplayed unchanged.
bool ClientVoteManager::sendMapIssue(int mapIndex)
{
    if (!participatesInVoting() || !world::playerManager || m_dword04) return false;
    if (getDisallowedForTimePeriod()) {
        displayMessage(static_cast<ClientVoteMessage>(0), false);
        return false;
    }
    VoteEvent* event = new VoteEvent;
    event->m_voteType = 1;
    event->m_voteStatus = 0;
    event->m_voteCaster = 254;
    event->setVoteItem(mapIndex);
    event->setVoteCaster(world::playerManager->getLocalHumanPlayer()->getId());
    if (gameLogic->isServer()) {
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);
    } else {
        const int playerId = world::playerManager->getLocalHumanPlayer()->getId();
        game->sendGameEvent(playerId, event);
    }
    return true;
}

// Complete ELF81289F0/client597B30. No active-vote or local-player guard was
// present beyond these gates. The event retains all unwritten allocation bytes.
bool ClientVoteManager::sendVote(Vote::VoteOption option)
{
    if (!participatesInVoting() || !world::playerManager) return false;
    if ((m_dword04 == 2 || m_dword04 == 3)
        && m_dword08 == world::playerManager->getLocalHumanPlayer()->getId()) return false;
    VoteEvent* event = new VoteEvent;
    event->m_voteType = m_dword04;
    event->m_voteStatus = 4;
    event->m_voteCaster = 254;
    event->m_voteOption = option;
    event->setVoteItem(m_dword08);
    if (gameLogic->isServer()) {
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);
    } else {
        world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
        const int playerId = player->getId();
        game->sendGameEvent(playerId, event);
    }
    m_messageDisplayed = true;
    return true;
}


#if !DEDICATED
// Clears timers, vote type, and every player's ballot/target flags. The manager
// is optional, but individual entries in getPlayers() must be valid.
void ClientVoteManager::resetActiveVote()
{
    m_reminderTimer = 0.0f;                      // : +0x48
    m_promptTimer   = 0.0f;                      // : +0x4C
    m_dword04       = 0;                         // : +0x04

    if (!world::playerManager)
        return;

    const auto& players = world::playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        (*it)->setVotingStatus(static_cast<world::PlayerVotingStatus>(0));   // PE +0x290 = ELF +0x294
        (*it)->setTargetOfVote(false);                                       // PE +0x2A0 = ELF +0x2A4
    }
}

// Sends the inverse of participate as the event option, then stores participate
// on the local player. Requires successful allocation and a local player. Preserve
// the separate player lookups and setVoteItem's -1-to-255 conversion.
void ClientVoteManager::setVotingParticipationStatus(bool participate)
{
    VoteEvent* const event = new VoteEvent;      // operator new(0x24), then the vtable store
    event->m_voteType   = 0;                     // : +0x18
    event->m_voteStatus = 8;                     // : +0x14 - VSParticipation
    event->m_voteCaster = 254;                   // : +0x21, written raw
    event->m_voteOption = (participate == 0);    // : +0x0C  <- the inverse, as shipped

    event->setVoteItem(world::playerManager->getLocalHumanPlayer()->getId());

    if (gameLogic->isServer())                                                 // PE +0x13C
        eventManager->postEventNextUpdate(ECGameLogic, 15, event);             // PE +0x18
    else
        game->sendGameEvent(world::playerManager->getLocalHumanPlayer()->getId(),
                            event);                                            // PE +0x8C = ELF +0x90

    world::playerManager->getLocalHumanPlayer()->setParticipateInVoting(participate);  // PE +0x298
}

// Synchronizes voting enablement, participation and team changes before advancing vote timers.
// Team changes store the new team before resetting commander votes. Requires HUD services
// on active paths; a missing player manager returns immediately.
void ClientVoteManager::update(float dt)
{
    if (!world::playerManager)                                     // : ds:0xA08F60
        return;

    // 2/3 - the server's flag. PE serverInfo +0x53; the ELF's own name for that byte is
    // m_block.votingEnabled (ServerSettings::setVotingEnabled writes both), and the +0x1C delta is
    // exactly std::string (+24) plus std::vector (+4) ahead of it inside ServerInfoBlock.
    const bool votingEnabled = serverInfo->m_block.votingEnabled != 0;
    if (votingEnabled != m_messagesEnabled)
    {
        hudManager->refreshChatCategory(8);                   // : +0x140
        if (hudManager->getQuestionManager())                 // : +0x218
            hudManager->getQuestionManager()
                ->removeQuestions(6, -1, true, 0.0f);
    }
    m_messagesEnabled = votingEnabled;
    if (!votingEnabled)
        return;

    // 4 - the participation re-sync.
    GeneralSettings* const generalSettings = static_cast<GeneralSettings*>(
        classManager->getSingleton(std::string("GeneralSettings")));   // : +0x48
    if (generalSettings)
    {
        if (world::playerManager->getLocalHumanPlayer())            // : +0x30
        {
            const bool participates =
                world::playerManager->getLocalHumanPlayer()->getParticipateInVoting();  // PE +0x29C
            if (generalSettings->getOutOfVoting() == participates)  // : the +0x80 slot
                setVotingParticipationStatus(generalSettings->getOutOfVoting() == 0);   // re-read, as shipped
        }
    }

    // 5 - the local team, re-read every frame.
    if (world::playerManager->getLocalHumanPlayer())
    {
        if (m_localTeam != world::playerManager->getLocalHumanPlayer()->getTeam())      // PE +0xE4
        {
            const int team = world::playerManager->getLocalHumanPlayer()->getTeam();
            const bool duringCommanderVote = (m_dword04 == 3);      // read before the store
            m_localTeam = team;
            if (duringCommanderVote)
                resetActiveVote();
        }
    }

    // 6 - the disallow cooldown. The clamp is `if (!(0.0f < v)) v = 0.0f;`, spelled as the PE's
    // `fld 0.0 / fcomp st(1)` comparison rather than as `v < 0.0f`, so a NaN would be clamped too.
    float disallowed = m_disallowedTime - dt;
    m_disallowedTime = disallowed;                                  // : fst, not fstp
    if (!(0.0f < disallowed))
        disallowed = 0.0f;
    m_disallowedTime = disallowed;

    // 7 - the vote clock and the two message timers.
    if (m_dword04)
    {
        m_voteTimeRemaining -= dt;                                  // : +0x54
        m_reminderTimer     += dt;                                  // : +0x48

        if (!m_messageDisplayed)                                    // : `cmp byte [esi+1], 0`
            m_promptTimer += dt;                                    // : +0x4C
        else
            m_promptTimer = 0.0f;

        if (m_promptTimer >= 5.0f)                                  // : flt_8E615C
        {
            m_promptTimer = 0.0f;
            displayMessage(static_cast<ClientVoteMessage>(7), false);
        }
        if (m_reminderTimer >= 30.0f)                               // : flt_8E6158
        {
            m_reminderTimer = 0.0f;
            displayMessage(static_cast<ClientVoteMessage>(8), false);
        }
    }
}
// Returns false without playerManager; otherwise requires a local human player
// and checks voting availability.
bool ClientVoteManager::votingEnabled()
{
    if (!world::playerManager) return false;
    // PE +0x29C = ELF +0x2A0 getParticipateInVoting.
    if (!world::playerManager->getLocalHumanPlayer()->getParticipateInVoting()) return false;
    return m_messagesEnabled;
}

// Allows a new issue only when voting is enabled and no vote is active.
// A present playerManager must provide a local human player.
bool ClientVoteManager::voteIssueAllowed()
{
    if (!world::playerManager) return false;
    if (!world::playerManager->getLocalHumanPlayer()->getParticipateInVoting()) return false;  // +0x29C
    if (!m_messagesEnabled) return false;                                        // +00
    return m_dword04 == 0;                                                       // +04, no vote running
}

// Defaults to team-only voting before server information arrives.
bool ClientVoteManager::getTeamVoteOnly()
{
    if (!serverInfo) return true;
    return serverInfo->m_block.teamVoteOnly != 0;
}
#endif // !dedicated

} // namespace hfe
} // namespace dice
