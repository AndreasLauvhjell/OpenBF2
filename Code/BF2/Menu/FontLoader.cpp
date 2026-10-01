// [INFERRED] source path, client loaders7D3840/7D4E10/7D5440 and token helpers.
#include "Font.h"
#if !DEDICATED
#include "DifFontMetadata.h"
#include "../External/BFMeme/FileIStream.h"
#include "../GeomMisc/MenuRend/IMemeTexture.h"
#include "../Core/ClassManager.h"
#include "../Core/Ids.h"
#include "../Core/StringUtil.h"
#include "../IO/File/FileManager.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace dice { namespace hfe {
extern ClassManager* classManager;
namespace menu {
namespace {
bool skipWhitespace(meme::IStream* stream)
{
    signed char character;
    if (stream->read(&character, 1) != 1) return false;
    while (character <= 32)
        if (stream->read(&character, 1) != 1) return false;
    stream->setPosition(stream->getPosition() - 1);
    return true;
}
bool readToken(meme::IStream* stream, char* text)
{
    if (!skipWhitespace(stream)) return false;
    int count = 0;
    bool whitespace = false;
    signed char character;
    while (stream->read(&character, 1) == 1)
    {
        if (character <= 32)
        {
            stream->setPosition(stream->getPosition() - 1);
            whitespace = true;
            break;
        }
        text[count++] = static_cast<char>(character); // no capacity check in PE
    }
    if (!count && !whitespace) return false;
    text[count] = 0;
    return true;
}
bool skipLine(meme::IStream* stream)
{
    char character;
    do { if (stream->read(&character, 1) != 1) return false; }
    while (character != 10);
    return true;
}
}
Font* loadFontFile(const char* name)
{
    std::string path(name);
    if (!io::fileManager->fileExists(path))
    {
        path = name;
        if (!io::fileManager->fileExists(path)) return 0;
    }
    const std::string extension = extractFileExt(path);
    if (equalsIgnoreCase(extension, std::string("font"))) return loadBitmapFont(name);
    if (equalsIgnoreCase(extension, std::string("dif"))) return loadDifFont(name);
    return 0;
}
Font* loadDifFont(const char* name)
{
    const std::string path(name);
    DifFontMetadata* metadata = new DifFontMetadata(path);
    std::string folder;
    const std::string existencePath(name);
    if (io::fileManager->fileExists(existencePath))
        folder = extractFilePath(std::string(name), '/');
    if (folder == "")
    {
        if (metadata) delete metadata;
        return 0;
    }
    geom::IMemeTexture* texture = static_cast<geom::IMemeTexture*>(
        classManager->createInstance(geom::CID_MemeTexture, geom::IID_IMemeTexture, 0));
    texture->setTexture(metadata->getTextureName().c_str(), folder.c_str(), 0);
    texture->setByte66(1);
    DifFont* font = new DifFont(name);
    font->m_texture = texture;
    font->m_metadata = metadata;
    return font;
}
Font* loadBitmapFont(const char* name)
{
    std::string path;                             // shipped first lookup is empty
    if (!io::fileManager->fileExists(path))
    {
        path = std::string("Fonts/") + name;
        if (!io::fileManager->fileExists(path)) return 0;
    }
    meme::FileIStream stream(path.c_str());
    BitmapGlyph glyphs[256];                      // unlisted glyph records untouched
    geom::IMemeTexture* texture = 0;
    float scaleU = 0.0f, scaleV = 0.0f;
    int height = 0;
    char token[256];
    bool haveToken = readToken(&stream, token);
    while (haveToken)
    {
        if (token[0] == '#')
        {
            if (!skipLine(&stream)) break;
        }
        else if (std::strncmp(token, "0x", 2) == 0)
        {
            if (!texture) return 0;
            unsigned int index;
            std::sscanf(token, "%x", &index);       // retail has no index guard
            readToken(&stream, token); const int u = std::atoi(token);
            readToken(&stream, token); const int v = std::atoi(token);
            readToken(&stream, token);
            glyphs[index].u = static_cast<float>(static_cast<double>(u) * scaleU);
            glyphs[index].v = static_cast<float>(static_cast<double>(v) * scaleV);
            glyphs[index].width = static_cast<float>(static_cast<double>(std::atoi(token)) * scaleU);
            glyphs[index].height = static_cast<float>(static_cast<double>(height) * scaleV);
            if (!skipLine(&stream)) break;
        }
        else if (std::strcmp(token, "Texture") == 0)
        {
            readToken(&stream, token); readToken(&stream, token);
            texture = static_cast<geom::IMemeTexture*>(classManager->createInstance(
                geom::CID_MemeTexture, geom::IID_IMemeTexture, 0));
            texture->setTexture(token, "", 0);
            const float uExtent = texture->getUvWidthCopy();
            scaleU = static_cast<float>(static_cast<double>(uExtent) / texture->getPixelWidth());
            const float vExtent = texture->getUvHeightCopy();
            scaleV = static_cast<float>(static_cast<double>(vExtent) / texture->getPixelHeight());
        }
        else if (std::strcmp(token, "CharacterHeight") == 0)
        {
            readToken(&stream, token); readToken(&stream, token);
            height = std::atoi(token);
        }
        else if (std::strcmp(token, "Version") == 0)
        {
            readToken(&stream, token); readToken(&stream, token);
            if (std::atoi(token) != 1) return 0;     // acquired texture leaks in retail
        }
        haveToken = readToken(&stream, token);
    }
    BitmapFont* font = new BitmapFont(name);
    font->m_height = static_cast<float>(height);
    std::memcpy(font->m_glyphs, glyphs, sizeof(glyphs));
    font->m_scaleV = scaleV;
    font->m_scaleU = scaleU;
    font->m_texture = texture;
    return font;
}
} } }
#endif
