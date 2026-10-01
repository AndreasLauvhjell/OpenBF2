#include "font_runtime_support.h" // [TEMP] recorded files/texture collaborators
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    // FileManager's unrelated destructor remains a frontier: this one TEMP
    // collaborator is intentionally process-lifetime, never deleted.
    Files* files=new Files;
    Classes classes;
    io::fileManager=files; classManager=&classes;
    const std::string content=
        "header 2 Fixture Face\n256 128 19\nkerning 2\n65 65 -2\n65 65 99\n"
        "glyphs 4\n65 1.25 7 2.5 -3 10 20 16 29\n"
        "32 0 4 0 0 1 2 5 9\n128 2 3 4 1 20 30 22 35\n"
        "65408 9 10 11 -2 40 50 44 57\n";
    files->data["Fonts/face.dif"]=content;
    {
        Font base("plain.font"); dice::meme::GlyphMetrics untouched{1,2,3,4,5};
        check(base.referenceCount()==0 && std::string(base.getName())=="plain.font","base native name/refcount");
        check(!base.getGlyphMetrics(0,&untouched) && untouched.height==4,"base metrics false without writes");
        check(!base.drawCharacter(0,nullptr) && base.getHeight()==0 && base.getKerning(1,2)==0,"base five-slot defaults");
        Font* loaded=loadFontFile("Fonts/face.dif");
        check(loaded!=nullptr,"real dif loader");
        DifFont* font=static_cast<DifFont*>(loaded);
        DifFontMetadata* meta=font->m_metadata;
        check(meta->m_headerName=="Fixture Face" && meta->m_textureName=="face","native header/name extraction");
        check(meta->m_version==2 && meta->getTextureWidth()==256 && meta->getTextureHeight()==128,"header dimensions");
        check(meta->getLineHeight()==19 && meta->m_resolutionIndex==2 && meta->getScale()==1,"constructor fixed800 resolution");
        check(meta->m_original.size()==4 && meta->m_scaled.size()==4,"native glyph maps");
        check(meta->findGlyph(0x10041)==meta->findGlyph(65),"full-width key narrows at metadata");
        check(meta->findGlyph(-128)==meta->findGlyph(65408),"signed key narrows low16");
        check(meta->getKerning(0x10041,0x10041)==-2 && meta->m_kerning.size()==1,"packed key and first duplicate wins");
        check(texture.name=="face" && texture.prefix=="Fonts/" && texture.flag==1,"dif texture setup");
        dice::meme::GlyphMetrics metric{90,91,92,93,94};
        check(!font->getGlyphMetrics(999,&metric) && metric.leftBearing==90 && metric.height==93,"missing metric leaves output");
        check(font->getGlyphMetrics(0x10041,&metric) && metric.leftBearing==1 && metric.width==7 && metric.height==9 && metric.verticalOffset==-3,"dif metrics values/full char");
        check(font->getGlyphMetrics(65,&metric) && metric.leftBearing==-1,"shared previous full char virtual kerning");
        ObservingFont second("second.dif"); second.m_metadata=new DifFontMetadata("Fonts/face.dif");
        check(second.getGlyphMetrics(65,&metric) && metric.leftBearing==-1,"previous char shared across font objects");
        second.getGlyphMetrics(0x10041,&metric);
        second.getGlyphMetrics(-128,&metric);
        check(second.previous==0x10041 && second.current==-128,"virtual kerning receives both full signed ints");
        second.getGlyphMetrics(999,&metric);
        check(font->getGlyphMetrics(65,&metric) && metric.leftBearing==1,"missing lookup globally resets previous");
        check(font->getGlyphMetrics(-128,&metric) && metric.width==10,"metrics preserves negative until ushort lookup");
        dice::meme::Rect bounds{1,2,7,9,0};dice::meme::Quad quad(&bounds,nullptr);
        int before=texture.draws;
        check(font->drawCharacter(-128,&quad) && texture.draws==before+1,"draw negative instead uses unsigned byte");
        check(closeFloat(texture.uv[0],20.01f/256) && closeFloat(texture.uv[3],35.01f/128),"biased UV rectangle");
        check(quad.corners[1].x==7 && quad.corners[2].x==7,"draw subtract/add width stores");
        before=texture.draws;
        check(font->drawCharacter(32,&quad) && texture.draws==before,"space skips draw after lookup");
        check(!font->drawCharacter(999,&quad) && texture.draws==before,"missing draw false");
        meta->m_scale=2;
        font->drawCharacter(65,&quad);
        check(texture.uv[0]==10.0f/256,"scaled draw removes bias");
        dice::meme::Style style("fixture",reinterpret_cast<int>(font));
        style.prepareFont();check(style.m_scale==2 && style.m_reciprocalScale==0.5f,"real Style dif preparation");
        style.m_font=reinterpret_cast<int>(&base);style.prepareFont();
        check(style.m_scale==1 && style.m_reciprocalScale==1,"real Style other preparation");
        meta->buildScaledGlyphs(1);
        check(closeFloat(meta->findGlyph(65)->width,8.75f) && meta->findGlyph(65)->verticalOffset==-3.75f,"resolution640 scaling/floor negative");
        check(meta->findGlyph(65)->y1==29 && meta->findGlyph(65)->scaledHeight==11.25f,"scaled height retains raw rect");
        meta->buildScaledGlyphs(3);check(closeFloat(meta->findGlyph(65)->width,7.03125f),"resolution1024 scale");
        meta->buildScaledGlyphs(4);check(closeFloat(meta->findGlyph(65)->width,7.6388884f),"resolution1152 scale");
        meta->buildScaledGlyphs(5);check(meta->findGlyph(65)->width==6.875f,"resolution1280 scale");
        meta->buildScaledGlyphs(6);check(meta->findGlyph(65)->width==7,"resolution1600 copy path");
        meta->clearScaledGlyphs();check(meta->m_scaled.size()==4 && !meta->findGlyph(65),"clear preserves keys");
        meta->buildScaledGlyphs(2);
        const int released=texture.releases;delete font;
        check(texture.releases==released+1,"dif texture release once");
    }
    files->data["Fonts/legacy.font"]="Version = 1\nTexture = atlas\nCharacterHeight = 12\n0x41 16 32 8\n";
    Font* bitmap=loadBitmapFont("legacy.font");
    check(bitmap!=nullptr && bitmap->getHeight()==12,"real bitmap parser height");
    dice::meme::GlyphMetrics bm;
    check(bitmap->getGlyphMetrics(65,&bm) && bm.leftBearing==0 && bm.width==8 && bm.rightBearing==0 && bm.height==12 && bm.verticalOffset==0,"bitmap metric divisors");
    dice::meme::Rect bounds{0,0,8,12,0};dice::meme::Quad q(&bounds,nullptr);
    check(bitmap->drawCharacter(65,&q),"bitmap ignores false texture result");
    check(texture.uv[0]==0.0625f && texture.uv[1]==0.125f && texture.uv[2]==0.09375f && texture.uv[3]==0.171875f,"bitmap UV values");
    const int releases=texture.releases;delete bitmap;
    check(texture.releases==releases+1,"bitmap texture release once");
    files->opens.clear();
    { DifFontMetadata missing("not-found.dif");check(missing.getScale()==1 && missing.m_original.empty(),"missing metadata keeps bounded ctor state");
      std::istringstream header("1 First Name\n64 7\n");missing.readHeader(header);
      check(missing.getTextureWidth()==64 && missing.getTextureHeight()==64 && missing.getLineHeight()==7,"version1 square dimensions");
      std::istringstream again("1 Second\n32 9\n");missing.readHeader(again);
      check(missing.m_headerName=="First NameSecond","repeated header appends name");
      std::istringstream glyph("1 66 1 2 3 -5 65537 65538 65540 65539\n");missing.readGlyphs(glyph);
      missing.buildScaledGlyphs(2);
      check(missing.findGlyph(66)->x0==1 && missing.findGlyph(66)->y0==2 && missing.findGlyph(66)->scaledHeight==1,"rect coordinates truncate to unsigned16 before height");
      missing.clearScaledGlyphs();missing.scaleGlyphs(0.125f,1.0f);
      check(missing.findGlyph(66)->scaledHeight==1,"zero scaled height restores raw height");
    }
    check(files->opens.size()==2 && files->opens[0]==files->opens[1],"metadata repeats same failed path");
    files->exists.clear();check(!loadFontFile("absent.font"),"load file absent false");
    check(files->exists.size()==2 && files->exists[0]==files->exists[1],"outer loader repeats same existence path");
    int widths[]={640,800,1024,1152,1280,1600,1920,2048};
    for(int i=0;i<8;++i)check(DifFontMetadata::resolutionForWidth(widths[i])==i+1,"resolution map");
    int unknown=999;check(DifFontMetadata::resolutionForWidth(unknown)==2 && unknown==800,"resolution fallback writes800");
    std::printf("font runtime: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
