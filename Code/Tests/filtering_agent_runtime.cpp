// [TEMP] Dedicated x86 lifetime/dispatch fixture; strict link, real FilteringAgent/PunkBuster bodies.
#include "../BF2/IO/Network/PacketFilter.h"
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <crtdbg.h>
using dice::hfe::io::FilteringAgent;
using dice::hfe::io::PunkBusterFilterAgent;
using dice::hfe::io::PacketFilter;
namespace {
int checks=0, failures=0, destroyed=0, deallocated=0;
void check(bool ok,const char* label) {
    ++checks; if(!ok) ++failures;
    std::printf("%s %s\n",ok?"PASS":"FAIL",label);
}
// This concrete observer adds its own resource lifetime above the real retail-derived agent.
// Its delete callback verifies virtual deletion finished the most-derived destructor first.
struct ObservedAgent : PunkBusterFilterAgent {
    PacketFilter::AgentData* record=0;
    unsigned int expectedPackets=0;
    ~ObservedAgent() {
        if(record) check(record->m_packetList.size()==expectedPackets,
            "classifier is destroyed before queued packet nodes are cleared");
        ++destroyed;
    }
    static void* operator new(std::size_t size) { return ::operator new(size); }
    static void operator delete(void* storage) {
        check(destroyed==deallocated+1,"most-derived destruction precedes storage release");
        ++deallocated; ::operator delete(storage);
    }
};
__declspec(noinline) void destroyThroughBase(FilteringAgent* agent) { delete agent; }
}
int main() {
    static_assert(sizeof(void*)==4,"x86 only");
    static_assert(sizeof(FilteringAgent)==4 && sizeof(PunkBusterFilterAgent)==4,"retail vptr-only classifier layout");
    static_assert(std::is_abstract<FilteringAgent>::value,"classifier is an abstract interface");
    static_assert(std::has_virtual_destructor<FilteringAgent>::value,"base deletion dispatches virtually");
    const unsigned char pb[]={0xff,0xff,0xff,0xff,'P','B','_'};
    const unsigned char normal[]={0xff,0xff,0xff,0xff,'G','A','M','E'};
    FilteringAgent* actual=new PunkBusterFilterAgent();
    check(actual->match(pb,sizeof(pb)),"retail derived match dispatch remains after destructor slot");
    check(!actual->match(pb,6),"short packet cannot match seven-byte prefix");
    check(!actual->match(normal,sizeof(normal)),"ordinary traffic remains unclaimed");
    destroyThroughBase(actual);
    check(true,"actual PunkBusterFilterAgent deletes through FilteringAgent");
    FilteringAgent* observed=new ObservedAgent();
    check(observed->match(pb,sizeof(pb)),"further-derived instance inherits actual packet classifier");
    destroyThroughBase(observed);
    check(destroyed==1 && deallocated==1,"derived resources and allocation release exactly once");
    alignas(PunkBusterFilterAgent) unsigned char storage[sizeof(PunkBusterFilterAgent)];
    actual=new(storage) PunkBusterFilterAgent();
    actual->~FilteringAgent();
    _CrtMemState before, after;
    _CrtMemCheckpoint(&before);
    { PacketFilter empty; }
    _CrtMemCheckpoint(&after);
    check(before.lSizes[_NORMAL_BLOCK]==after.lSizes[_NORMAL_BLOCK],"empty filter releases its vector and mutex storage");
    _CrtMemCheckpoint(&before);
    {
        PacketFilter filter;
        ObservedAgent* owned=new ObservedAgent();
        filter.registerAgent(dice::hfe::io::FILTEREDPACKETTYPE_0,owned);
        owned->record=filter.m_agents[0]; owned->expectedPackets=2;
        filter.storePacket(pb,sizeof(pb),dice::hfe::io::FILTEREDPACKETTYPE_0,"first");
        filter.storePacket(normal,sizeof(normal),dice::hfe::io::FILTEREDPACKETTYPE_0,"second");
        check(!filter.processPacket(pb,sizeof(pb)) && filter.processPacket(normal,sizeof(normal)),
            "registered actual classifier keeps packet filter claim polarity");
    }
    _CrtMemCheckpoint(&after);
    check(destroyed==2 && deallocated==2,"filter destroys and deletes its currently registered classifier");
    check(before.lSizes[_NORMAL_BLOCK]==after.lSizes[_NORMAL_BLOCK],
        "queued payloads, classifier, record, list, vector and mutex leave no normal-heap allocation");
    {
        PacketFilter filter;
        filter.registerAgent(dice::hfe::io::FILTEREDPACKETTYPE_0,new ObservedAgent());
        PacketFilter::AgentData* lost=filter.m_agents[0];
        filter.registerAgent(dice::hfe::io::FILTEREDPACKETTYPE_0,new ObservedAgent());
        check(destroyed==2 && filter.m_agents[0]!=lost,"retail re-registration overwrites without deleting the old record");
        delete lost; // Fixture retains and releases the deliberately orphaned retail record.
    }
    check(destroyed==4 && deallocated==4,"explicit orphan cleanup and filter teardown each delete one classifier");
    check(true,"explicit virtual destruction calls real derived and pure-base body without freeing placement storage");
    actual=new(storage) PunkBusterFilterAgent();
    check(actual->match(pb,sizeof(pb)),"placement storage can be reused after destruction");
    actual->~FilteringAgent();
    std::printf("filtering agent: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
