// [INFERRED] Source placement.

// File callbacks used by the sound DLL. No virtual destructor or IBase methods.
// seek() returns the resulting position, or zero on failure; successful seeks to zero are indistinguishable.
// read() returns bytes read. [CHOSEN] Handle types model the opaque value passed across the DLL boundary.


#ifndef SOUND_ISOUNDFILEIO_H     // [INFERRED] guard style
#define SOUND_ISOUNDFILEIO_H

namespace dice {
namespace hfe {

class ISoundFileIO
{
public:
    virtual void* open(const char* name) = 0;                            // +0x00
    virtual int   close(void* handle) = 0;                               // +0x04
    virtual int   seek(void* handle, int offset, int origin) = 0;        // +0x08 position, 0 = fail
    virtual int   read(void* handle, void* buffer, int bytes) = 0;       // +0x0C bytes read

protected:
    ~ISoundFileIO() {}
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ISOUNDFILEIO_H
