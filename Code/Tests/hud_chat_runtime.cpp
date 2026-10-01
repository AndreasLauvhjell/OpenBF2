// [TEMP] Diagnostic x86 fixture. No production body is replaced. Partially
// recovered vtables are used only on the explicitly measured paths below.
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <limits>
#include "../BF2/Core/IBase.h"
#include "../BF2/External/BFMeme/Quad.h"
#define private public
#define protected public
#include "../BF2/Menu/HudChat.h"
#include "../BF2/Menu/BfListBoxData.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/TypeDescriptor.h"

namespace dice { namespace hfe { namespace hud { class BF2HudManager; }
hud::BF2HudManager* hudManager = nullptr; // [TEMP] uncalled formatter dependency
} }
static int checks = 0, failures = 0;
static void check(bool value, const char* label)
{ ++checks; if (!value) { ++failures; std::printf("FAIL %s\n", label); } }
static void* watched[128];
static bool freed[128];
static int watchCount;
static int watch(void* p) { const int i = watchCount++; watched[i] = p; freed[i] = false; return i; }
void* operator new(std::size_t n) { void* p = std::malloc(n ? n : 1); if (!p) throw std::bad_alloc(); return p; }
void operator delete(void* p) noexcept
{ for (int i = 0; i < watchCount; ++i) if (watched[i] == p) freed[i] = true; std::free(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
static bool untouched(const void* p, std::size_t n)
{ const unsigned char* b = static_cast<const unsigned char*>(p); for (std::size_t i=0;i<n;++i) if(b[i]!=0xCD) return false; return true; }
using namespace dice::meme;
using dice::hfe::hud::HudChat;
static bool white(const Color& c) { return c.r==1 && c.g==1 && c.b==1 && c.a==1; }
static void freeLeakedColumns(const std::vector<ListBoxColumn*>& columns)
{ for (ListBoxColumn* c : columns) if(c) { for(ListBoxCell* cell:c->cells) delete cell; delete c; } }

struct ResourceObserver : dice::hfe::IBase
{
    BfListBoxData* data;
    int calls = 0;
    void addRef() override { check(false,"unexpected addRef"); }
    int getRef() override { check(false,"unexpected getRef"); return 1; }
    int release() override {
        ++calls;
        check(data->m_columns.empty(),"vector released before resource callback");
        check(std::strcmp(data->getName(),"ingameChat")==0,"Data name lives through resource callback");
        return 0;
    }
    void* queryInterface(unsigned int) const override { return nullptr; }
};
struct TimerObserver : HudChat
{
    TimerObserver() : HudChat(nullptr), drops(0), expected(nullptr), mutateOther(false) {}
    int drops;
    int* expected;
    bool mutateOther;
    void dropRow(int first, int* count) override {
        ++drops;
        check(count == expected,"timer forwards real count address");
        check(first==12,"timer forwards first row");
        --*count;
        m_remaining[0] = 99; // Retail writes zero after callback.
        if (mutateOther) { m_count[1]=1; m_remaining[1]=4; }
    }
};

int main()
{
    static_assert(sizeof(void*)==4,"x86 fixture");
    static_assert(sizeof(ListBoxData)==16,"32-slot base native layout");
    static_assert(sizeof(BfListBoxData)==100,"native vector reduces retail104 by4");
    static_assert(sizeof(ListBoxCell)==140,"native strings reduce retail148 by8");
    static_assert(sizeof(ListBoxColumn)==32,"native vector reduces retail36 by4");
    static_assert(sizeof(HudChat)==972,"native list reduces retail976 by4");

    alignas(HudChat) unsigned char storage[sizeof(HudChat)];
    std::memset(storage,0xCD,sizeof(storage));
    System* identity = reinterpret_cast<System*>(0x12340000); // [TEMP] never dereferenced
    HudChat* chat = new(storage) HudChat(identity);
    check(chat->m_system==identity && chat->getData()==nullptr,"constructor binds root and clears list");
    for(const Color& c:chat->m_field008) check(white(c),"seven constructor colors");
    for(const auto& v:chat->m_colors) for(const Color& c:v) check(white(c),"36 constructor colors");
    for(int i=0;i<9;++i) check(chat->getCapacity(i)==0 && chat->m_spacing[i]==0 && chat->m_duration[i]==5,"nine defaults");
    check(untouched(chat->m_field07c,sizeof(chat->m_field07c)),"unknown words untouched");
    check(untouched(chat->m_firstRow,sizeof(chat->m_firstRow)),"initial first rows untouched");
    check(untouched(chat->m_count,sizeof(chat->m_count)),"initial counts untouched");
    check(untouched(chat->m_remaining,sizeof(chat->m_remaining)),"initial timers untouched");
    check(untouched(&chat->m_field3b8,1) && untouched(&chat->m_field3c8,1) && untouched(&chat->m_field3c9,1),"unwritten flags untouched");
    check(chat->m_field3bc.empty() && chat->m_historyOutput,"list and history flag");
    check(chat->init("ingameChat",16,500),"real chat init returns true");
    BfListBoxData* data=chat->getData();
    check(std::strcmp(data->getName(),"ingameChat")==0,"Data name clone");
    check(data->m_columns.size()==3 && data->m_columnCount==3,"default column plus two init columns");
    check(data->m_columns[0]->width==0 && data->m_columns[1]->width==16 && data->m_columns[2]->width==500,"column widths preserve call order");
    check(data->m_columns[0]->field019 && !data->m_columns[1]->field019 && data->m_columns[2]->field019,"column flags preserve call order");
    check(!data->m_field021 && data->m_field03c==6 && !data->m_field040,"chat init list flags");
    check(data->m_visibleRows==0 && data->getRowCount()==0,"initial rebuild uses zero total");
    check(data->m_row==-1,"Bf selected row overrides base zero");
    check(untouched(chat->m_firstRow+3,4*sizeof(int)),"rebuild preserves first rows3..6");
    for(int i=0;i<9;++i) check(*chat->getCount(i)==0 && chat->m_remaining[i]==0,"rebuild clears counters and timers");
    chat->setCapacity(0,4); chat->setCapacity(1,5); chat->setCapacity(7,3); chat->setCapacity(8,2);
    chat->setSpacing(0,4); chat->setSpacing(7,12); chat->setSpacing(8,9);
    chat->rebuildRows();
    check(chat->getRowsUsed()==39 && data->m_visibleRows==39,"HUD-style history total39");
    check(chat->getFirstRow(1)==0 && chat->getFirstRow(2)==5 && chat->getFirstRow(0)==9 && chat->getFirstRow(7)==25 && chat->getFirstRow(8)==37,"retail first row arithmetic");
    check(data->m_columns[0]->cells.size()==39 && data->m_columns[1]->cells.size()==39 && data->m_columns[2]->cells.empty(),"rebuild appends only columns0/1");
    check(white(data->m_columns[0]->cells[0]->color) && data->getWideCell(1,0).empty(),"real empty white cell construction");
    chat->rebuildRows();
    check(data->getRowCount()==78 && data->m_visibleRows==39,"rebuild appends rather than clears existing rows");
    chat->resetRows();
    check(data->getRowCount()==39,"reset actually destroys rows before rebuilding");
    check(chat->getHistoryInfo()=="number of lines used: 39\nchat message history size: 4\ngameInfo message history size: 5\nkill message history size: 0\npersonal message history size: 3\nrequest message history size: 2\n","real formatted history info");
    chat->setDuration(8,500); chat->setColor(8,3,.1f,.2f,.3f,.4f); chat->setHistoryOutput(false);
    check(chat->m_duration[8]==500 && chat->m_colors[3][8].b==.3f && !chat->m_historyOutput,"configuration writes exact indexed fields");
    check(data->appendWideCell(-1,L"ignored",1,1,1,1,0),"negative column silently succeeds");
    check(data->m_columns.size()==3,"invalid append does not create columns");
    check(data->setWideCell(1,0,L"wide-value",.2f,.3f,.4f,.5f,17,23),"update existing wide cell");
    ListBoxCell* cell=data->m_columns[1]->cells[0];
    check(cell->wide==L"wide-value" && cell->isWide && !cell->isImage && cell->field064==17 && cell->field090==23 && cell->resourceIndex==-1,"wide-cell payload and reset flags");
    data->m_activeColumn=data->m_columns[1];
    check(data->getCharacterElement(nullptr,0,2)=='d' && data->getCharacterElement(nullptr,0,99)==0,"wide character access and upper bound");
    cell->isWide=false; cell->narrow=std::string(1,static_cast<char>(0xE1));
    check(data->getCharacterElement(nullptr,0,0)==-31,"narrow byte sign extension");
    cell->isImage=true;
    check(data->getCharacterElement(nullptr,0,0)==0,"image cells have no text");
    check(data->getCharacterElement(nullptr,-1,0)==0,"negative element rejected by unsigned bound");
    check(data->setWideCell(1,400,L"append-tail",1,1,1,1,7,99),"out of range row appends");
    check(data->m_columns[1]->cells.size()==40 && data->m_columns[1]->cells.back()->field090==0,"append ignores requested row and extra");
    check(data->getWideCell(-1,0).empty() && data->getWideCell(0,-1).empty(),"wide getter invalid coordinates empty");
    data->setRow(500);
    check(!data->updateSelection(nullptr,nullptr,nullptr) && data->m_row==0,"selection clamps tozero and returns false");
    check(data->activateSelection(nullptr),"rowzero with no action returns true");
    data->setRow(-1);
    check(!data->activateSelection(nullptr),"negative row rejects activation");
    ResourceObserver resource; resource.data=data; data->m_resource=&resource;
    const std::vector<ListBoxColumn*> leaked=data->m_columns;
    const int columnWatch=watch(leaked[0]), cellWatch=watch(leaked[0]->cells[0]);
    chat->~HudChat();
    check(resource.calls==1,"chat deletion releases resource exactlyonce");
    check(!freed[columnWatch] && !freed[cellWatch],"retail destructor leaks columns and cells");
    freeLeakedColumns(leaked); // [TEMP] explicit fixture cleanup after observing retail leak

    TimerObserver timer;
    for(int i=0;i<9;++i) { timer.m_count[i]=0; timer.m_remaining[i]=10; timer.m_firstRow[i]=12; }
    timer.m_count[0]=2; timer.m_remaining[0]=.5f; timer.expected=&timer.m_count[0]; timer.mutateOther=true;
    timer.update(.5f);
    check(timer.drops==1 && timer.m_count[0]==1 && timer.m_remaining[0]==0,"timer equality expires and overwrites callback timer");
    check(timer.m_count[1]==1 && timer.m_remaining[1]==3.5f,"timer rereads later category after callback");
    timer.m_count[0]=0; timer.m_count[1]=1; timer.m_remaining[1]=std::numeric_limits<float>::quiet_NaN();
    timer.advanceTimers(1);
    check(timer.drops==1 && timer.m_remaining[1]!=timer.m_remaining[1],"NaN timer does not expire");
    int count=2;
    check(timer.nextRow(12,&count,3)==14 && count==2,"nextRow below capacity no mutation");
    timer.expected=&count; timer.mutateOther=false;
    check(timer.nextRow(12,&count,2)==13 && count==1,"nextRow saves pre-callback count");

    ListBoxData base("base");
    check(base.getObjectSize()==16 && base.m_row==0 && base.onCreate(nullptr),"real generic base defaults");
    check(base.getCharacterElement(nullptr,0,0)==0 && base.getElementCount(nullptr)==0 && !base.activateSelection(nullptr) && !base.updateSelection(nullptr,nullptr,nullptr),"generic base folded methods");
    TypeDescriptor* type=ListBoxData::getClassType();
    check(std::strcmp(type->getName(),"dice::meme::ListBoxData")==0 && type->getBaseType()==Data::getClassType(),"complete base descriptor identity");
    RuntimeObject* created=type->create();
    check(created && created->getObjectSize()==16 && static_cast<ListBoxData*>(created)->m_row==0,"complete generic factory actual constructor");
    delete created;
    std::printf("HudChat/BfListBoxData diagnostic: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}
