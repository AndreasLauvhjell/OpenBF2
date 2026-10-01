// Names, namespace and placement [INFERRED] from the Ref2System client cluster.
// BF2.exe tables939A50/939ED0/939F10 each have exactly five virtual slots.
// Native containers replace compiler-specific VC7 storage.
#ifndef MENU_FONT_H
#define MENU_FONT_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <string>
namespace dice { namespace meme { struct Quad; struct GlyphMetrics; } }
namespace dice { namespace hfe {
namespace geom { class IMemeTexture; }
namespace menu {
class DifFontMetadata;
class Font
{
public:
    explicit Font(const char*);
    virtual ~Font();
    virtual bool drawCharacter(int, meme::Quad*);
    virtual float getHeight();
    virtual bool getGlyphMetrics(int, meme::GlyphMetrics*); // +0C 455420
    virtual float getKerning(int, int);

    // [INFERRED] source spelling for field access in7AD150/7AEBB0/7AEF10.
    const char* getName() const { return m_name.c_str(); }
    int& referenceCount() { return m_references; }
protected:
    std::string m_name;                           // VC7+04, native STL
    int m_references;                             // VC7+20
};
class DifFont : public Font
{
public:
    explicit DifFont(const char*);                // inlined7D4E10
    ~DifFont() override;
    bool drawCharacter(int, meme::Quad*) override;
    float getHeight() override;
    bool getGlyphMetrics(int, meme::GlyphMetrics*) override;
    float getKerning(int, int) override;
    const DifFontMetadata* getMetadata() const { return m_metadata; }
private:
    friend Font* loadDifFont(const char*);
    geom::IMemeTexture* m_texture;                // VC7+24
    DifFontMetadata* m_metadata;                  // VC7+28, owned
};
struct BitmapGlyph { float u, v, width, height; };
class BitmapFont : public Font
{
public:
    explicit BitmapFont(const char*);
    ~BitmapFont() override;
    bool drawCharacter(int, meme::Quad*) override;
    float getHeight() override;
    bool getGlyphMetrics(int, meme::GlyphMetrics*) override;
private:
    friend Font* loadBitmapFont(const char*);
    geom::IMemeTexture* m_texture;                // VC7+24
    float m_height;                              // VC7+28
    BitmapGlyph* m_glyphs;                        // VC7+2C, 256 owned records
    float m_scaleU, m_scaleV;                     // VC7+30/+34, ctor untouched
};
Font* loadFontFile(const char*);
Font* loadDifFont(const char*);
Font* loadBitmapFont(const char*);
} } }
#endif
#endif
