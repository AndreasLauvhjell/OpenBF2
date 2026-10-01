// Demo-file header: version integer, then length-prefixed server name, date/time,
// and map name. [INFERRED] header path; host STL layout varies.
#ifndef GAME_DEMORECORDER_DEMOHEADER_H
#define GAME_DEMORECORDER_DEMOHEADER_H
#include <string>

namespace dice { namespace hfe {
namespace io { class IStream; }

class DemoHeader
{
public:
    DemoHeader();                                 //  / PE 5B0490: 0, "", "", "", false
    bool readFrom(io::IStream* stream);
    void writeTo(io::IStream* stream) const;      //  (server side)

    int version;                                  // +00 / PE +00  3153 when recorded
    std::string serverName;                       // +04 / PE +04  ServerSettings::m_serverName
    std::string dateTime;                         // +08 / PE +20  ISystemInfo::getDateTimeString
    std::string mapName;                          // +0C / PE +3C  Game::getCurrentMapName
    bool byte10;                                  // +10 / PE +58  [UNRESOLVED] zeroed, never serialized
};

} }
#endif
