// [INFERRED] name/path. Client constructor7D3540/table939908 implements a
// BFMeme IStream backed by the real io::MemStream loaded through io::File.
#ifndef BFMEME_FILEISTREAM_H
#define BFMEME_FILEISTREAM_H
#include "IStream.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace io { class IMemStream; } } }
namespace dice { namespace meme {
class FileIStream : public IStream
{
public:
    explicit FileIStream(const char* path);
    ~FileIStream() override;
    int getAvailableBytes() override;
    int skip(int bytes) override;                 // 7D3640, returns low8 bits of io skip
    int read(void* buffer, int bytes) override;
    int getPosition() override;
    bool setPosition(int position) override;
    // The inherited retail IStream descriptor/size remain8 despite concrete12.
private:
    hfe::io::IMemStream* m_memory;                // +8, released by destructor
};
} }
#endif
#endif
