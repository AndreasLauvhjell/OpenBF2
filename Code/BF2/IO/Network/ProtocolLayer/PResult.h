// Connection results and disconnect reasons. Enumerator names remain [UNRESOLVED].

#ifndef IO_NETWORK_PROTOCOLLAYER_PRESULT_H           // [INFERRED] guard style
#define IO_NETWORK_PROTOCOLLAYER_PRESULT_H

namespace dice {
namespace hfe {
namespace io {

// Results include data (0), completed handshake (1), drained queue (8), empty payload (9), timeout (10) and refusal (13).
// Reasons 28 and 29 identify invalid delivery notifications and payloads.
enum PResult
{
    PResult_0  = 0,            // [UNRESOLVED name] - a data packet. was misnamed
    PResult_1  = 1,            // [UNRESOLVED name] - a NEW connection completed its handshake
    PResult_8  = 8,            // [UNRESOLVED name] - nothing left to receive this pass
    PResult_9  = 9,            // [UNRESOLVED name] - a zero-length payload
    PResult_6  = 6,            // [UNRESOLVED name] - a ping was answered (handlePingRequest stores it)
    PResult_10 = 10,           // [UNRESOLVED name] - the connection timeout reason, used by both
                               //   _closeConnection and storeConnectionEvent
    PResult_11 = 11,           // [UNRESOLVED name] - a return code (out-of-window); never stored, but
                               //   handleDataPacket / pingConfirmation return it. Kept for completeness.
    PResult_13 = 13,           // [UNRESOLVED name] - refused / dropped
    PResult_14 = 14            // [UNRESOLVED name] - recv ring full: the packet was dropped behind a
                               //   full 256-slot receive queue (handleDataPacket's backpressure arm)
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_PROTOCOLLAYER_PRESULT_H
