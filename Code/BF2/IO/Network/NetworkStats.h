// Per-class bit counters registered by name for network statistics displays.

#ifndef IO_NETWORK_NETWORKSTATS_H     // [INFERRED] guard style
#define IO_NETWORK_NETWORKSTATS_H

#include <map>
#include <string>
#include <vector>

#include "../../Math/Vec3.h"
#include "ProtocolLayer/NetServer.h"             // ConnectionStatus, the by-value telemetry tail

namespace dice {
namespace hfe {
namespace io {

} // namespace io
class IClientConnection;                        // Game/Common/IClientConnection.h
namespace io {

// Aggregate network telemetry. Preserve destructor/update slot order and use native container layouts.
class BasicNetworkStats
{
public:
    BasicNetworkStats();
    virtual ~BasicNetworkStats();
    virtual void update();

    void sampleStreamManagerReadStats(::dice::hfe::IClientConnection* connection, bool accumulate);
    void sampleStreamManagerWriteStats(::dice::hfe::IClientConnection* connection, bool accumulate);
    void sampleNetworkableReadStats();
    void sampleNetworkableWriteStats();
    void updateClientStats();
    void updateServerStats();

    // [INFERRED] accessor names for the retail status-screen member reads; no virtual slots.
    const float* getStreamManagerReadPercent() const { return m_streamManagerReadPercent; }
    const float* getStreamManagerWritePercent() const { return m_streamManagerWritePercent; }
    const ConnectionStatus& getConnectionStatus() const { return m_connectionStatus; }

protected:
    // [INFERRED] member names; vector<float> types and order are attested by ctor/dtor/samplers.
    // Offset pairs are retail ELF / PE, not assertions about the current compiler's STL.
    std::vector<float> m_networkableReadBits;         // +04 / +04
    std::vector<float> m_networkableWriteBits;        // +10 / +14
    std::vector<float> m_streamManagerReadBits;       // +1C / +24
    std::vector<float> m_streamManagerWriteBits;      // +28 / +34
    float m_streamManagerReadPercent[3];             // +34 / +44
    float m_streamManagerWritePercent[3];            // +40 / +50
    ConnectionStatus m_connectionStatus;            // +4C / +5C; deliberately not initialized by ctor
};

class RegistredNetworkStats
{
public:
    RegistredNetworkStats(const std::string& name, const Vec3& colour);   // 270 B - registers itself

    static RegistredNetworkStats* getNetWorkStats(const std::string& name);       // 64 B  [sic]
    static std::map<std::string, RegistredNetworkStats*>* getNetworkStatsMap();

    /* +0x00 */ std::string  m_name;
    /* +0x04 */ int          m_writeBits;      // ctor 0 - getNetUpdate's "bits written this call" sum
    /* +0x08 */ int          m_readBits;       // ctor 0 - setNetUpdate's "bits read this call" sum
    /* +0x0C */ Vec3         m_colour;         // the graph colour
};

// The map behind the two statics - a function-local static (0x8AC3CD8 guard, 0x8AC3CE0 object).
std::map<std::string, RegistredNetworkStats*>* internal_getNetworkStatsMap();

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_NETWORKSTATS_H
