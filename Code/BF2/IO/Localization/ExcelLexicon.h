// Localization table mapping narrow keys to wide strings. Header path [INFERRED].
// Lookup uppercases the key and hashes without case; a miss returns the original key widened.
// Preserve the virtual destructor after load and getString; this service uses reference-counted lifetime.

#ifndef IO_LOCALIZATION_EXCELLEXICON_H          // [INFERRED] guard style, per Core/IBase.h
#define IO_LOCALIZATION_EXCELLEXICON_H

#include <string>

#include "../../Core/IBase.h"
#include "../../Core/Hash.h"

namespace dice {
namespace hfe {
namespace io {

// Existing singleton storage in Core/Globals.cpp and ModuleRegistry.cpp uses IBase*.
extern IBase* lexicon;

// Localized-string interface. [CHOSEN, not recovered] Interface/concrete boundary; preserve virtual order.
class ILexicon : public IBase
{
public:
    virtual bool         load(const std::string& filename)          = 0;   // +0x10
    virtual std::wstring getString(const std::string& key) const    = 0;   // +0x14

    virtual ~ILexicon() {}                                                 // +0x18 / +0x1C
};

// Localized-string table with uppercase lookup keys and 4711 buckets.
// [UNRESOLVED] Purpose of the unused default-string member.
class ExcelLexicon : public ILexicon
{
public:
    ExcelLexicon();

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;           // 0x8465760  - _ZNK..., const

    // ILexicon, +0x10..+0x14
    virtual bool         load(const std::string& filename);
    virtual std::wstring getString(const std::string& key) const;

    // +0x18 / +0x1C
    virtual ~ExcelLexicon();

    // The factory ClassManager::createInstance calls.
    // _ZN4dice3hfe2io12ExcelLexicon6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* 0x04 */ int          m_refCount;
    /* 0x08 */ std::wstring m_default;      // [UNRESOLVED] purpose - never read anywhere
    /* 0x0C */ Hash<std::string, std::wstring, StringHashFunc> m_strings;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_LOCALIZATION_EXCELLEXICON_H
