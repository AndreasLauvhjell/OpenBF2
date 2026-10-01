// [TEMP] [CHOSEN, not recovered] UI resource-only fixture. Real Ref2System
// constructor/dispatch/cache/release and real font loader run; init/close/paint,
// descriptors and actual renderer remain uncalled. No production overrides.
#include "font_runtime_support.h"
#define private public
#define protected public
#include "../BF2/Menu/Ref2System.h"
#undef protected
#undef private
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/External/BFMeme/IStream.h"
namespace dice { namespace hfe { SettingsRepostitory* settings=nullptr; } }
static std::vector<std::string> events;
struct ResourceTexture : Texture
{
    std::string id;
    void* link=nullptr;
    int flags=-1;
    int adds=0, sets=0, otherDraws=0;
    int pixelWidth=-17,pixelHeight=31;
    geom::IMemeTexture* other=nullptr;
    dice::meme::Quad* request=nullptr;
    bool drawResult=true;
    void addRef() override { ++adds;events.push_back(id+":addRef"); }
    int release() override { ++releases;events.push_back(id+":release");return 0; }
    void setTexture(const char* n,const char* p,int f) override
    { Texture::setTexture(n,p,f);flags=f;++sets;events.push_back(id+":setTexture"); }
    const char* getName() const override { events.push_back(id+":name");return name.c_str(); }
    void setUserObject(void* value) override { link=value;events.push_back(id+":setLink"); }
    void* getUserObject() const override { events.push_back(id+":getLink");return link; }
    void releaseTexture() override { events.push_back(id+":releaseTexture"); }
    bool drawOther(geom::IMemeTexture* value,dice::meme::Quad* q) override
    { ++otherDraws;other=value;request=q;events.push_back(id+":drawOther");return drawResult; }
    int getPixelWidth() const override { events.push_back(id+":width");return pixelWidth; }
    int getPixelHeight() const override { events.push_back(id+":height");return pixelHeight; }
};
struct ResourceClasses : Classes
{
    std::vector<ResourceTexture*> textures;
    IBase* createInstance(unsigned int cid,unsigned int iid,IBase* outer) override
    {
        if(cid==io::CID_MemStream)return Classes::createInstance(cid,iid,outer);
        check(cid==geom::CID_MemeTexture && iid==geom::IID_IMemeTexture && !outer,"resource texture factory IDs");
        ResourceTexture* t=new ResourceTexture;
        t->id=std::to_string(textures.size());textures.push_back(t);
        events.push_back(t->id+":create");return t;
    }
};
struct FontRecorder : Font
{
    explicit FontRecorder(const char* name) : Font(name) {}
    int draws=0,metrics=0,kernings=0,heights=0,a=0,b=0;
    dice::meme::Quad* quad=nullptr;
    bool drawCharacter(int value,dice::meme::Quad* q) override { ++draws;a=value;quad=q;return false; }
    bool getGlyphMetrics(int value,dice::meme::GlyphMetrics* out) override
    { ++metrics;a=value;out->width=13;return false; }
    float getKerning(int first,int second) override { ++kernings;a=first;b=second;return -3.5f; }
    float getHeight() override { ++heights;return 21.25f; }
};
struct OutputRecorder : dice::meme::OStream
{
    std::string text;
    int count=0,calls=0;
    int write(const void* bytes,int n) override
    { ++calls;count=n;text=static_cast<const char*>(bytes);return -1; }
};
struct OutputSystem : Ref2System
{
    OutputSystem() : Ref2System(800,600) {}
    dice::meme::OStream* output=nullptr;
    int queries=0;
    dice::meme::OStream* getOutputStream() override { ++queries;return output; }
};
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    Files* files=new Files; // same explicitly process-lifetime fixture boundary
    ResourceClasses classes;io::fileManager=files;classManager=&classes;
    Ref2System root(800,600);
    files->data["direct.ui"]="ABC";
    files->data["menu/nested.ui"]="DEFG";
    files->exists.clear();files->opens.clear();
    dice::meme::IStream* input=root.createInputStream("direct.ui");
    check(input && files->exists==std::vector<std::string>({"direct.ui"}) && files->opens==std::vector<std::string>({"direct.ui"}),"direct input existence/open path");
    char buffer[5]={};
    check(input->getAvailableBytes()==3 && input->read(buffer,3)==3 && std::string(buffer)=="ABC","direct input real memory read");
    delete input;
    files->exists.clear();files->opens.clear();
    input=root.createInputStream("nested.ui");
    check(input && files->exists==std::vector<std::string>({"nested.ui","menu/nested.ui"}) && files->opens==std::vector<std::string>({"menu/nested.ui"}),"menu fallback exact lookup order");
    check(input->read(buffer,4)==4 && std::string(buffer)=="DEFG","menu input real memory read");
    delete input;
    files->exists.clear();files->opens.clear();
    check(!root.createInputStream("absent.ui") && files->exists==std::vector<std::string>({"absent.ui","menu/absent.ui"}) && files->opens.empty(),"missing input returns null without open");
    check(root.getOutputStream()==&root.m_output,"embedded output virtual returned");
    check(root.getOutputStream()->write("longer text",1)==1 && root.getOutputStream()->write("",-12)==-12,"retail output returns caller count unchanged");
    OutputRecorder output;OutputSystem printer;
    dice::meme::systemPrintf(&printer,nullptr);
    check(printer.queries==1 && !output.calls,"null output suppresses even format read");
    printer.output=&output;
    dice::meme::systemPrintf(&printer,"%s:%d", "abc",42);
    check(printer.queries==2 && output.calls==1 && output.text=="abc:42" && output.count==6,"systemPrintf formats then writes strlen once");
    root.m_textureFlags=0xA5;
    int first=root.loadTexture("Alpha",true);
    ResourceTexture* a=static_cast<ResourceTexture*>(reinterpret_cast<geom::IMemeTexture*>(first));
    check(a->prefix=="Menu/HUD/Texture/" && a->name=="Alpha" && a->flags==0xA5,"texture exact prefix/name/flags");
    check(events==std::vector<std::string>({"0:create","0:setTexture","0:setLink"}),"texture construction call order");
    events.clear();
    check(root.loadTexture("aLPHA",false)==first && classes.textures.size()==1,"case-insensitive linked cache hit");
    check(a->sets==1 && a->adds==0 && events==std::vector<std::string>({"0:name"}),"cache hit does not reload/addref");
    int second=root.loadTexture("Beta",false);
    ResourceTexture* b=static_cast<ResourceTexture*>(reinterpret_cast<geom::IMemeTexture*>(second));
    check(b->prefix.empty() && b->link==a && root.m_textureList==b,"new texture prepends singly linked cache");
    events.clear();check(root.loadTexture("ALPHA",true)==first,"cache walks through first node");
    check(events==std::vector<std::string>({"1:name","1:getLink","0:name"}),"cache lookup order");
    dice::meme::Rect bounds{0,0,2,3,0};dice::meme::Quad q(&bounds,nullptr);
    ResourceTexture blank;blank.id="blank";root.m_blankTexture=&blank;
    root.m_field054=second;a->drawResult=false;
    check(!root.drawTexture(first,&q) && a->other==a && a->request==&q,"explicit draw passes self and result");
    check(root.drawTexture(0,&q) && b->other==b && b->request==&q,"default texture draw passes self");
    root.m_field054=0;blank.drawResult=false;
    check(!root.drawTexture(0,&q) && blank.other==nullptr && blank.request==&q,"blank texture receives null other/result");
    check(root.getTextureName(0)==nullptr && root.getTextureWidth(0)==0 && root.getTextureHeight(0)==0,"null texture queries");
    events.clear();
    check(std::string(root.getTextureName(first))=="Alpha" && root.getTextureWidth(first)==-17 && root.getTextureHeight(first)==31,"name and signed texture dimensions");
    check(events==std::vector<std::string>({"0:name","0:width","0:height"}),"texture accessor slots");
    events.clear();root.releaseTexture(first);
    check(root.m_textureList==b && !b->link && a->link==nullptr && a->releases==1,"tail release unlinks and releases once");
    check(events==std::vector<std::string>({"1:getLink","0:getLink","1:setLink","0:releaseTexture","0:setLink","0:release"}),"tail release callback order");
    events.clear();root.releaseTexture(first);root.releaseTexture(0);
    check(a->releases==1 && events==std::vector<std::string>({"1:getLink"}),"unknown and null release are no-ops");
    events.clear();root.releaseTexture(second);
    check(root.m_textureList==nullptr && b->releases==1,"head release clears list");
    check(events==std::vector<std::string>({"1:getLink","1:releaseTexture","1:setLink","1:release"}),"head release callback order");

    FontRecorder alpha("alpha.font"),zeta("zeta.font");
    dice::meme::GlyphMetrics metric{};
    check(!root.drawCharacter(65,0,&q),"empty font draw returns false");
    check(!root.drawCharacter(-128,reinterpret_cast<int>(&zeta),&q) && zeta.a==-128 && zeta.quad==&q,"explicit signed font dispatch");
    root.m_fonts.insert(std::make_pair("zeta",&zeta));root.m_fonts.insert(std::make_pair("alpha",&alpha));
    check(!root.drawCharacter(0x10041,0,&q) && alpha.a==0x10041 && alpha.draws==1,"default font chooses first ordered key/full int");
    check(!root.getGlyphMetrics(-128,0,&metric) && alpha.metrics==1 && alpha.a==-128 && metric.width==13,"default metrics forwards false and output");
    check(!root.getGlyphMetrics(0x10041,reinterpret_cast<int>(&zeta),&metric) && zeta.a==0x10041 && zeta.metrics==1,"explicit full-width metrics");
    check(root.getKerning(-128,0x10041,0)==-3.5f && alpha.a==-128 && alpha.b==0x10041,"default kerning two signed ints");
    check(root.getKerning(3,4,reinterpret_cast<int>(&zeta))==-3.5f && zeta.a==3 && zeta.b==4,"explicit kerning");
    check(root.getFontHeight(0)==21.25f && alpha.heights==1,"default font height");
    check(root.getFontHeight(reinterpret_cast<int>(&zeta))==21.25f && zeta.heights==1,"explicit font height");
    check(root.getFontName(0)==nullptr && std::string(root.getFontName(reinterpret_cast<int>(&zeta)))=="zeta.font","font name/null accessors");
    root.m_fonts.clear();

    files->data["Fonts/resources.dif"]="header 1 Resource Font\n64 12\nglyphs 1\n65 1 2 3 0 1 2 3 4\n";
    int fontHandle=root.loadFont("Fonts/resources.dif");
    Font* font=reinterpret_cast<Font*>(fontHandle);
    check(font && font->referenceCount()==1 && root.m_fonts["Fonts/resources.dif"]==font,"first real load caches original key/ref1");
    const auto opened=files->opens.size();const auto textures=classes.textures.size();
    check(root.loadFont("Fonts/resources.dif")==fontHandle && font->referenceCount()==1,"local font cache hit leaves refs unchanged");
    check(files->opens.size()==opened && classes.textures.size()==textures,"local font hit performs no IO/texture load");
    { Ref2System another(800,600);
      check(another.loadFont("Fonts/resources.dif")==fontHandle && font->referenceCount()==2,"shared font cache increments once per root");
      check(another.m_fonts["Fonts/resources.dif"]==font && files->opens.size()==opened && classes.textures.size()==textures,"shared hit inserts key without loading");
      check(another.loadFont("Fonts/resources.dif")==fontHandle && font->referenceCount()==2,"shared root repeat hits local cache");
    }
    check(font->referenceCount()==2,"destructor does not perform close-owned refcount release");
    // Shared font and texture objects deliberately survive to process exit:
    // close is an unresolved UI lifecycle boundary, never substituted here.
    std::printf("Ref2 resource runtime: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
