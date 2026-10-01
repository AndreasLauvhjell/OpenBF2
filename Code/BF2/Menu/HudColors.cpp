#include "HudColors.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace hud {
// Complete initializer instructions and exact PE bytes read.
// Every row is an independent mutable copy. These are not aliases of chat colors.

meme::Color teamTopColors[4] = {
    {1, 1, 1, 1},
    {1, 0.7098039388656616f, 0.7098039388656616f, 1},
    {0.6235294342041016f, 0.843137264251709f, 1, 1},
    {1, 0, 1, 1}
};
// PE8657E0: 97E1A0,A1C908,A1CB58,97E190; the middle two are themselves
// initialized by8644A0 from97DEA0 and864530 from97E030 before the copy.
meme::Color ticketColors[4] = {
    {1, 1, 1, 1},
    {0.8700000047683716f, 0.09399999678134918f, 0.09399999678134918f, 1},
    {0.2540000081062317f, 0.5519999861717224f, 0.8899999856948853f, 1},
    {1, 0, 1, 1}
};

meme::Color textColors[4] = {
    {1, 1, 1, 1},
    {1, 0.4000000059604645f, 0.4000000059604645f, 1},
    {0.4000000059604645f, 0.6000000238418579f, 1, 1},
    {1, 0, 1, 1}
};

meme::Color brightColors[4] = {
    {1, 1, 1, 1},
    {0.7882353067398071f, 0.16078431904315948f, 0.16078431904315948f, 1},
    {0.2078431397676468f, 0.27450981736183167f, 0.5490196347236633f, 1},
    {1, 0, 1, 1}
};

// Decimal literals below round to the exact four-byte PE initializer values.
// These are independent mutable copies, as in the retail initialization chain.
meme::Color defaultChatColors[4] = {
    {0.765f, 0.729f, 0.663f, 1},
    {1, 0.3921568691730499f, 0.3921568691730499f, 1},
    {0.0117647061124444f, 0.7215686440467834f, 0.9490196108818054f, 1},
    {1, 0, 1, 1}
};
meme::Color squadChatColors[4] = {
    {1, 1, 1, 1},
    {0.4470588266849518f, 0.8901960849761963f, 0.3450980484485626f, 1},
    {0.4470588266849518f, 0.8901960849761963f, 0.3450980484485626f, 1},
    {0.4470588266849518f, 0.8901960849761963f, 0.3450980484485626f, 1}
};
meme::Color commanderChatColor =
    {0.9686274528503418f, 0.8274509906768799f, 0.2549019753932953f, 1};
// Complete PE8655A0 copies97E1A0/97DFA0/97E130/97E190, in this order.
meme::Color categoryThreeChatColors[4] = {
    {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 0, 1}, {1, 0, 1, 1}
};
// PE864BF0: 97E1A0,97DE70,97E000,97DE70. Note the last source: brightColors' initializer
// (864B30) is otherwise identical but ends at 97E190, the magenta marker. Here entry [3] repeats
// entry [1]'s red, so an out-of-range team paints red rather than magenta.
// PE8649D0: 97E1A0,97DF00,97E090,97DF00. The only one of the three whose two side entries
// differ - [1] enemy red, [2] friendly blue - which is what confirms the index is a side.
meme::Color scoreboardNameColors[4] = {
    {1, 1, 1, 1},
    {0.9921568632125854f, 0, 0.003921568859368563f, 1},
    {0, 0.3803921639919281f, 1, 1},
    {0.9921568632125854f, 0, 0.003921568859368563f, 1}
};
// PE864920: 97E1A0,97DEF0,97E080,97DEF0. Entries [1] [2] [3] are the same green - a squadmate is
// painted identically whichever side the list is showing.
meme::Color scoreboardSquadColors[4] = {
    {1, 1, 1, 1},
    {0.03137255087494850f, 0.4666666686534882f, 0.007843137718737125f, 1},
    {0.03137255087494850f, 0.4666666686534882f, 0.007843137718737125f, 1},
    {0.03137255087494850f, 0.4666666686534882f, 0.007843137718737125f, 1}
};
// PE864A80: 97E1A0,97DF10,97E0A0,97DF10. Also uniform across [1] [2] [3] - a dark olive that
// reads as greyed out on either side.
meme::Color scoreboardDeadColors[4] = {
    {1, 1, 1, 1},
    {0.2901960909366608f, 0.2705882489681244f, 0.2078431397676468f, 1},
    {0.2901960909366608f, 0.2705882489681244f, 0.2078431397676468f, 1},
    {0.2901960909366608f, 0.2705882489681244f, 0.2078431397676468f, 1}
};
meme::Color scoreboardColors[4] = {
    {1, 1, 1, 1},
    {0.7882353067398071f, 0.16078431904315948f, 0.16078431904315948f, 1},
    {0.2078431397676468f, 0.27450981736183167f, 0.5490196347236633f, 1},
    {0.7882353067398071f, 0.16078431904315948f, 0.16078431904315948f, 1}
};
meme::Color miniControlPointColors[4] = {
    {1, 1, 1, 1},
    {0.5400000214576721f, 0.5400000214576721f, 0.5400000214576721f, 1},
    {0.5400000214576721f, 0.5400000214576721f, 0.5400000214576721f, 1},
    {0.5400000214576721f, 0.5400000214576721f, 0.5400000214576721f, 1}
};
meme::Color mapObjectColors[4] = {
    {1, 1, 1, 1},
    {0.8700000047683716f, 0.09399999678134918f, 0.09399999678134918f, 1},
    {0.2540000081062317f, 0.5519999861717224f, 0.8899999856948853f, 1},
    {1, 0, 1, 1}
};
meme::Color mapSelectedColors[4] = {
    {1, 1, 1, 1}, {0, 1, 0, 1}, {0, 1, 1, 1}, {1, 0, 1, 1}
};
meme::Color mapWhiteColors[4] = {
    {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}
};
meme::Color mapNeutralColors[4] = {
    {1, 1, 1, 1}, {0.7f, 0.7f, 0.7f, 1},
    {0.7f, 0.7f, 0.7f, 1}, {0.7f, 0.7f, 0.7f, 1}
};
meme::Color mapTimedColors[4] = {
    {1, 1, 1, 1}, {1, 1, 0, 1}, {1, 1, 0, 1}, {1, 1, 0, 1}
};
meme::Color mapAlertColors[4] = {
    {1, 1, 1, 0.8f}, {1, 0, 0, 0.8f},
    {1, 0, 0, 0.8f}, {1, 0, 0, 0.8f}
};
// PE865AC0: 97E1A0,97DFF0,97E180,97DFF0 - white, then the same opaque red three times (97DFF0 and
// 97E180 hold identical bytes). Only entry [2] has a reader, Bf2MenuMapNode's collectAirstrikeIcon.
meme::Color mapAirstrikeColors[4] = {
    {1, 1, 1, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}, {1, 0, 0, 1}
};
// Map order palette. Preserve literal precision when changing formatting.
meme::Color mapOrderColors[11] = {
    {1, 1, 0, 1},                  // 0 artillery
    {1, 1, 0, 1},                  // 1 UAV
    {1, 1, 0, 1},                  // 2 vehicle drop
    {1, 1, 0, 1},                  // 3 supplies
    {0.796f, 0.776f, 0.2156f, 1},  // 4 move
    {1, 0.611f, 0, 1},             // 5 attack
    {0.792f, 0.2f, 0.949f, 1},     // 6 defend
    {0.792f, 0.2f, 0.949f, 1},     // 7 repair   (copied from A1D3C0)
    {1, 0.611f, 0, 1},             // 8 destroy  (copied from A1D3D0)
    {1, 0.611f, 0, 1},             // 9 mine     (copied from A1D3B0)
    {0.757f, 0.2196f, 0.196f, 1}   // 10
};
// PE865FE0 copies eight per-TU constants, in this order: 97E260, A1D038 (a copy of 97E260, 8646E0),
// A1CA08 (97E240, 864710), A1CB08 (a copy of A1CA08, 864740), 97E250, A1C688 (a copy of 97E250,
// 8646B0), 97E270, A1C9D8 (a copy of 97E270, 864770). 97E260 and 97E270 hold the same red.
meme::Color mapSquadRequestColors[4][2] = {
    { {0.9215686321258545f, 0.2078431397676468f, 0.2078431397676468f, 1},
      {0.9215686321258545f, 0.2078431397676468f, 0.2078431397676468f, 1} },
    { {1, 0.7921568751335144f, 0.04313725605607033f, 1},
      {1, 0.7921568751335144f, 0.04313725605607033f, 1} },
    { {0, 1, 0, 1}, {0, 1, 0, 1} },
    { {0.9215686321258545f, 0.2078431397676468f, 0.2078431397676468f, 1},
      {0.9215686321258545f, 0.2078431397676468f, 0.2078431397676468f, 1} }
};
// PE864D50: 97E1A0, 97DF20, 97E0B0, 97DF20 (97DF20 and 97E0B0 hold identical bytes 3F800000
// 3F666666 3E4CCCCD 3F800000). 0.9f = 3F666666 and 0.2f = 3E4CCCCD exactly.
meme::Color squadInviteColors[4] = {
    {1, 1, 1, 1}, {1, 0.9f, 0.2f, 1}, {1, 0.9f, 0.2f, 1}, {1, 0.9f, 0.2f, 1}
};
// Commander squad-row palettes indexed by selection state: zero unselected, one selected.
meme::Color commanderIconColors[2] = {
    {0.5686274766921997f, 0.5490196347236633f, 0.43921568989753723f, 1},
    {1, 1, 1, 1}
};
// A1CDA8 (864590's copy of 97E1E0), then 97E200.
meme::Color commanderFrameColors[2] = {
    {0.364705890417099f, 0.3529411852359772f, 0.3019607961177826f, 1},
    {0.6549019813537598f, 0.6431372761726379f, 0.5254902243614197f, 1}
};

meme::Color commanderCellColors[2] = {
    {0.364705890417099f, 0.3529411852359772f, 0.3019607961177826f, 1},
    {0.4941176474094391f, 0.48627451062202454f, 0.3921568691730499f, 1}
};
// PE 865CE0: 97E240, then A1C6D8 (864620's copy of 97DD50).
meme::Color commanderSquadIconColors[2] = {
    {1, 0.7921568751335144f, 0.04313725605607033f, 1},
    {1, 1, 1, 1}
};
// A1CEE8 (8645C0's copy of 97E210), then 97E210 - both entries the same olive.
meme::Color commanderNumberColors[2] = {
    {0.24300000071525574f, 0.22699999809265137f, 0.1679999977350235f, 1},
    {0.24300000071525574f, 0.22699999809265137f, 0.1679999977350235f, 1}
};

meme::Color commanderRowFrameColors[2] = {
    {0.33725491166114807f, 0.3333333432674408f, 0.2862745225429535f, 1},
    {0.7803921699523926f, 0.772549033164978f, 0.6627451181411743f, 1}
};
// A1C678 - never written, so zero at run time - then A1CC68 (8645F0's copy of 97E1F0).
meme::Color commanderHighlightColors[2] = {
    {0, 0, 0, 0},
    {0.4941176474094391f, 0.48627451062202454f, 0.3921568691730499f, 1}
};
} } }
#endif
