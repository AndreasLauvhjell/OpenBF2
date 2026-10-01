// [TEMP] [CHOSEN, not recovered] Native x86 Effect/Style closure fixture.
// Only renderer responses, character input and observations are fabricated.
// Real Effect traversal/defaults/descriptors, Style font preparation/drawing,
// Data indexed forwarding and Quad construction run. No UI init/close/paint.
#include "../BF2/External/BFMeme/Effect.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/Menu/Font.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace dice::meme;
namespace {
int checks=0, failures=0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr,"FAIL: %s\n",label); }
}
std::vector<std::string> events;
struct Draw
{
    int character, font;
    float x,y,width,height,z;
    Color color;
    const Rect* clip;
};
// [TEMP] Finite metrics and observed draw return. This is not a font backend.
struct DrawSystem : System
{
    DrawSystem() : System(800,600) {}
    std::vector<Draw> draws;
    std::vector<int> metricCharacters;
    bool result=true, metricResult=false;
    int rejectAt=0, texture=0;
    Quad* seenQuad=nullptr;
    float getFontHeight(int) override { events.push_back("height"); return 10.0f; }
    bool getGlyphMetrics(int character,int,GlyphMetrics* out) override
    {
        metricCharacters.push_back(character); events.push_back("metrics");
        *out=GlyphMetrics{1,4,2,6,2}; return metricResult;
    }
    bool drawCharacter(int character,int font,Quad* quad) override
    {
        events.push_back("system.character"); seenQuad=quad;
        const QuadCorner& p=quad->corners[0];
        draws.push_back(Draw{character,font,p.x,p.y,quad->corners[1].x-p.x,
            quad->corners[3].y-p.y,p.z,p.color,quad->clip});
        return result && static_cast<int>(draws.size())!=rejectAt;
    }
    bool drawTexture(int value,Quad* quad) override
    { events.push_back("system.texture");texture=value;seenQuad=quad;return result; }
};
// [TEMP] Interceptor delegates to the real base to resume the parent chain.
struct RecordEffect : Effect
{
    explicit RecordEffect(const char* value) : name(value) {}
    std::string name;
    RenderContext* seenContext=nullptr;
    System* seenSystem=nullptr;
    Quad* seenQuad=nullptr;
    int character=0,font=0,element=0,texture=0,updates=0;
    float delta=0;
    bool reject=false, updateResult=false;
    bool update(System* system,float value) override
    { ++updates;seenSystem=system;delta=value;return updateResult; }
    bool drawCharacter(System* s,int c,int f,int i,Quad* q,RenderContext* r) override
    {
        events.push_back(name+".character");seenContext=r;seenSystem=s;
        seenQuad=q;character=c;font=f;element=i;
        if(reject)return false;
        return Effect::drawCharacter(s,c,f,i,q,r);
    }
    bool drawTexture(System* s,int t,Quad* q,RenderContext* r) override
    {
        events.push_back(name+".texture");seenContext=r;seenSystem=s;seenQuad=q;texture=t;
        if(reject)return false;
        return Effect::drawTexture(s,t,q,r);
    }
};
struct Characters : Data
{
    Characters(std::initializer_list<int> input) : Data("fixture text"), values(input) {}
    std::vector<int> values, indices, elements;
    int getCharacterElement(System* s,int element,int index) override
    { elements.push_back(element);return Data::getCharacterElement(s,element,index); }
    int getCharacter(System*,int index) override
    {
        events.push_back("data");indices.push_back(index);
        return index<static_cast<int>(values.size()) ? values[index] : 0;
    }
};
struct DrawStyle : Style
{
    DrawStyle(int font,int shadow=0) : Style("fixture style",font) { m_shadowFont=shadow; }
    // Observation only: the real preparation and positioning always execute.
    void prepareFont() override { events.push_back("prepare");Style::prepareFont(); }
    void positionGlyph(Rect* out,const GlyphMetrics* m,float* x,float* y) override
    { events.push_back("position");Style::positionGlyph(out,m,x,y); }
};
struct FontVisitor : Stream
{
    int calls=0, value=0;
    System* seen=nullptr;
    bool result=false;
    bool streamFont(System* s,int* font,const char* label) override
    {
        ++calls;seen=s;value=*font;
        check(std::strcmp(label,"Font handle")==0,"Style property label");
        *font=73;return result;
    }
};
void chain()
{
    DrawSystem system;
    Rect bounds{1,2,3,4,5}; Quad quad(&bounds,&bounds);
    Effect base; Effect* typed=&base;
    check(!typed->setColor(1,2,3,4) && typed->getColor()==nullptr,
          "real base color hooks retain false/null defaults");
    check(typed->update(&system,-2.5f) && updateEffect(nullptr,nullptr,9),
          "real update default and null-effect guard");
    RecordEffect first("first"), last("last");
    check(!updateEffect(&system,&first,-2.5f) && first.updates==1 &&
          first.delta==-2.5f && first.seenSystem==&system,"typed update forwards and propagates false");
    RenderContext tail{nullptr,nullptr,0,0,0,0,0};
    RenderContext far{&tail,&last,0,0,0,0,0};
    RenderContext gap{&far,nullptr,0,0,0,0,0};
    RenderContext near{&gap,&first,0,0,0,0,0};
    RenderContext head{&near,nullptr,0,0,0,0,0};
    events.clear();
    check(drawContextCharacter(&head,&system,-128,0x12345678,-17,&quad),"character chain succeeds");
    check(events==std::vector<std::string>{"first.character","last.character","system.character"},
          "empty contexts skipped and parent effects visited in order");
    check(first.seenContext==&near && last.seenContext==&far && first.seenQuad==&quad &&
          last.character==-128 && last.font==0x12345678 && last.element==-17,
          "each Effect receives its matching context and unchanged full-width payload");
    check(system.draws.back().character==-128 && system.draws.back().font==0x12345678 &&
          system.seenQuad==&quad,"System receives exact character/font/quad fallback");
    events.clear();last.reject=true;
    check(!drawContextCharacter(&head,&system,0x10041,77,123,&quad),"Effect rejection propagated");
    check(events==std::vector<std::string>{"first.character","last.character"} &&
          last.character==0x10041 && last.element==123,"rejection stops renderer and retains high character bits");
    last.reject=false;system.result=false;events.clear();
    check(!drawContextTexture(&head,&system,-73,&quad),"texture fallback result propagated");
    check(events==std::vector<std::string>{"first.texture","last.texture","system.texture"} &&
          last.texture==-73 && system.texture==-73 && system.seenQuad==&quad,
          "texture chain order and signed handle preserved");
    first.reject=true;events.clear();
    check(!drawContextTexture(&head,&system,0,&quad) && events==std::vector<std::string>{"first.texture"},
          "first texture rejection prevents parent traversal");
    system.result=true;events.clear();
    check(drawContextCharacter(nullptr,&system,0x10041,-9,777,&quad) &&
          events==std::vector<std::string>{"system.character"},"null context falls directly to System");
    events.clear();
    check(typed->drawTexture(&system,31,&quad,&tail) && system.texture==31 &&
          events==std::vector<std::string>{"system.texture"},"base draw starts at supplied context parent");
}
void descriptors()
{
    for(TypeDescriptor* type : {Effect::getClassType(),Style::getClassType()})
    {
        const bool effect=type==Effect::getClassType();
        check(type==TypeDescriptor::find(effect ? "dice::meme::Effect" : "dice::meme::Style"),
              "descriptor registered with attested name");
        check(type->getBaseType()==Object::getClassType() && type->getDescription()==nullptr,
              "descriptor Object base and null description");
        check(type->getInstanceSize()==(effect ? 4u:24u),"descriptor native x86 allocation size");
        const int count=type->getInstanceCount();
        RuntimeObject* object=nullptr;
        check(createRuntimeObject(object,type) && object->getType()==type &&
              object->getObjectSize()==type->getInstanceSize(),"real descriptor factory and runtime creation");
        check(type->getInstanceCount()==count+1,"runtime helper increments concrete counter");
        check(static_cast<Object*>(object)->getName()==nullptr,"factory default name is null");
        check(destroyRuntimeObject(object) && !object && type->getInstanceCount()==count,
              "real destruction restores counter and clears ownership");
    }
    check(getEffectClassType()==Effect::getClassType(),"Stream Effect descriptor adapter");
    alignas(Style) unsigned char storage[sizeof(Style)];
    std::memset(storage,0xA5,sizeof(storage));
    Style* style=new(storage) Style;
    bool untouched=true;
    for(unsigned i : {8u,9u,10u,11u,16u,17u,18u,19u,20u,21u,22u,23u})
        untouched=untouched && storage[i]==0xA5;
    check(untouched,"constructor preserves shadow and scale object bytes");
    DrawSystem system; FontVisitor visitor;
    check(!style->streamProperties(&system,&visitor) && visitor.calls==1 && visitor.value==0 &&
          visitor.seen==&system,"Style property invokes typed font visitor and propagates false");
    visitor.result=true;
    check(style->streamProperties(&system,&visitor) && visitor.value==73,
          "font visitor mutation retained and true propagated");
    style->~Style();
}
void textDrawing()
{
    dice::hfe::menu::Font font("fixture.font"); // real native Font supports prepareFont name path
    const int handle=reinterpret_cast<int>(&font);
    Rect bounds{10,20,100,100,3}, clip{-1,-2,500,600,0};
    DrawSystem system; DrawStyle style(handle,-77);
    Characters text{-128,0x10041}; RecordEffect record("effect");
    RenderContext context{nullptr,&record,0,0,0,0,0};
    events.clear();
    style.drawText(&system,&text,919,&bounds,&clip,&context);
    check(system.draws.size()==4,"two glyphs draw shadow then main");
    check(events==std::vector<std::string>{"height","prepare","data","metrics","position",
          "effect.character","system.character","effect.character","system.character",
          "data","metrics","position","effect.character","system.character",
          "effect.character","system.character","data"},"real text preparation/metrics/effect/draw ordering");
    check(system.metricCharacters==std::vector<int>{-128,0x10041} &&
          text.indices==std::vector<int>{0,1,2} && text.elements==std::vector<int>{919,919,919},
          "Data element forwarding, complete characters and terminator query");
    const Draw& shadow=system.draws[0];const Draw& main=system.draws[1];
    check(shadow.font==-77 && main.font==handle && shadow.character==-128 &&
          system.draws[2].character==0x10041,"shadow/main font and complete character values");
    check(shadow.color.r==0 && shadow.color.g==0 && shadow.color.b==0 && shadow.color.a==1 &&
          main.color.r==1 && main.color.g==1 && main.color.b==1 && main.color.a==1,
          "shadow black and primary white Quad colors");
    check(main.x==11 && main.y==32 && main.width==4 && main.height==6 && main.z==3 && main.clip==&clip,
          "glyph bounds include bearing, line height, vertical offset and clip identity");
    check(system.draws[3].x==18 && record.element==1,"advance and Effect glyph index use character position");
    check(!system.metricResult && system.draws.size()==4,"false metric result ignored after writing finite outputs");

    for(int reject : {1,2})
    {
        DrawSystem stop;stop.rejectAt=reject;Characters input{'A','B'};
        style.drawText(&stop,&input,0,&bounds,&clip,nullptr);
        check(static_cast<int>(stop.draws.size())==reject && input.indices==std::vector<int>{0},
              reject==1 ? "shadow failure stops before primary/next character" : "primary failure stops before next character");
    }
    DrawStyle plain(handle);DrawSystem control;Characters controls{'A',13,'B',10,'C'};
    plain.drawText(&control,&controls,-3,&bounds,&clip,nullptr);
    check(control.draws.size()==3 && control.metricCharacters==std::vector<int>{'A','B','C'},
          "CR and LF produce neither glyph metrics nor draw");
    check(control.draws[0].x==11 && control.draws[0].y==32 && control.draws[1].x==11 &&
          control.draws[1].y==32 && control.draws[2].x==18 && control.draws[2].y==42,
          "CR resets x only; LF advances y without resetting x");
    Rect wrap{10,20,7,30,3};DrawSystem wrapped;Characters three{'A','B','C'};
    plain.drawText(&wrapped,&three,1,&wrap,&clip,nullptr);
    check(wrapped.draws.size()==2 && wrapped.draws[1].x==11 && wrapped.draws[1].y==42,
          "exact right edge wraps next glyph to original x");
    check(three.indices==std::vector<int>{0,1},"exact bottom edge stops without fetching next character");
    DrawSystem empty;Characters nothing{};events.clear();
    plain.drawText(&empty,&nothing,5,&bounds,&clip,nullptr);
    check(empty.draws.empty() && events==std::vector<std::string>{"height","prepare","data"},
          "empty text still reads height and prepares font before termination");
}
}
int main()
{
    chain();descriptors();textDrawing();
    std::printf("effect_style_runtime: %d checks, %d failures\n",checks,failures);
    return failures ? 1:0;
}
