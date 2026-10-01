// [INFERRED] path. Full client PE66E7D0/5B0190/5B01C0/5B0400/5B0320/5B0260 read.
#include "PasswordCodec.h"
#if !DEDICATED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wincrypt.h>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <sstream>
#pragma comment(lib, "crypt32.lib")

namespace dice { namespace hfe {

PasswordCodec::PasswordCodec() // PE66E7D0
    : m_blob(0)
{
}

PasswordCodec::~PasswordCodec() // PE5B0190
{
    if (m_blob) {
        LocalFree(m_blob->pbData);
        delete m_blob;
        m_blob = 0;
    }
}

bool PasswordCodec::encode(const std::string& plain) // PE5B01C0
{
    // The input blob is filled (terminator included) before the empty test, as shipped.
    DATA_BLOB input;
    input.pbData = (BYTE*)plain.c_str();
    input.cbData = (DWORD)std::strlen(plain.c_str()) + 1;
    if (plain.size() == 0)
        return false;
    if (m_blob) {
        LocalFree(m_blob->pbData);
        delete m_blob;
        m_blob = 0;
    }
    m_blob = new DATA_BLOB;
    return CryptProtectData(&input, L"This is the description string.", 0, 0, 0,
                            CRYPTPROTECT_UI_FORBIDDEN, m_blob) != 0;
}

void PasswordCodec::write(std::ostream& stream) // PE5B0400
{
    for (unsigned int i = 0; i < m_blob->cbData; ++i)
        stream << std::hex << std::setw(2) << std::setfill('0')
               << (unsigned int)m_blob->pbData[i];
}

void PasswordCodec::readHexPairs(const std::string& text, unsigned char* out) // PE5B0260
{
    const unsigned int count = text.size() >> 1;
    std::stringstream unused(std::ios::in | std::ios::out); // constructed and never used (retail)
    std::istringstream reader(text);
    for (unsigned int i = 0; i < count; ++i) {
        char high, low;
        reader >> high >> low;
        const int h = high <= '9' ? high - '0' : high - 'a' + 10;
        const int l = low <= '9' ? low - '0' : low - 'a' + 10;
        out[i] = (unsigned char)(16 * h + l);
    }
}

bool PasswordCodec::decode(std::string& text) // PE5B0320
{
    const unsigned int count = text.size() >> 1;
    LPWSTR description = 0;
    if (!count)
        return false;
    unsigned char* bytes = new unsigned char[count];
    readHexPairs(text, bytes);
    DATA_BLOB input;
    input.pbData = bytes;
    input.cbData = count;
    DATA_BLOB output;
    if (CryptUnprotectData(&input, &description, 0, 0, 0, CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        text = std::string((const char*)output.pbData);
        LocalFree(description);
        delete[] bytes;
        LocalFree(output.pbData);
        return true;
    }
    // The system message is formatted and released without being shown; `bytes` is not freed.
    const DWORD error = GetLastError();
    LPSTR message = 0;
    if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message, 0, 0))
        LocalFree(message);
    return false;
}

} }
#endif
