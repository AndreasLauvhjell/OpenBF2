// Engine string/path helpers from the attested Core/StringUtil.cpp translation unit.
// This header declares narrow-string helpers; the original also instantiates wide counterparts.

#ifndef CORE_STRINGUTIL_H     // [INFERRED] guard style
#define CORE_STRINGUTIL_H

#include <string>

namespace dice {
namespace hfe {

// Formats printf-style arguments into a string. [UNRESOLVED] Original return type beyond observed string consumers.
std::string formatString(const char* fmt, ...);
#if !DEDICATED
// Client PE 431130: wide overload, including the shared buffer and overflow warning.
// [INFERRED] overload spelling; the dedicated ELF has no HUD caller for it.
std::wstring formatString(const wchar_t* fmt, ...);
#endif

// Converts multibyte text to wide characters, returning an empty string on failure. [INFERRED] Name.
std::wstring toWideString(const std::string& s);

// Case-insensitive comparison
// Four overloads exist. The (const char*, const char*) form is a straight strcasecmp; the
// std::string forms null-terminate their operands first and then delegate to it.
int  compareIgnoreCase(const char* a, const char* b);
int  compareIgnoreCase(const char* a, const char* b, int n);
int  compareIgnoreCase(const std::string& a, const std::string& b);
int  compareIgnoreCase(const std::string& a, const std::string& b, int n);

bool equalsIgnoreCase(const std::string& a, const std::string& b);
bool equalsIgnoreCase(const std::string& a, const std::string& b, int n);

// Case conversion
// The set* forms are 479 bytes each - they go through the locale, not a naive toupper loop.
// The get* forms are 46-byte wrappers that copy and then call the corresponding set*.
void        setUpperCase(std::string&);
void        setLowerCase(std::string&);
std::string getUpperCase(const std::string&);
std::string getLowerCase(const std::string&);
std::wstring makeWide(const std::string&);
std::string  makeNarrow(const std::wstring&);

// Returns the case-insensitive hash used for engine hash-table bucket selection.
unsigned int calcStringHashValueNoCase(const std::string&);

// Paths
// note the difference between the two "fix" families, which is easy to miss and is load-bearing:
//   fixPathDelimiters  normalises separators only
//   fixPath            normalises separators and guarantees a trailing separator
// getFixPath / getFixPathDelimiters are the copying forms of each.
void        fixPathDelimiters(std::string&, char);
void        fixPath(std::string&, char);
std::string getFixPathDelimiters(const std::string&, char);
std::string getFixPath(const std::string&, char);

bool        isPathAbsolute(const std::string&);
// Normalizes repeated path separators before parent traversal.
// [CHOSEN, not recovered] The helper body still needs direct binary verification.
void        removeDoubleSlashes(std::string&);
void        goUpInPath(std::string&);

std::string extractFilePath(const std::string&, char);
std::string extractFileName(const std::string&);
std::string extractFileNameWithoutExt(const std::string&);
std::string extractFileExt(const std::string&);
std::string extractEverythingExceptExt(const std::string&);                  // internal 0x83d17f0

// Misc
// Returns a shared empty std::string. Used as a sentinel return by FileManager::fileExistsEx and
// getFileInfo rather than constructing a fresh empty string each time.
const std::string& getEmptyString();

} // namespace hfe
} // namespace dice

#endif // CORE_STRINGUTIL_H
