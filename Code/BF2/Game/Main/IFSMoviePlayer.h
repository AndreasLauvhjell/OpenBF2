// Renderer plugin's full-screen movie interface. [INFERRED] path/interface name.
// Preserve slot order; release uses IBase and the deleting-destructor slot is withheld.
#ifndef GAME_MAIN_IFSMOVIEPLAYER_H
#define GAME_MAIN_IFSMOVIEPLAYER_H
#include "../../Core/IBase.h"
#include <string>
#if !DEDICATED
namespace dice { namespace hfe {

class IFSMoviePlayer : public IBase
{
public:
    // Opens a movie from a by-value path and builds playback geometry.
    // Failed loads are released before trying the next queued movie.
    virtual bool load(std::string path) = 0;
    // +14 (0x100A2200): drops the video, the quad and the effect handles.
    virtual void releaseAll() = 0;
    // +18 (0x100A2300): the per-frame movie draw; mainLoop calls it once per frame while a movie
    // plays (PE 40D3xx, the arm this tree spells moviePlayer_update until now).
    virtual void drawFrame() = 0;
    // +1C (0x100A1FD0): the video's +30 query - non-zero once the movie has ended; mainLoop
    // releases the player and asks checkMovie for the next one.
    virtual bool finished() = 0;
    // +20: the deleting destructor (0x100A2460), withheld.
};

} }
#endif
#endif
