// [INFERRED] name/path. Client7D3790/table9399B0 is an eight-byte file OStream.
#ifndef BFMEME_FILEOSTREAM_H
#define BFMEME_FILEOSTREAM_H
#include "OStream.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace io { class INonlinearStream; } } }
namespace dice { namespace meme {
class FileOStream : public OStream
{
public:
    explicit FileOStream(const char* path);
    ~FileOStream() override;
    int write(const void*, int bytes) override;
    int getPosition() override;
    bool setPosition(int) override;
    // Inherits the actual base four-byte size result and empty flush method.
private:
    hfe::io::INonlinearStream* m_file;              // +4, owns one released reference
};
} }
#endif
#endif
