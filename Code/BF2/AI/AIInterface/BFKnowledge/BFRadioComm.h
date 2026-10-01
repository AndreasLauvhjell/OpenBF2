// Exposes engine radio communication to the AI.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFRADIOCOMM_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFRADIOCOMM_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class IBFRadioComm
{
public:
    IBFRadioComm();
    virtual ~IBFRadioComm();

    virtual int sendAIRadioMessage(int playerId, ChannelMessageType channel, AIRadioChannelType type, unsigned char a, int b, unsigned char c, unsigned char d) = 0;
};

class BFRadioComm : public IBFRadioComm
{
public:
    BFRadioComm();
    virtual ~BFRadioComm();

    virtual int sendAIRadioMessage(int playerId, ChannelMessageType channel, AIRadioChannelType type, unsigned char a, int b, unsigned char c, unsigned char d);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFRADIOCOMM_H
