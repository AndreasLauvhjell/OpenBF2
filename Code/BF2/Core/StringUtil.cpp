// Shared string comparisons, hashing, formatting and path helpers.
// Source path is attested by anonymous-namespace symbols.

#include "StringUtil.h"

#include <cstdarg>      // formatString is variadic
#include <cstdlib>      // mbstowcs
#include <cstdio>       // vsnprintf / sprintf
#include <cwchar>
#include <vector>

#ifdef _MSC_VER
#  include <string.h>
#  define strcasecmp  _stricmp
#  define strncasecmp _strnicmp
#else
#  include <strings.h>
#endif

namespace dice {
namespace hfe {

const std::string& getEmptyString()
{
    static const std::string empty;
    return empty;
}

// Converts wide text using the active C locale. The existing error check tests count + 1,
// so callers must not rely on it to reject every invalid conversion.
std::string makeNarrow(const std::wstring& source)
{
    const std::size_t capacity = std::wcstombs(0, source.c_str(), 0) + 1;
    if (capacity == static_cast<std::size_t>(-1))
        return "no_conversion_exists";
    std::vector<char> buffer(capacity, '\0');
    const std::size_t converted = std::wcstombs(buffer.data(), source.c_str(), capacity);
    if (converted == static_cast<std::size_t>(-1))
        return "no_conversion_exists";
    if (converted == 0)
        return "";
    return std::string(buffer.data(), converted);
}

// Converts multibyte text to wide characters using the active C locale.
std::wstring makeWide(const std::string& source)
{
    const std::size_t capacity = std::mbstowcs(0, source.c_str(), 0) + 1;
    wchar_t* buffer = new wchar_t[capacity];
    const std::size_t converted = std::mbstowcs(buffer, source.c_str(), capacity);

    std::wstring result;
    if (converted != 0)
        result.assign(buffer, converted);

    delete[] buffer;
    return result;
}

// Formats into a shared 4-KB buffer and returns a string copy; not reentrant or thread-safe.
// A negative vsnprintf result replaces the message with a warning. Successful formatting writes
// the terminator at the reported length. Preserve the original runtime-dependent overflow behavior.
std::string formatString(const char* fmt, ...)
{
    // ds:0x8ABBD20. 4096 bytes - vsnprintf is given 4095 and byte [n] is written explicitly.
    static char s_buffer[4096];             // [INFERRED] name; the ELF slot is unnamed

    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(s_buffer, 0xFFF, fmt, args);   // 0x83D03DA maxlen, 0x83D03EA call
    va_end(args);                                          // no-op on this ABI; not in the binary

    if (n == -1)
        sprintf(s_buffer, "%s",
                "WARNING: The original message was too long for formatString(). \n");
    else
        s_buffer[n] = 0;

    return std::string(s_buffer);
}

#if !DEDICATED
// Client PE 431130, all 29 instructions read. Buffer 99C158 is shared, and
// _vsnwprintf's -1 result replaces the whole message with the shipped warning.
std::wstring formatString(const wchar_t* fmt, ...)
{
    static wchar_t buffer[4096];
    va_list args;
    va_start(args, fmt);
    const int count = _vsnwprintf(buffer, 0xFFF, fmt, args);
    va_end(args);
    if (count == -1)
        _swprintf(buffer, L"%s",
            L"WARNING: The original message was too long for formatString(). \n");
    else
        buffer[count] = 0;
    return std::wstring(buffer);
}
#endif

// Compares null-terminated strings without case. String overloads delegate through c_str().
int compareIgnoreCase(const char* a, const char* b)
{
    return strcasecmp(a, b);
}

int compareIgnoreCase(const char* a, const char* b, int n)
{
    return strncasecmp(a, b, n);
}

int compareIgnoreCase(const std::string& a, const std::string& b)
{
    return compareIgnoreCase(a.c_str(), b.c_str());
}

int compareIgnoreCase(const std::string& a, const std::string& b, int n)
{
    return compareIgnoreCase(a.c_str(), b.c_str(), n);
}

bool equalsIgnoreCase(const std::string& a, const std::string& b)
{
    return compareIgnoreCase(a, b) == 0;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b, int n)
{
    return compareIgnoreCase(a, b, n) == 0;
}

// Returns the engine's case-insensitive hash using ASCII-only uppercase folding.
// Preserve its constants and shifts because bucket order affects duplicate selection.
unsigned int calcStringHashValueNoCase(const std::string& s)
{
    unsigned int a    = 39842;
    unsigned int b    = 230498;
    unsigned int hash = 0;

    for (unsigned int i = 0; i < s.size(); )
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (static_cast<unsigned char>(c - 'a') <= 25)
            c -= 32;                                   // ASCII-only uppercase

        const int shiftA = (b & 7) - (a & 7);
        const unsigned int bPrev = b;
        b -= 1994177228u;

        const unsigned int term = static_cast<unsigned int>(c) << (shiftA + 16);
        const int shiftB = (a & 3) - (bPrev & 3);
        a += 1465131657u;

        hash += (i++) | (((hash << (shiftB + 15)) + term) >> 14);
    }

    return (hash >> 16) + hash;                        // hiword(hash) + hash
}

// Reports whether the virtual-filesystem path is absolute under the selected platform rules.
// [PLATFORM] POSIX requires a leading slash; Windows searches for :\ or :/ anywhere in the string.
// Leading slash alone is relative on Windows, allowing engine search-path resolution.
bool isPathAbsolute(const std::string& s)
{
#ifdef _WIN32
    return s.find(":\\") != std::string::npos || s.find(":/") != std::string::npos;
#else
    return !s.empty() && s[0] == '/';
#endif
}

// Normalizes both slash forms to c. fixPath additionally ensures a trailing separator;
// both preserve an empty string as empty.
void fixPathDelimiters(std::string& s, char c)
{
    for (std::string::size_type i = 0; i < s.size(); ++i)
        if (s[i] == '/' || s[i] == '\\')
            s[i] = c;
}

void fixPath(std::string& s, char c)
{
    if (s.empty())
        return;                                        // guarded - "" stays ""

    fixPathDelimiters(s, c);

    if (s[s.size() - 1] != c)
        s.append(1, c);                                // guarantee the trailing separator
}

std::string getFixPathDelimiters(const std::string& s, char c)
{
    std::string out(s);
    fixPathDelimiters(out, c);
    return out;
}

std::string getFixPath(const std::string& s, char c)
{
    std::string out(s);
    fixPath(out, c);
    return out;
}

// Wide-character helper counterparts remain [UNRESOLVED]; the narrow implementations follow below.

// Changes case using explicit Latin-1 pairs for å/Å, ä/Ä, ö/Ö, ü/Ü and é/É before CRT fallback.
// Keep these mappings independent of the active locale; archive and content lookup depend on them.

namespace {

// The shared table, written once rather than twice. The binary has it open-coded in both
// directions; the two functions are byte-for-byte mirrors of each other.
struct CasePair { unsigned char lower; unsigned char upper; };

const CasePair kLatin1CasePairs[] =
{
    { 0xE5, 0xC5 },   // å Å
    { 0xE4, 0xC4 },   // ä Ä
    { 0xF6, 0xD6 },   // ö Ö
    { 0xFC, 0xDC },   // ü Ü
    { 0xE9, 0xC9 }    // é É
};

const unsigned int kLatin1CasePairCount = sizeof(kLatin1CasePairs) / sizeof(kLatin1CasePairs[0]);

} // anonymous namespace

// Uppercases text in place, handling the explicit extended-character pairs before toupper.
void setUpperCase(std::string& s)
{
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);

        unsigned int p = 0;
        for (; p < kLatin1CasePairCount; ++p)
        {
            if (c == kLatin1CasePairs[p].lower)
            {
                s[i] = static_cast<char>(kLatin1CasePairs[p].upper);
                break;
            }
        }

        if (p == kLatin1CasePairCount)
            s[i] = static_cast<char>(toupper(c));
    }
}

// The exact mirror - same five pairs, tested upper-first.
void setLowerCase(std::string& s)
{
    for (std::string::size_type i = 0; i < s.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);

        unsigned int p = 0;
        for (; p < kLatin1CasePairCount; ++p)
        {
            if (c == kLatin1CasePairs[p].upper)
            {
                s[i] = static_cast<char>(kLatin1CasePairs[p].lower);
                break;
            }
        }

        if (p == kLatin1CasePairCount)
            s[i] = static_cast<char>(tolower(c));
    }
}

// each: copy-construct, convert in place, return.

// these genuinely return by value, unlike IArchiveHandler::getExtensions - and the same ABI test
// settles it in the opposite direction. Here the body constructs into the hidden sret pointer
// (`std::string::string(this, a2)` where `this` is the return slot) and hands that back. A
// reference-returning function has nothing to construct.
std::string getUpperCase(const std::string& s)
{
    std::string result(s);
    setUpperCase(result);
    return result;
}

std::string getLowerCase(const std::string& s)
{
    std::string result(s);
    setLowerCase(result);
    return result;
}

// Returns the directory through the last slash of either kind, using delimiter for the trailing separator.
// Returns an empty string when no separator exists.
std::string extractFilePath(const std::string& path, char delimiter)
{
    std::string::size_type last = path.rfind('\\');
    const std::string::size_type slash = path.rfind('/');
    if (slash != std::string::npos && (last == std::string::npos || slash > last))
        last = slash;
    if (last == std::string::npos)
        return std::string();

    std::string out = path.substr(0, last);
    out.append(1, delimiter);
    return out;
}

// Returns text after the last dot, or empty for no dot/trailing dot.
// A preceding dot suppresses extensions only when the last dot is at index 2 or later:
// ..x yields x, while a..x yields empty.
std::string extractFileExt(const std::string& path)
{
    const std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return std::string();

    if (dot > 1 && path[dot - 1] == '.')
        return std::string();

    return path.substr(dot + 1);
}

// Normalizes repeated separators, strips a leading slash and recursively collapses /../ segments.
// Traversal beyond the beginning is absorbed rather than rejected; this is not a containment check.
// [CHOSEN, not recovered] removeDoubleSlashes still requires direct binary verification, including
// backslash and leading-double-slash behavior.
void removeDoubleSlashes(std::string& path)
{
    std::string::size_type at = path.find("//");
    while (at != std::string::npos)
    {
        path.erase(at, 1);
        at = path.find("//", at);
    }
}

void goUpInPath(std::string& path)
{
    removeDoubleSlashes(path);

    if (!path.empty() && path[0] == '/')
        path.erase(0, 1);                       // absolute becomes relative - see above

    const std::string::size_type found = path.find("/../");
    if (found == std::string::npos)
        return;

    std::string::size_type start = (found == 0) ? std::string::npos
                                                : path.rfind('/', found - 1);
    if (start == std::string::npos)
        start = 0;

    std::string::size_type count = (found + 3) - start;
    if (count > path.size() - start)
        count = path.size() - start;

    path.erase(start, count);

    goUpInPath(path);                           // recurse until no "/../" remains
}

// Returns the final path component without its last extension. Names without a dot remain unchanged.
std::string extractFileNameWithoutExt(const std::string& path)
{
    const std::string name = extractFileName(path);
    const std::string::size_type dot = name.rfind('.');
    if (dot == std::string::npos)
        return name;
    return name.substr(0, dot);
}
// Returns the component after the last slash of either kind. A trailing slash produces an empty name.
std::string extractFileName(const std::string& path)
{
    const std::string::size_type backslash = path.rfind('\\');
    const std::string::size_type slash = path.rfind('/');

    std::string::size_type last = backslash;
    if (slash != std::string::npos && (last == std::string::npos || slash > last))
        last = slash;

    if (last == std::string::npos)
        return path;
    return path.substr(last + 1);
}
// Converts multibyte text to wide characters; failed conversion returns an empty string.
std::wstring toWideString(const std::string& s)
{
    const size_t count = mbstowcs(0, s.c_str(), 0) + 1;         // + terminator; 0 on an invalid sequence
    wchar_t* buffer = new wchar_t[count];
    std::wstring result;
    if (mbstowcs(buffer, s.c_str(), count))
        result = buffer;
    else
        result = L"";
    delete[] buffer;
    return result;
}

} // namespace hfe
} // namespace dice
