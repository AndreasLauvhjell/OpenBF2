// VC7.1 string ABI adapter, [CHOSEN, not recovered] source.
// The 28-byte layout has allocator padding at +0, a 16-byte inline buffer/pointer at +4,
// length at +20 and capacity at +24. Capacity below 16 selects the inline buffer.
// Heap strings allocate capacity (length | 15) plus the terminator through MSVCR71, because
// MSVCP71 may destroy them in the host. Never free those buffers through Memory.dll.
// Bytes after the terminator are unspecified, including the unused inline-buffer tail.

#ifndef RENDDX9ABI_VC71STRING_H
#define RENDDX9ABI_VC71STRING_H

namespace vc71 {

class string
{
public:
    // The published constant, so callers can assert against it rather than repeat 16.
    enum { BUF_SIZE = 16 };

    string();
    string(const char* s);
    // [CHOSEN, not recovered] Counted native-host conversion; keeps embedded NUL bytes.
    // The ABI carrier layout is unchanged, and storage uses the same assign implementation.
    string(const char* s, unsigned int len);
    string(const string& rhs);
    ~string();

    string& operator=(const string& rhs);
    string& operator=(const char* s);

    // Appends text while retaining the VC7.1 string storage and allocator contract.
    string& operator+=(const char* s);
    string& operator+=(const string& rhs);

    const char*  c_str() const { return m_res < BUF_SIZE ? m_bx.buf : m_bx.ptr; }
    unsigned int size()  const { return m_size; }
    bool         empty() const { return m_size == 0; }

private:
    void assign(const char* s, unsigned int len);
    void tidy();

    // the field order and the padding member are the point of this class. Do not reorder, do not
    // remove m_alloc, do not add a member. A static_assert in Vc71String.cpp holds the size at 28
    // and the offsets at 4/20/24.
    char         m_alloc[4];                            // +0   std::allocator<char>, padded
    union Bx { char buf[BUF_SIZE]; char* ptr; } m_bx;   // +4
    unsigned int m_size;                                // +20  _Mysize
    unsigned int m_res;                                 // +24  _Myres
};

// The wide twin: VC7.1 std::wstring. The same 28 bytes and the same field order; the in-buffer
// capacity is _BUF_SIZE = 16 / sizeof(wchar_t) = 8, so the in-buffer test is `_Myres < 8` - which is
// exactly how the shipped SwiffPlayer's Locale.getString (sub_1004C650) reads the host's answer
// (`v13[5] < 8u ? &v13 : v13[0]`). The host localisation call PRE-constructs one of these, the host
// assigns into it with MSVCP71's operator=, and the caller destroys it: the long case therefore has
// to free through the MSVCR71 heap, as the narrow class does. Only what the bridge needs is here.
class wstring
{
public:
    enum { BUF_SIZE = 8 };

    wstring();
    wstring(const wchar_t* s);
    ~wstring();

    wstring& operator=(const wchar_t* s);

    const wchar_t* c_str() const { return m_res < BUF_SIZE ? m_bx.buf : m_bx.ptr; }
    unsigned int   size()  const { return m_size; }
    bool           empty() const { return m_size == 0; }

private:
    wstring(const wstring&) = delete;
    wstring& operator=(const wstring&) = delete;
    void assign(const wchar_t* s, unsigned int len);
    void tidy();

    // Same discipline as vc71::string: the order and the padding member are the point.
    char         m_alloc[4];                                    // +0   std::allocator<wchar_t>, padded
    union Bx { wchar_t buf[BUF_SIZE]; wchar_t* ptr; } m_bx;     // +4
    unsigned int m_size;                                        // +20  _Mysize
    unsigned int m_res;                                         // +24  _Myres
};

} // namespace vc71

#endif // RENDDX9ABI_VC71STRING_H
