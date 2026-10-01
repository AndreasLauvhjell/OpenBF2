// [INFERRED] source path, namespace and names. Mutable client palette storage
// recovered from PE864440/470/4D0/500/800 and8650C0/5660.
#ifndef MENU_HUDCOLORS_H
#define MENU_HUDCOLORS_H
#include "../External/BFMeme/Quad.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace hud {
// [INFERRED] family names; index1 is enemy, index2 is friendly in PE4644A0.
extern meme::Color teamTopColors[4]; // A1CD08..A1CD44, initializer865720
extern meme::Color ticketColors[4];  // A1C958..A1C994, initializer8657E0
extern meme::Color textColors[4];    // A1CDF8..A1CE34, initializer864860
extern meme::Color brightColors[4];  // A1CA18..A1CA54, initializer864B30

extern meme::Color defaultChatColors[4];
extern meme::Color squadChatColors[4];
extern meme::Color commanderChatColor;
extern meme::Color categoryThreeChatColors[4]; // A1C918..A1C954, inferred name

// A1CF18..A1CF54, initializer 864BF0. [INFERRED] name: its only reader in the whole image is
// HudScoreboard::updateFinalList (7A3610), indexed by IPlayer::getTeam().
// It is a near-twin of brightColors above - 864BF0 copies the same 97E1A0 / 97DE70 / 97E000
// sources - except that entry [3] takes 97DE70 (the red again) where brightColors takes 97E190
// (the magenta "unused team" marker). Two separate mutable arrays, not aliases.
extern meme::Color scoreboardColors[4];

// The three palettes HudScoreboard::paintRow (7A38D0) picks between for a row's text. All three are
// indexed by side, not by team number: `(m_team == team) + 1`, so [1] is the enemy and [2] is the
// friendly side - the same convention teamTopColors already documents. [0] and [3] are written by
// the initializers but no call site can reach them.
// [INFERRED] names, from which branch of the painter selects each.
extern meme::Color scoreboardNameColors[4];  // A1C7B8..A1C7F4, initializer 8649D0 - the default
extern meme::Color scoreboardSquadColors[4]; // A1CC78..A1CCB4, initializer 864920 - own squad, or
                                             //   the local player while commanding
extern meme::Color scoreboardDeadColors[4];  // A1CF58..A1CF94, initializer 864A80 - not alive

// Map-marker palettes selected by Bf2MenuMapNode::colorMapIcon. Names are
// [INFERRED]; addresses and values are PE initializer evidence.
extern meme::Color miniControlPointColors[4]; // A1C858, initializer 8654F0; 7CF5A0 only
extern meme::Color mapObjectColors[4];
extern meme::Color mapSelectedColors[4];
extern meme::Color mapWhiteColors[4];
extern meme::Color mapNeutralColors[4];
extern meme::Color mapTimedColors[4];
extern meme::Color mapAlertColors[4];
// Map airstrike palette; the collector reads entry two. [INFERRED] Table name.
extern meme::Color mapAirstrikeColors[4];
// Order colors indexed by SquadOrder::orderId. [INFERRED] Name; [CHOSEN] Shared table.
extern meme::Color mapOrderColors[11];
// [INFERRED] Squad-request colors by request type and display column. Both colors in each row are equal.
extern meme::Color mapSquadRequestColors[4][2];
// [INFERRED] Invite-row palette; another squad's player uses entry two with half opacity.
extern meme::Color squadInviteColors[4];
// [INFERRED] Commander-row palettes indexed by squad selection state.
extern meme::Color commanderIconColors[2];       // A1C6B8, initializer 865DA0 - member/leader icons
extern meme::Color commanderFrameColors[2];      // A1C7F8, initializer 865E60 - the icon frames
extern meme::Color commanderCellColors[2];       // A1CE78, initializer 865E00 - cell backgrounds
extern meme::Color commanderSquadIconColors[2];  // A1D018, initializer 865CE0 - kit_Squad_big
extern meme::Color commanderNumberColors[2];     // A1CA98, initializer 865F20 - the squad number
extern meme::Color commanderRowFrameColors[2];   // A1CAB8, initializer 865C80 - squadFrame.tga
// A1C698, initializer 865C20 - the row highlight. [0] is all zero: its source A1C678 is never
// written anywhere in the image (a code-operand scan finds only 865C20's reads).
extern meme::Color commanderHighlightColors[2];
} } }
#endif
#endif
