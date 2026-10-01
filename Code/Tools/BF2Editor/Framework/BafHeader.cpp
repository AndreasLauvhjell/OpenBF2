// Code/Tools/BF2Editor/Framework/BafHeader.cpp - see BafHeader.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/BafHeader.h"

#include <cstring>

namespace bf2editor
{

namespace
{

template <typename T>
bool take(const std::string& bytes, size_t& at, T& value)
{
    if (at + sizeof(T) > bytes.size())
        return false;
    std::memcpy(&value, bytes.data() + at, sizeof(T));      // little-endian, as the files are
    at += sizeof(T);
    return true;
}

} // namespace

bool readBafHeader(const std::string& bytes, BafHeader& header, std::string& error)
{
    header = BafHeader();
    size_t at = 0;
    int version = 0;
    if (!take(bytes, at, version) || version != 4)
    {
        error = "not a version 4 .baf";
        return false;
    }
    header.version = version;
    unsigned short bones = 0;
    if (!take(bytes, at, bones))
    {
        error = "the .baf ends inside its header";
        return false;
    }
    header.boneIds.resize(bones);
    for (unsigned short i = 0; i < bones; ++i)
        if (!take(bytes, at, header.boneIds[i]))
        {
            error = "the .baf ends inside its bone list";
            return false;
        }
    if (!take(bytes, at, header.frames) || !take(bytes, at, header.precision))
    {
        error = "the .baf ends inside its header";
        return false;
    }
    return true;
}

} // namespace bf2editor
