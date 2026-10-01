// Names/path [INFERRED]. System+48 owns the12-byte queue constructed at83B7D0.
// Event records are six dwords; all instructions read in the client PE.
#ifndef BFMEME_EVENTQUEUE_H
#define BFMEME_EVENTQUEUE_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include <cstdint>
namespace dice { namespace meme {
class System;
struct QueuedEvent
{
    QueuedEvent* next;
    int type, index;
    float first, second;
    std::uint32_t field14; // [UNRESOLVED] raw payload word; all observed callers pass0.
    QueuedEvent(int type, int index, float first, float second, std::uint32_t field14);
};
class EventQueue
{
public:
    explicit EventQueue(unsigned int additionalNodes); // 83B7D0, always one plus count
    ~EventQueue();
    void enqueue(System*, const QueuedEvent&);         // 83B890; System argument unused
    bool process(System*);
private:
    void processEvent1(System*, int first, int index, int second);
    void processEvent2(System*, int first, int index);
    void processEvent4(System*, int first, int index, int second);
    QueuedEvent* m_free;
    QueuedEvent* m_first;
    QueuedEvent* m_last;
};
} }
#endif
#endif
