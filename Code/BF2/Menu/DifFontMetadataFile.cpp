// [INFERRED] source split for ctor7D6A90; native strings/strstream, no VC7 blobs.
#include "DifFontMetadata.h"
#if !DEDICATED
#include "../External/BFMeme/FileIStream.h"
#include "../Core/StringUtil.h"
#include <strstream>
namespace dice { namespace hfe { namespace menu {
DifFontMetadata::DifFontMetadata(const std::string& name)
    : m_scale(1.0f)
{
    m_textureName = extractFileNameWithoutExt(name);
    std::string path = extractFilePath(name, '/') + m_textureName + ".dif";
    meme::FileIStream first(path.c_str());
    unsigned int size = static_cast<unsigned int>(first.getAvailableBytes());
    if (size)
        readStream(&first, size);
    else
    {
        path = extractFilePath(name, '/') + m_textureName + ".dif";
        meme::FileIStream second(path.c_str());      // same path, both streams alive
        size = static_cast<unsigned int>(second.getAvailableBytes());
        if (size) readStream(&second, size);
    }
    // No-file branch preserves untouched resolution/header/dimension/height scalars.
}
void DifFontMetadata::readStream(meme::IStream* input, unsigned int size)
{
    char* bytes = new char[size];
    input->read(bytes, static_cast<int>(size));     // returned byte count ignored
    std::istrstream stream(bytes, static_cast<int>(size));
    while (!stream.eof())
    {
        std::string token;
        stream >> token;
        if (token == "") break;
        if (token == "header") readHeader(stream);
        else if (token == "kerning") readKerning(stream);
        else if (token == "glyphs") readGlyphs(stream);
    }
    delete[] bytes;                                // before strstream destruction
    m_resolutionIndex = 0;
    int width = 800;
    int index = resolutionForWidth(width);
    if (index <= 0 || index >= 9) index = 2;
    m_resolutionIndex = index;
    if (width <= 800)
        m_scale = static_cast<float>(static_cast<double>(width) * 0.0012499999720603228f);
    else if (width >= 1024)
        m_scale = static_cast<float>(static_cast<double>(width) * 0.0009765625f);
    buildScaledGlyphs(index);
}
} } }
#endif
