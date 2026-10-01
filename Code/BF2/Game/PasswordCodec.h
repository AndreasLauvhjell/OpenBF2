// Client-only dpapi password codec owning one DATA_BLOB pointer, without a vtable.
// Used for saved profile passwords. [INFERRED] path and class name.
#pragma once
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <iosfwd>
#include <string>

struct _CRYPTOAPI_BLOB; // DATA_BLOB (wincrypt.h); the header stays free of windows.h

namespace dice { namespace hfe {

class PasswordCodec
{
public:
    PasswordCodec();                                            // PE66E7D0: the pointer is zeroed
    ~PasswordCodec();                                           // PE5B0190: LocalFree + delete

    // PE5B01C0. False for an empty string; otherwise the previous blob is released and the plain
    // text (with its terminator) is protected into a fresh DATA_BLOB. Returns the API result.
    bool encode(const std::string& plain);
    // Writes protected bytes as lowercase hex. [INFERRED] Generalized ostream parameter spelling.
    void write(std::ostream& stream);
    // Decodes in place, leaving text unchanged on failure. Preserves the original decoded-buffer leak on failure.
    static bool decode(std::string& text);

private:
    // PE5B0260, static, fastcall. Reads size()/2 hex pairs through an istringstream; digits above
    // '9' are taken as lowercase letters (no validation, retail).
    static void readHexPairs(const std::string& text, unsigned char* out);

    _CRYPTOAPI_BLOB* m_blob;                                    // +00; the one member (4 bytes)
};

} }
#endif
