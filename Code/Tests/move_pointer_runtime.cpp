// [TEMP] [CHOSEN, not recovered] Native x86 MovePointer operational fixture.
// A storage-only constructor bypasses the unpublished MovePointer factory.
// TransformNode construction/metadata and recovered operations are production
// bodies; HUD enablePointer remains outside this fixture.
// [RETIRE-WHEN: MOVE-POINTER-STORAGE-FIXTURE]
#include "../BF2/External/BFMeme/MovePointer.h"
#include "../BF2/External/BFMeme/TransformNode.h"
#include "../BF2/External/BFMeme/ColorEffect.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <float.h>
#include <limits>
#include <string>
#include <vector>

namespace dice { namespace meme {
// Only test storage: no descriptor, publication, selection or registration.
MovePointer::MovePointer() : m_selected(nullptr), m_dragging(false),
    m_relativeX(0), m_relativeY(0), m_startX(0), m_startY(0),
    m_startPositionX(0), m_startPositionY(0), m_startWidth(0), m_startHeight(0),
    m_resizing(false) {}
} }

using namespace dice::meme;
namespace {
int checks=0, failures=0;
void check(bool value,const char* label)
{
    ++checks;
    if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",label);}
}
struct PointerFixture : MovePointer
{
    void select(TransformNode* selected,bool dragging,bool resizing=false)
    { m_selected=selected;m_dragging=dragging;m_resizing=resizing; }
    void start(float x,float y,float px,float py,float width,float height)
    {m_startX=x;m_startY=y;m_startPositionX=px;m_startPositionY=py;m_startWidth=width;m_startHeight=height;}
    void graphic(Node* node){m_graphic=node;}
    TransformNode* selected(){return m_selected;}
    bool dragging(){return m_dragging;}
    bool resizing(){return m_resizing;}
    float startX(){return m_startX;}
    float startY(){return m_startY;}
    float relativeX(){return m_relativeX;}
    float relativeY(){return m_relativeY;}
};
struct DrawSystem : System
{
    DrawSystem():System(800,600){}
    struct Draw { int texture;float x,y,width,height;Color color;const Rect* clip; };
    std::vector<Draw> draws;
    bool result=false;
    int moves=0, character=0, font=0, characterCalls=0;
    PointerFixture* pointer=nullptr;
    TransformNode* replaceSelection=nullptr;
    Rect* mutateBounds=nullptr;
    std::vector<std::string> order;
    void position(float x,float y){m_pointerX=x;m_pointerY=y;}
    void root(Node* node){m_field018=node;}
    void state(float target){m_field028=target;}
    bool pointerPositionChanged(float* x,float* y) override
    {
        ++moves;order.push_back("position");
        if(pointer&&replaceSelection)pointer->select(replaceSelection,true);
        *x=-1000;*y=-2000;return false;
    }
    bool drawTexture(int texture,Quad* quad) override
    {
        const auto& p=quad->corners[0];
        draws.push_back({texture,p.x,p.y,quad->corners[1].x-p.x,
            quad->corners[3].y-p.y,p.color,quad->clip});
        order.push_back("texture");
        if(mutateBounds&&draws.size()==1)*mutateBounds=Rect{30,40,50,60,0};
        return result;
    }
    bool drawCharacter(int c,int f,Quad*) override
    {character=c;font=f;++characterCalls;return result;}
};
struct Selected : TransformNode
{
    bool named=true;
    int hasNameCalls=0,nameCalls=0,visits=0;
    PointerFixture* pointer=nullptr;
    Selected* replacement=nullptr;
    Rect bounds{10,20,30,40,0};
    bool slot_0x24() override
    {++hasNameCalls;if(pointer&&replacement)pointer->select(replacement,false);return named;}
    const char* getName() override{++nameCalls;return "fixture transform";}
    void visitChildren(System* system,NodeState*,const Rect*,NodeVisitor* visitor) override
    {
        ++visits;
        Node* child=nullptr;
        visitor->traverse(system,child,&bounds,nullptr); // highlight arm must not dereference state/node
        visitor->traverse(system,child,&bounds,nullptr); // next flag must already be cleared
    }
};
struct Graphic : Node
{
    int calls=0;
    void paint(System* s,const NodeState*,const Rect*,const Rect*,RenderContext*) override
    {++calls;static_cast<DrawSystem*>(s)->order.push_back("graphic");}
};
struct ColorStream : Stream
{
    int rejectAt=0;
    std::vector<std::string> labels;
    std::vector<float*> pointers;
    bool streamFloat(float* value,const char* label) override
    {labels.push_back(label);pointers.push_back(value);*value+=10;return static_cast<int>(labels.size())!=rejectAt;}
};
void colorEffect()
{
    DrawSystem system;
    ColorEffect color(2,3,4,5);
    Effect* typed=&color;
    check(sizeof(ColorEffect)==20 && sizeof(MovePointer)==64 && sizeof(TransformNode)==36,
          "native x86 ColorEffect/MovePointer/TransformNode storage");
    Color* actual=typed->getColor();
    check(actual==typed->getColor() && actual->r==2 && actual->a==5,"getColor returns actual stable storage");
    check(!typed->setColor(20,30,40,50) && actual->r==2 && actual->a==5,
          "ColorEffect retains inherited false setColor hook");
    Rect bounds{1,2,3,4,5}; Quad quad(&bounds,nullptr);
    RenderContext context{nullptr,&color,0,1,0,1,1};
    check(!typed->drawTexture(&system,-72,&quad,&context) && system.draws.size()==1 && system.draws[0].texture==-72,
          "color texture forwards signed handle and false renderer result");
    for(const auto& corner:quad.corners)
        check(corner.color.r==2 && corner.color.g==3 && corner.color.b==4 && corner.color.a==5,
              "all four RGBA components overwrite every texture corner");
    actual->r=-11;actual->a=.25f;system.result=true;
    check(typed->drawCharacter(&system,0x10041,-91,-15,&quad,&context) && system.character==0x10041 &&
          system.font==-91 && system.characterCalls==1,"character preserves full-width payload and true result");
    for(const auto& corner:quad.corners)
        check(corner.color.r==-11 && corner.color.a==.25f,"mutable color storage reaches character corners");
    for(int rejection=0;rejection<=4;++rejection)
    {
        ColorEffect value(1,2,3,4);ColorStream stream;stream.rejectAt=rejection;
        check(value.streamProperties(&system,&stream)==(rejection==0),"color properties propagate first failure");
        const std::vector<std::string> expected{"Red","Green","Blue","Alpha"};
        const int count=rejection ? rejection:4;
        check(stream.labels==std::vector<std::string>(expected.begin(),expected.begin()+count),
              "color properties preserve ordered short-circuit labels");
        for(int i=0;i<count;++i)
            check(stream.pointers[i]==&value.getColor()->r+i && *stream.pointers[i]==float(i+11),
                  "visitor receives each actual component and retains mutation");
    }
    auto* type=ColorEffect::getClassType();
    check(type==TypeDescriptor::find("dice::meme::ColorEffect") && type->getBaseType()==Effect::getClassType(),
          "complete ColorEffect descriptor registration and actual Effect base");
    check(type->getInstanceSize()==20 && type->getDescription()==nullptr,"descriptor native size and description");
    const int count=type->getInstanceCount();RuntimeObject* made=nullptr;
    check(createRuntimeObject(made,type) && made->getType()==type && made->getObjectSize()==20 &&
          type->getInstanceCount()==count+1,"real ColorEffect factory and instance accounting");
    auto* madeColor=static_cast<ColorEffect*>(made)->getColor();
    check(madeColor->r==1 && madeColor->g==1 && madeColor->b==1 && madeColor->a==1,"factory default opaque white");
    check(destroyRuntimeObject(made) && !made && type->getInstanceCount()==count,"real descriptor teardown accounting");
}
void movement()
{
    DrawSystem system;PointerFixture pointer;Selected first,second;
    pointer.select(&first,true);pointer.start(10,20,100,200,30,40);
    check(pointer.setPosition(&system,15,27) && first.getPosition().x==105 && first.getPosition().y==207,
          "direct disabled MovePointer still applies drag delta to selected transform");
    check(system.moves==0 && system.getPointerPosition()[0]==0,"disabled base leaves pointer coordinates and callback alone");
    pointer.setEnabled(true);system.pointer=&pointer;system.replaceSelection=&second;
    check(pointer.setPosition(&system,18,32) && first.getPosition().x==105 && second.getPosition().x==108 &&
          second.getPosition().y==212,"selection reloaded after real base pointer-position callback");
    check(system.moves==1 && system.getPointerPosition()[0]==18 && system.getPointerPosition()[1]==32,
          "base stores supplied coordinates before callback and ignores changed callback locals/false return");
    system.replaceSelection=nullptr;pointer.select(&first,true,true);pointer.start(10,20,100,200,30,40);
    check(pointer.setPosition(&system,18,32) && first.getSize().x==38 && first.getSize().y==52,
          "resize changes native size without changing position");
    check(first.getPosition().x==105 && first.getPosition().y==207,"resize preserves native transform position");
    check(pointer.setPosition(&system,-100,-100) && first.getSize().x==2 && first.getSize().y==2,
          "resize clamps both negative dimensions to two");
    const float nan=std::numeric_limits<float>::quiet_NaN();
    check(pointer.setPosition(&system,nan,nan) && std::isnan(first.getSize().x) && std::isnan(first.getSize().y),
          "unordered resize values retain NaN instead of clamping");
    pointer.select(nullptr,true);
    check(pointer.setPosition(&system,7,8) && system.moves==5,"null selection still executes base then returns true");
    pointer.select(&first,false);system.root(nullptr);
    check(pointer.setPosition(&system,9,10) && !pointer.selected(),"hover selection clears stale target for empty root");
    system.root(&first);system.state(nan);pointer.select(&first,false);
    check(pointer.setPosition(&system,9,10) && !pointer.selected() && first.visits==0,
          "unordered traversal state skips transform descriptor and children");
    system.state(1);pointer.select(&first,false);
    check(pointer.setPosition(&system,9,10) && !pointer.selected() && first.visits==0,
          "positive traversal state skips selection and children");
    system.root(nullptr);system.position(12,34);pointer.setEnabled(false);pointer.select(&first,false);
    check(pointer.event1(&system,4,-99,77) && system.getLockCount()==1 && pointer.dragging() && pointer.resizing(),
          "disabled direct event1 locks and starts resize without an enabled guard");
    check(!pointer.selected() && pointer.startX()==12 && pointer.startY()==34 && pointer.relativeX()==12 && pointer.relativeY()==34,
          "empty selection records pointer origin and zero-offset relative position");
    check(pointer.event1(&system,3,0,0) && system.getLockCount()==2 && !pointer.resizing(),
          "nested event1 increments lock and only first argument four enables resizing");
    check(pointer.event2(&system,0,0) && system.getLockCount()==1 && !pointer.dragging(),"event2 unlocks and clears drag");
    pointer.event2(&system,0,0);pointer.event2(&system,0,0);
    check(system.getLockCount()==-1,"event2 preserves missing lock-underflow guard");
    pointer.select(&first,true);first.named=false;
    check(pointer.event2(&system,0,0) && first.hasNameCalls==1 && first.nameCalls==0,
          "unnamed selection queries capability but does not request name");
    pointer.select(&first,true);first.named=true;first.pointer=&pointer;first.replacement=&second;
    check(pointer.event2(&system,0,0) && first.hasNameCalls==2 && first.nameCalls==0 && second.nameCalls==1,
          "release reloads selected transform between name capability and name call");
    const unsigned int savedControl=_controlfp(0,0);
    _controlfp(_PC_64,_MCW_PC);
    const float huge=std::ldexp(1.0f,60);
    pointer.select(&first,true);pointer.start(1,0,-huge,0,0,0);
    check(pointer.setPosition(&system,huge,0) && first.getPosition().x==-1,
          "extended x87 drag preserves live cancellation bits beyond double precision");
    _controlfp(savedControl,_MCW_PC);
}
void highlighting()
{
    DrawSystem system;PointerFixture pointer;Selected selected;Graphic graphic;
    system.root(&selected);pointer.select(&selected,false);pointer.graphic(&graphic);pointer.setEnabled(true);
    check(pointer.paint(&system) && system.draws.size()==5 && selected.visits==1 && graphic.calls==1,
          "real base graphic then four outlines and fill; renderer false results are ignored");
    check(system.order==std::vector<std::string>{"graphic","texture","texture","texture","texture","texture"},
          "base graphic draw precedes highlight traversal");
    const float expected[4][4]={{9,19,32,1},{9,60,32,1},{9,20,1,40},{40,20,1,40}};
    for(int i=0;i<4;++i)
    {
        const auto& draw=system.draws[i];
        check(draw.x==expected[i][0] && draw.y==expected[i][1] && draw.width==expected[i][2] &&
              draw.height==expected[i][3] && draw.clip==nullptr && draw.color.r==1 && draw.color.a==1,
              "outline bounds and opaque white un-clipped Quad");
    }
    const auto& fill=system.draws[4];
    check(fill.x==10 && fill.y==20 && fill.width==30 && fill.height==40 && fill.clip==&selected.bounds &&
          fill.color.r==.1f && fill.color.g==.1f && fill.color.b==.1f && fill.color.a==.1f,
          "fill uses actual ColorEffect and child bounds as clip");
    system.draws.clear();system.order.clear();system.mutateBounds=&selected.bounds;
    check(pointer.paint(&system) && system.draws.size()==5,"second highlight traverses after prior next flag was cleared");
    check(system.draws[0].x==9 && system.draws[1].x==29 && system.draws[1].y==100 &&
          system.draws[2].x==29 && system.draws[3].x==80 && system.draws[4].x==30,
          "each successive outline/fill reloads bounds after renderer callback mutation");
    system.draws.clear();pointer.select(&selected,true);
    check(pointer.paint(&system) && system.draws.empty() && graphic.calls==3,"dragging suppresses highlight but retains base graphic");
    pointer.select(&selected,false);pointer.setEnabled(false);system.mutateBounds=nullptr;
    check(pointer.paint(&system) && system.draws.size()==5 && graphic.calls==3,
          "direct disabled paint still highlights while base graphic is disabled");
    system.draws.clear();system.state(1);
    check(pointer.paint(&system) && system.draws.empty(),"positive root state prevents highlight traversal");
    system.state(-3);selected.bounds.height=std::numeric_limits<float>::infinity();
    check(pointer.paint(&system) && std::isnan(system.draws[0].height),
          "one-pixel outline preserves retail infinity subtraction instead of constant-folding height");
    system.draws.clear();selected.bounds=Rect{0,0,10,std::ldexp(1.0f,60),0};
    const unsigned int savedControl=_controlfp(0,0);
    _controlfp(_PC_64,_MCW_PC);
    check(pointer.paint(&system) && system.draws[0].height==1,
          "extended x87 outline keeps one-pixel thickness through large finite cancellation");
    _controlfp(savedControl,_MCW_PC);
}
}
int main()
{
    colorEffect();movement();highlighting();
    std::printf("move_pointer_runtime: %d checks, %d failures\n",checks,failures);
    return failures ? 1:0;
}
