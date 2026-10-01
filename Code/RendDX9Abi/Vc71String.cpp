// Path:     [CHOSEN, not recovered] - see Vc71String.h.

#include "Vc71String.h"
#include "Msvcr71Heap.h"

#include <string.h>
#include <stddef.h>
#include <type_traits>

namespace vc71 {

// The four facts this file exists to preserve, checked at compile time so a careless edit fails the
// build instead of the game.
static_assert(sizeof(string) == 28, "vc71::string must be 28 bytes - loadPluginDll proves it");
// standard layout is the property that makes the offsets real. It is what guarantees the compiler
// places the four members in declaration order with no reordering and no gaps beyond alignment,
// which is the whole basis for claiming +4/+20/+24 match VC7.1. offsetof() cannot be used to say
// it more directly - the members are private, by design - but this assertion covers the same
// ground, and it is the assertion that would actually fire if someone added a base class or gave
// one member a different access level.
static_assert(std::is_standard_layout<string>::value,
              "vc71::string must be standard-layout or its member offsets are not guaranteed");

namespace {

// VC7.1 _Copy(): _Newres = _Newsize | _ALLOC_MASK, where _ALLOC_MASK is _BUF_SIZE - 1 == 15.
inline unsigned int roundCapacity(unsigned int len)
{
    return len | (string::BUF_SIZE - 1);
}

} // anonymous namespace

string::string()
{
    m_alloc[0] = 0; m_alloc[1] = 0; m_alloc[2] = 0; m_alloc[3] = 0;
    m_bx.buf[0] = '\0';
    m_size = 0;
    // 15, not 0. `c_str()` on the host side tests `_Myres < 16`; a zero capacity happens to pass
    // that test, but VC7.1's _Tidy() writes _BUF_SIZE - 1 and a heap dump should match.
    m_res  = BUF_SIZE - 1;
}

string::string(const char* s)
{
    m_alloc[0] = 0; m_alloc[1] = 0; m_alloc[2] = 0; m_alloc[3] = 0;
    m_bx.buf[0] = '\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;
    if (s)
        assign(s, static_cast<unsigned int>(::strlen(s)));
}

string::string(const char* s, unsigned int len)
    : string()
{
    assign(s, len);
}

string::string(const string& rhs)
{
    m_alloc[0] = 0; m_alloc[1] = 0; m_alloc[2] = 0; m_alloc[3] = 0;
    m_bx.buf[0] = '\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;
    assign(rhs.c_str(), rhs.m_size);
}

string::~string()
{
    tidy();
}

string& string::operator=(const string& rhs)
{
    if (this != &rhs)
        assign(rhs.c_str(), rhs.m_size);
    return *this;
}

string& string::operator=(const char* s)
{
    assign(s, s ? static_cast<unsigned int>(::strlen(s)) : 0u);
    return *this;
}

string& string::operator+=(const char* s)
{
    if (!s || !*s)
        return *this;

    const unsigned int addLen = static_cast<unsigned int>(::strlen(s));
    const unsigned int newLen = m_size + addLen;

    // Build the whole result before touching our own storage - `s` may point into this very buffer.
    char* buffer = static_cast<char*>(renddx9abi::crtMalloc(newLen + 1));
    if (!buffer)
        return *this;

    ::memcpy(buffer, c_str(), m_size);
    ::memcpy(buffer + m_size, s, addLen);
    buffer[newLen] = '\0';

    assign(buffer, newLen);
    renddx9abi::crtFree(buffer);
    return *this;
}

string& string::operator+=(const string& rhs)
{
    return operator+=(rhs.c_str());
}

void string::tidy()
{
    // The host's ~basic_string does exactly this test, so ours must agree on the boundary or a
    // buffer would be freed twice or leaked.
    if (m_res >= BUF_SIZE && m_bx.ptr)
        renddx9abi::crtFree(m_bx.ptr);
    m_bx.buf[0] = '\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;
}

void string::assign(const char* s, unsigned int len)
{
    // Copy through a temporary first: `s` may point into our own buffer (self-assignment through
    // c_str()), and tidy() would free it out from under the memcpy.
    char*        heap = 0;
    unsigned int cap  = BUF_SIZE - 1;

    if (len >= BUF_SIZE)
    {
        cap  = roundCapacity(len);
        heap = static_cast<char*>(renddx9abi::crtMalloc(cap + 1));
        if (!heap)
        {
            // Nothing sensible remains. Leaving the string empty is the one outcome that cannot
            // corrupt the host: an empty registration name fails a lookup, it does not free a
            // foreign pointer.
            tidy();
            return;
        }
        ::memcpy(heap, s, len);
        heap[len] = '\0';
    }
    else
    {
        char local[BUF_SIZE];
        if (len)
            ::memcpy(local, s, len);
        local[len] = '\0';
        tidy();
        ::memcpy(m_bx.buf, local, len + 1);
        m_size = len;
        m_res  = BUF_SIZE - 1;
        return;
    }

    tidy();
    m_bx.ptr = heap;
    m_size   = len;
    m_res    = cap;
}

// The wide twin

static_assert(sizeof(wstring) == 28, "vc71::wstring must be 28 bytes - sub_1004C650 reads it so");
static_assert(std::is_standard_layout<wstring>::value,
              "vc71::wstring must be standard-layout or its member offsets are not guaranteed");

namespace {

inline unsigned int wlen(const wchar_t* s)
{
    unsigned int n = 0;
    if (s) while (s[n]) ++n;
    return n;
}

} // anonymous namespace

wstring::wstring()
{
    m_alloc[0] = 0; m_alloc[1] = 0; m_alloc[2] = 0; m_alloc[3] = 0;
    m_bx.buf[0] = L'\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;      // 7: VC7.1's _Tidy() for wchar_t (_BUF_SIZE 8)
}

wstring::wstring(const wchar_t* s)
{
    m_alloc[0] = 0; m_alloc[1] = 0; m_alloc[2] = 0; m_alloc[3] = 0;
    m_bx.buf[0] = L'\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;
    if (s)
        assign(s, wlen(s));
}

wstring::~wstring()
{
    tidy();
}

wstring& wstring::operator=(const wchar_t* s)
{
    assign(s ? s : L"", wlen(s));
    return *this;
}

void wstring::tidy()
{
    if (m_res >= BUF_SIZE && m_bx.ptr)
        renddx9abi::crtFree(m_bx.ptr);
    m_bx.buf[0] = L'\0';
    m_size = 0;
    m_res  = BUF_SIZE - 1;
}

void wstring::assign(const wchar_t* s, unsigned int len)
{
    wchar_t*     heap = 0;
    unsigned int cap  = BUF_SIZE - 1;

    if (len >= BUF_SIZE)
    {
        cap  = len | (BUF_SIZE - 1);    // VC7.1 _Copy(): _Newsize | _ALLOC_MASK, 7 for wchar_t
        heap = static_cast<wchar_t*>(renddx9abi::crtMalloc((cap + 1) * sizeof(wchar_t)));
        if (!heap)
        {
            tidy();
            return;
        }
        ::memcpy(heap, s, len * sizeof(wchar_t));
        heap[len] = L'\0';
    }
    else
    {
        wchar_t local[BUF_SIZE];
        if (len)
            ::memcpy(local, s, len * sizeof(wchar_t));
        local[len] = L'\0';
        tidy();
        ::memcpy(m_bx.buf, local, (len + 1) * sizeof(wchar_t));
        m_size = len;
        m_res  = BUF_SIZE - 1;
        return;
    }

    tidy();
    m_bx.ptr = heap;
    m_size   = len;
    m_res    = cap;
}

} // namespace vc71
