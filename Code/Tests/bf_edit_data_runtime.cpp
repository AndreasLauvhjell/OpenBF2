// [TEMP] Native x86 component fixture. Actual BfEditData factory/lifetime and
// bounded BfEditNode bodies; callbacks record only tested interactions. The
// incomplete edit-node paint/input/descriptor methods are never called.
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <new>
#include <cstdlib>
#define private public
#define protected public
#include "../BF2/Menu/BfEditData.h"
#include "../BF2/Menu/BfEditNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
using namespace dice::meme;
static int checks, failures, nodeDestructions;
static bool watchFourByteAllocation;
static void* watchedBuffer;
static bool watchedBufferDeleted;
void* operator new(std::size_t bytes)
{
    void* result=std::malloc(bytes?bytes:1);
    if (!result) throw std::bad_alloc();
    if (watchFourByteAllocation && bytes==4) {
        watchedBuffer=result; watchedBufferDeleted=false; watchFourByteAllocation=false;
    }
    return result;
}
void operator delete(void* value) noexcept
{ if(value && value==watchedBuffer)watchedBufferDeleted=true; std::free(value); }
void operator delete(void* value, std::size_t) noexcept { ::operator delete(value); }
static void check(bool value, const char* label)
{ ++checks; if (!value) { ++failures; std::printf("FAIL %s\n", label); } }
struct InputCall { int type, value, index, second; System* system; const Rect* rect; const NodeState* state; };
struct ObservedNode : BfEditNode
{
    std::vector<InputCall> calls;
    ~ObservedNode() override { ++nodeDestructions; }
    bool onEvent1(System* s, const Rect* r, const NodeState* n, int v, int i, int b) override
    { calls.push_back({1,v,i,b,s,r,n}); m_pendingInput=71; return false; }
    bool onEvent4(System* s, const Rect* r, const NodeState* n, int v, int i, int b) override
    { calls.push_back({4,v,i,b,s,r,n}); m_pendingInput=72; return false; }
};
struct CharacterData : Data
{
    std::vector<int> characters, indices;
    System* expectedSystem = nullptr;
    BfEditData* edit = nullptr;
    BfEditNode* originalNode = nullptr;
    BfEditNode* replacementNode = nullptr;
    Data* replacementString = nullptr;
    int setCalls = 0;
    int getCharacter(System* system, int index) override
    {
        check(system == expectedSystem, "character callback preserves System");
        indices.push_back(index);
        if (index == 0 && edit) {
            edit->m_activeNode = replacementNode;
            originalNode->m_string = replacementString;
        }
        return index >= 0 && static_cast<unsigned>(index) < characters.size() ? characters[index] : 0;
    }
    bool setString(System* system, const char* text) override
    { ++setCalls; check(system == expectedSystem && text[0] == 0, "clear forwards empty string to reloaded data"); return false; }
};
struct MetricSystem : System
{
    MetricSystem() : System(800,600) {}
    std::vector<int> characters, fonts;
    GlyphMetrics metric{0,2,0,9,-3};
    BfEditNode* mutateNode = nullptr;
    bool getGlyphMetrics(int c, int f, GlyphMetrics* out) override
    { characters.push_back(c); fonts.push_back(f); *out=metric; if(mutateNode)mutateNode->m_font=22; return false; }
};
struct PropertyStream : Stream
{
    std::vector<std::string> labels;
    int failAt = 0;
    BfEditData* edit;
    Data* text;
    BfEditNode* node;
    bool sawRegistered = false;
    bool step(const char* label) { labels.push_back(label); return static_cast<int>(labels.size()) != failAt; }
    bool streamNode0x88(System*, Node*& value, const char* label, TypeDescriptor* type) override
    { check(&value == &node->m_next && !type, "Next node uses actual link and default requested type"); return step(label); }
    bool streamFont(System*, int* value, const char* label) override
    { check(value==&node->m_font, "font streams live field"); *value=99; return step(label); }
    bool streamData(System*, Data*& value, const char* label, TypeDescriptor* type) override
    {
        if (std::strcmp(label,"String")==0) {
            check(&value==&node->m_string && !type, "String uses live Data field"); value=text;
        } else {
            check(&value==reinterpret_cast<Data**>(&node->m_editData) && type==BfEditData::getClassType(),
                "Editbox uses live field and real complete BfEditData descriptor"); value=edit;
        }
        return step(label);
    }
    bool streamInt(int* value, const char* label) override
    { check(value==&node->m_maxCharacters, "maximum streams signed int field"); *value=12; return step(label); }
    bool streamAction(System*, Action*& value, const char* label, TypeDescriptor* type) override
    { check(&value==&node->m_selectAction && !type, "selection uses Action field"); return step(label); }
    bool streamBool(bool* value, const char* label) override
    { sawRegistered=edit->m_activeNode==node && !edit->m_nodes.empty() && edit->m_nodes.back()==node;
      check(value==&node->m_focused, "focus streams live bool"); *value=true; return step(label); }
};
int main()
{
    static_assert(sizeof(void*)==4, "x86 only");
    static_assert(sizeof(BfEditNode)==80, "non-STL edit node layout");
    MetricSystem system;
    Rect rect{0,0,6,20,0}; NodeState state{1,1};
    ObservedNode first, second;
    check(first.m_editData==nullptr && first.m_string==nullptr && first.m_selectAction==nullptr,
          "node constructor leaves three borrowed resources null");
    check(first.m_font==0 && first.m_maxCharacters==-1 && first.m_firstVisible==0 && first.m_cursor==0,
          "node constructor establishes font and cursor defaults");
    check(first.m_blinkTime==-0.4f && !first.m_focused && first.m_pendingInput==0 && !first.m_outline,
          "node constructor establishes blink and input flags");
    check(first.m_color.r==0 && first.m_color.g==0 && first.m_color.b==1 && first.m_color.a==1,
          "node constructor preserves blue opaque color");
    TypeDescriptor* type=BfEditData::getClassType();
    check(std::strcmp(type->getName(),"dice::meme::BfEditData")==0 && type==TypeDescriptor::find("BfEditData"),
          "real descriptor is discoverable by short name");
    check(type->getBaseType()==Data::getClassType() && type->isDerivedFrom(Object::getClassType()),
          "descriptor preserves Data/Object ancestry");
    check(type->getDescription()==nullptr && type->getInstanceSize()==sizeof(BfEditData),
          "descriptor null description and native size");
    RuntimeObject* object=nullptr;
    const int previousCount=type->getInstanceCount();
    check(createRuntimeObject(object,type), "actual BfEditData descriptor factory succeeds");
    BfEditData* edit=static_cast<BfEditData*>(object);
    check(edit->getType()==type && edit->getObjectSize()==sizeof(*edit) && edit->getName()==nullptr,
          "factory creates correct real family with null name");
    check(edit->m_nodes.empty() && !edit->m_activeNode && !edit->m_focusLocked && !edit->m_hideText,
          "factory initializes actual native list and flags");
    check(type->getInstanceCount()==previousCount+1, "factory helper counts actual instance");
    edit->registerNode(&first); edit->registerNode(&second); edit->registerNode(&first);
    check(std::vector<BfEditNode*>(edit->m_nodes.begin(),edit->m_nodes.end())==
          std::vector<BfEditNode*>({&first,&second,&first}), "node list preserves order and duplicates");
    check(edit->m_activeNode==nullptr, "registerNode does not invent active-node selection");
    const int destroyedBefore=nodeDestructions;
    check(destroyRuntimeObject(object) && !object && type->getInstanceCount()==previousCount,
          "real descriptor lifecycle destroys and decrements");
    check(nodeDestructions==destroyedBefore, "BfEditData destruction does not own borrowed nodes");
    BfEditData data("edit");
    check(std::strcmp(data.getName(),"edit")==0 && data.getElement(0)==&data && data.getElement(-1)==nullptr,
          "inherited name/scalar element behavior survives subclass table");
    check(data.getCharacter(&system,0)==0 && data.getFloat(&system)==0 && data.getElementCount(&system)==1,
          "data family retains actual scalar defaults");
    data.queueKey(18); data.queueCharacter(0x10041); data.clear(nullptr); data.setFocus(true);
    check(!first.m_focused && first.m_pendingInput==0, "null active-node helpers are guarded");
    data.m_activeNode=&first;
    data.queueKey(-128);
    check(first.m_pendingInput==1 && first.m_pendingKey==-128, "queued key retains signed int");
    data.queueCharacter(0x10041);
    check(first.m_pendingInput==2 && first.m_pendingCharacter==0x10041, "queued character retains full word");
    data.setFocus(true); data.m_focusLocked=true; data.setFocus(false);
    check(first.m_focused, "focus lock prevents active-node mutation");
    data.m_focusLocked=false; data.setFocus(false);
    check(!first.m_focused, "unlocked focus updates node without System lock operation");
    CharacterData original, replacement;
    original.expectedSystem=replacement.expectedSystem=&system;
    original.characters={-128,0x10000,0x10041,0};
    original.edit=&data; original.originalNode=&first; original.replacementNode=&second;
    original.replacementString=&replacement;
    first.m_string=&original; first.m_cursor=8; second.m_cursor=9;
    data.clear(&system);
    check(original.indices==std::vector<int>({0,1,2,3}), "length tests full int values, not low byte/word");
    check(original.setCalls==0 && replacement.setCalls==1, "clear reloads retained node string after length callbacks");
    check(data.m_activeNode==&second && first.m_cursor==0 && second.m_cursor==9,
          "clear retains original active node across reentrant active-node replacement");

    first.m_pendingInput=1; first.m_pendingKey=18;
    check(first.updateSelf(&system,&state,&rect,0.1f), "real update ignores event observer's false result");
    check(first.calls.size()==1 && first.calls[0].type==1 && first.calls[0].value==18 &&
          first.calls[0].index==1 && first.calls[0].second==1 && first.calls[0].system==&system &&
          first.calls[0].rect==&rect && first.calls[0].state==&state, "pending key uses actual virtual slot and argument order");
    check(first.m_pendingInput==0, "pending clear follows callback mutation");
    first.m_pendingInput=2; first.m_pendingCharacter=-128;
    first.updateSelf(&system,&state,&rect,0);
    check(first.calls.size()==2 && first.calls[1].type==4 && first.calls[1].value==-128 && first.m_pendingInput==0,
          "character pending dispatch uses +80 with signed value");
    first.m_pendingInput=37; first.updateSelf(&system,&state,&rect,0);
    check(first.m_pendingInput==37 && first.calls.size()==2, "unknown pending code retained without callback");
    first.m_slowBlink=false; first.m_blinkTime=0.3f; first.updateSelf(&system,&state,&rect,0.1f);
    check(first.m_blinkTime==-0.4f, "ordinary blink wraps at original threshold");
    first.m_slowBlink=true; first.m_blinkTime=0.7f; first.updateSelf(&system,&state,&rect,0.11f);
    check(first.m_blinkTime==-0.8f, "slow blink uses independent threshold");
    first.m_slowBlink=false; first.m_blinkTime=0.4f; first.updateSelf(&system,&state,&rect,-1.0e-9f);
    check(first.m_blinkTime==0.4f, "blink compares unrounded sum before stored float threshold");

    PropertyStream visitor; visitor.edit=&data; visitor.text=&replacement; visitor.node=&first;
    visitor.failAt=7;
    check(!first.streamProperties(&system,&visitor), "focus visitor failure propagated");
    check(visitor.labels==std::vector<std::string>({"Next node","Font","String",
          "Max characters(-1 = no limit)","Select action","Editbox data","Focus"}),
          "properties preserve full stream order and labels");
    check(visitor.sawRegistered && data.m_activeNode==&first && first.m_focused,
          "node registration and visitor writes remain on final focus failure");
    const std::size_t registrations=data.m_nodes.size();
    visitor.labels.clear(); visitor.failAt=0;
    check(first.streamProperties(&system,&visitor) && data.m_nodes.size()==registrations+1,
          "repeated serialization appends duplicate node");
    visitor.labels.clear(); visitor.failAt=6; data.m_activeNode=&second;
    check(!first.streamProperties(&system,&visitor) && visitor.labels.size()==6 &&
          data.m_nodes.size()==registrations+1 && data.m_activeNode==&second && first.m_editData==&data,
          "failed edit-data visitor keeps pointer mutation but skips registration/focus");
    visitor.labels.clear(); visitor.failAt=2;
    check(!first.streamProperties(&system,&visitor) && visitor.labels.size()==2,
          "early property failure suppresses later visitors");

    CharacterData characters; characters.expectedSystem=&system;
    characters.characters={0x10041,-128,66,67,0};
    first.m_string=&characters; first.m_cursor=4; first.m_firstVisible=0; first.m_font=11;
    system.mutateNode=&first;
    first.updateVisibleStart(&system,&rect);
    check(first.m_firstVisible==2 && characters.indices==std::vector<int>({3,2,1}),
          "scroll measures backward and advances start on exact zero remaining width");
    check(system.characters==std::vector<int>({67,66,-128}) && system.fonts==std::vector<int>({11,22,22}),
          "scroll forwards full signed characters and reloads font per iteration");
    system.mutateNode=nullptr; characters.indices.clear(); system.characters.clear();
    first.m_cursor=2; first.m_firstVisible=3;
    first.updateVisibleStart(nullptr,nullptr);
    check(first.m_firstVisible==2 && characters.indices.empty(), "cursor before start bypasses bounds and callbacks");
    first.m_string=nullptr; first.m_firstVisible=7; first.updateVisibleStart(nullptr,nullptr);
    check(first.m_firstVisible==7, "null string bypasses all scrolling inputs");
    first.m_string=&characters; first.m_cursor=1; first.m_firstVisible=0;
    system.metric={0,2,0,9,0}; rect.width=0;
    first.updateVisibleStart(&system,&rect);
    check(first.m_firstVisible==1 && system.characters.back()==0x10041,
          "zero width still measures one full-width character before advancing");
    characters.indices.clear(); first.m_cursor=2; first.m_firstVisible=0; rect.width=1;
    system.metric={-std::ldexp(1.0f,80),1,std::ldexp(1.0f,80),0,0};
    first.updateVisibleStart(&system,&rect);
    check(first.m_firstVisible==0 && characters.indices==std::vector<int>({1,0}),
          "scroll bearing-width addition order retains cancellation witness");
    StringData text("ac");
    first.m_string=&text; first.m_focused=true; first.m_cursor=1; first.m_firstVisible=0;
    first.m_limitWidth=false; first.m_slowBlink=false; first.m_skipCharacter=false;
    first.m_maxCharacters=-1; system.metric={0,2,0,9,0}; rect.width=100;
    check(first.BfEditNode::onEvent4(&system,&rect,&state,'b',77,99),
          "actual character handler ignores final index/second operands");
    char buffer[32]; text.getString(&system,buffer,sizeof(buffer));
    check(std::strcmp(buffer,"abc")==0 && first.m_cursor==2, "real StringData insertion preserves tail and advances cursor");
    const bool skip=first.m_skipCharacter;
    check(!first.BfEditNode::onEvent4(nullptr,nullptr,nullptr,13,0,0) &&
          !first.BfEditNode::onEvent4(nullptr,nullptr,nullptr,27,0,0) &&
          !first.BfEditNode::onEvent4(nullptr,nullptr,nullptr,22,0,0) &&
          !first.BfEditNode::onEvent4(nullptr,nullptr,nullptr,257,0,0) && first.m_skipCharacter==skip,
          "excluded control/full-width characters precede toggle and callbacks");
    first.m_slowBlink=true; first.m_skipCharacter=true;
    check(!first.BfEditNode::onEvent4(nullptr,nullptr,nullptr,'x',0,0) && !first.m_skipCharacter,
          "alternate-character gate clears then rejects without string access");
    first.m_slowBlink=false; first.m_maxCharacters=3;
    check(!first.BfEditNode::onEvent4(&system,nullptr,nullptr,'x',0,0) && first.m_skipCharacter,
          "length rejection follows the skip-flag toggle");
    first.m_maxCharacters=0; first.m_cursor=0;
    check(first.BfEditNode::onEvent4(&system,&rect,nullptr,-128,0,0), "zero maximum is unlimited and negative character accepted");
    text.getString(&system,buffer,sizeof(buffer));
    check(static_cast<unsigned char>(buffer[0])==128 && std::strcmp(buffer+1,"abc")==0,
          "negative character is narrowed only when stored");
    first.m_cursor=1;
    check(first.BfEditNode::onEvent4(&system,&rect,nullptr,256,0,0), "character256 is accepted and narrowed to NUL");
    check(text.getStringLength(&system)==1 && first.m_cursor==2,
          "inserted NUL truncates text yet any length change advances cursor");
    text.setString(&system,"ab"); first.m_cursor=1; first.m_firstVisible=0;
    first.m_limitWidth=true; rect.width=5; system.characters.clear(); system.fonts.clear();
    watchedBuffer=nullptr; watchFourByteAllocation=true;
    check(!first.BfEditNode::onEvent4(&system,&rect,nullptr,-128,0,0),
          "width-limited insertion rejects when total width reaches boundary");
    text.getString(&system,buffer,sizeof(buffer));
    check(std::strcmp(buffer,"ab")==0 && first.m_cursor==1 &&
          system.characters==std::vector<int>({'a',-128,'b'}),
          "width rejection preserves original text/cursor and measures signed inserted bytes");
    check(watchedBuffer && !watchedBufferDeleted, "width rejection preserves observed retail allocation leak");
    // Fixture-only cleanup after proving the production early-return leak.
    void* leaked=watchedBuffer; watchedBuffer=nullptr; ::operator delete(leaked);
    rect.width=7; system.characters.clear();
    check(first.BfEditNode::onEvent4(&system,&rect,nullptr,'c',0,0), "width-limited insertion succeeds with positive remaining space");
    text.getString(&system,buffer,sizeof(buffer));
    check(std::strcmp(buffer,"acb")==0 && first.m_cursor==2, "successful width-limited insertion reaches real setter and scrolling");
    std::printf("BfEditData/Node: %d/%d checks passed; diagnostic x86 component\n",checks-failures,checks);
    return failures?1:0;
}
