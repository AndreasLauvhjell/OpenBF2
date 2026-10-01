// Names/path [INFERRED] beside attested BFMeme/System.cpp. Both complete41-slot
// tables949918/94EFA8 inherit ForwardingIStream.
#ifndef BFMEME_NAMEDINPUTSTREAM_H
#define BFMEME_NAMEDINPUTSTREAM_H
#include "ClassIStream.h"
#if !DEDICATED
namespace dice { namespace meme {
class OStream;
class TeeIStream : public ForwardingIStream
{
public:
    TeeIStream(IStream* input, OStream* output); // inlined82A4AB..82A4B8
    ~TeeIStream() override;
    int read(void*, int bytes) override;
private:
    OStream* m_output;                          // +10, borrowed cache output
};
class RelativeIStream : public ForwardingIStream
{
public:
    explicit RelativeIStream(IStream* input);   // 83B360, borrowed cache input
    ~RelativeIStream() override;
    int skip(int bytes) override;               // 83B3A0; logical adjustment only
    int read(void*, int bytes) override;
    int getPosition() override;
    // setPosition is inherited unchanged and passes an absolute position.
private:
    int m_initialPosition;                      // +10
    int m_skippedBytes;                         // +14
};
} }
#endif
#endif
