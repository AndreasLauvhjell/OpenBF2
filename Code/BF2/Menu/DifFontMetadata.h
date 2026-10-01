// [INFERRED] native client .dif metadata class and path. Nonpolymorphic.
// BF2.exe ctor7D6A90, dtor7D6930; VC7 allocation120 contains three maps/two strings.
#ifndef MENU_DIFFONTMETADATA_H
#define MENU_DIFFONTMETADATA_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
namespace dice { namespace meme { class IStream; } }
namespace dice { namespace hfe { namespace menu {
struct DifGlyph
{
    std::uint16_t character;                      // +00; padding+02 untouched
    float leftBearing, width, rightBearing;       // +04/+08/+0C
    std::uint16_t x0, y0, x1, y1;                 // +10..+16
    float verticalOffset, scaledHeight;           // +18/+1C
};
class DifFontMetadata
{
public:
    explicit DifFontMetadata(const std::string&);
    ~DifFontMetadata();
    DifGlyph* findGlyph(int);                      // 7D5D20, low16 key
    float getKerning(int, int);                    // 7D5D60, packed unsigned key
    void readHeader(std::istream&);
    void readKerning(std::istream&);
    void readGlyphs(std::istream&);
    void clearScaledGlyphs();                     // 7D63F0, preserves map keys
    void scaleGlyphs(float, float);
    void buildScaledGlyphs(int);
    static int resolutionForWidth(int&);
    float getScale() const { return m_scale; }
    float getLineHeight() const { return m_lineHeight; }
    unsigned int getTextureWidth() const { return m_textureWidth; }
    unsigned int getTextureHeight() const { return m_textureHeight; }
    const std::string& getTextureName() const { return m_textureName; }
private:
    void readStream(meme::IStream*, unsigned int); // factored duplicate7D6A90 branches
    float m_scale;                                // VC7+00
    std::map<std::uint16_t, DifGlyph*> m_original;  // VC7+04
    std::map<std::uint16_t, DifGlyph*> m_scaled;    // VC7+10
    int m_resolutionIndex;                        // VC7+1C, only set after file read
    std::uint32_t m_field20;                      // VC7+20, untouched
    std::map<std::uint32_t, float> m_kerning;      // VC7+24
    unsigned int m_version, m_textureWidth, m_textureHeight; // VC7+30..38
    std::string m_headerName, m_textureName;       // VC7+3C/+58
    float m_lineHeight;                           // VC7+74
};
} } }
#endif
#endif
