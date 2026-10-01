// [INFERRED] path; ELF 814F120/814F1C0/814F2B0 and PE 5B0490/5B06C0 read in full.
#include "DemoHeader.h"
#include "../../IO/Stream/IStream.h"

namespace dice { namespace hfe {

DemoHeader::DemoHeader() : version(0), byte10(false) {}

//  / PE 5B06C0: four raw bytes into the version (the count is not checked), then the
// three strings through io::read, each gating the next; byte10 is never touched.
bool DemoHeader::readFrom(io::IStream* stream)
{
    stream->read(&version, 4);
    return io::read(stream, serverName) && io::read(stream, dateTime) && io::read(stream, mapName);
}

// : the version, then each string as its 32-bit length followed by its bytes. The last
// write's count is what the ELF leaves in eax; nothing reads it.
void DemoHeader::writeTo(io::IStream* stream) const
{
    int value = version;
    stream->write(&value, 4);
    const std::string* strings[3] = { &serverName, &dateTime, &mapName };
    for (int i = 0; i < 3; ++i) {
        int length = static_cast<int>(strings[i]->size());
        stream->write(&length, 4);
        stream->write(strings[i]->data(), static_cast<unsigned int>(length));
    }
}

} }
