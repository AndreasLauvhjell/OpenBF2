// Serialized end-of-round data with a dirty cache and delivery delay. STL members
// make object size and offsets toolchain-dependent.

#ifndef GAME_ENDOFROUNDINFO_H
#define GAME_ENDOFROUNDINFO_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {

class EndOfRoundInfo
{
public:
    EndOfRoundInfo();

    void clear();
    const std::string& getData() const;
    void setData(const std::string& data);
    int getDelay();
    void setDelay(int delay);

    const void* getNetBuffer();
    unsigned int getNetBufferSize();
    void setFromDataBlock(const void* data, unsigned int size);

private:
    void updateNetBuffer();

    std::string m_data;
    int m_delay;                         // deliberately uninitialised by the shipped constructor
    bool m_dirty;
    std::vector<unsigned char> m_netBuffer;
};

} // namespace hfe
} // namespace dice

#endif // GAME_ENDOFROUNDINFO_H
