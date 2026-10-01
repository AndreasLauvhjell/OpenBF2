// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: menu-team-runtime]
// Strict native x86 owner/parser/XML tests. Only fileManager/mainConsole global
// expressions are redirected to narrow typed observers while including the real
// implementation files. No MenuTeamManager, tokenizer, or XML body is replaced.
// This is component validation, not real console/VFS or client startup acceptance.
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>
#include "../BF2/Menu/MenuTeamManager.h"
#include "../BF2/IO/File/FileManager.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/Game/Utility/XMLDocument.h"
#include "../BF2/Core/Ids.h"

using namespace dice::hfe;
static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)

namespace fixture {
std::string key(std::string value)
{
    for (char& c : value) { if (c == '\\') c = '/'; else if (c >= 'A' && c <= 'Z') c += 'a'-'A'; }
    return value;
}
struct Stream : io::INonlinearStream
{
    std::string data; unsigned position = 0; static unsigned destroyed;
    explicit Stream(const std::string& s) : data(s) {}
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { ++destroyed; delete this; return 0; }
    void* queryInterface(unsigned) const override { return nullptr; }
    unsigned read(void* buffer, unsigned size) override {
        unsigned count = (std::min)(size, static_cast<unsigned>(data.size())-position);
        std::memcpy(buffer, data.data()+position, count); position += count; return count;
    }
    unsigned write(const void*, unsigned) override { std::abort(); }
    int skip(unsigned n) override { position += n; return position; }
    int getAvailableBytes() override { return static_cast<int>(data.size()-position); }
    io::IStream* clone() override { std::abort(); }
    bool canRead() override { return true; }
    bool canWrite() override { return false; }
    int seek(io::SeekOrigin, int) override { std::abort(); }
    int getPosition() override { return position; }
    int getSize() override { return static_cast<int>(data.size()); }
};
unsigned Stream::destroyed;
struct Files
{
    std::map<std::string, std::string> content;
    std::vector<std::string> calls;
    bool rejectRead = false, reportShort = false;
    virtual bool getFileInfo(const std::string& file, io::FileInfo& info) {
        calls.push_back("info:"+file); auto it=content.find(key(file));
        if(it==content.end()) return false; info.size=static_cast<unsigned>(it->second.size()); return true;
    }
    virtual bool readFile(const std::string& file, void* data, unsigned size, unsigned& read) {
        calls.push_back("read:"+file); auto it=content.find(key(file));
        if(rejectRead || it==content.end()) return false;
        CHECK(size==it->second.size()); std::memcpy(data,it->second.data(),size);
        read=reportShort?0:size; return true;
    }
    virtual io::IStream* open(const std::string& file, io::FileMode mode, io::FileAccess access, unsigned flags) {
        calls.push_back("open:"+file); CHECK(mode==io::FILEMODE_READ && access==0 && flags==0);
        auto it=content.find(key(file)); return it==content.end()?nullptr:new Stream(it->second);
    }
    void put(const std::string& file,const std::string& data) { content[key(file)]=data; }
};
struct Console
{
    std::vector<std::string> calls;
    virtual bool run(const std::string& file, const std::string& a,const std::string& b,
        const std::string& c,const std::string& d,const std::string& e,const std::string& f,
        const std::string& g,const std::string& h,std::string& out) {
        CHECK(a.empty()&&b.empty()&&c.empty()&&d.empty()&&e.empty()&&f.empty()&&g.empty()&&h.empty());
        CHECK(&out==&io::Console::ignoredString_); calls.push_back(file); return false;
    }
};
}
namespace dice { namespace hfe { namespace io {
fixture::Files* menuFixtureFiles;
fixture::Console* menuFixtureConsole;
std::string Console::ignoredString_;
} } }

// [TEMP] The two collaborator seams only. Their original headers are already
// loaded above; all record declarations and production method signatures stay real.
#define fileManager menuFixtureFiles
#define mainConsole menuFixtureConsole
#include "../BF2/Menu/MenuTeamManager.cpp"
#include "../BF2/Menu/MenuTeamManagerData.cpp"
#include "../BF2/Game/Utility/XMLDocument.cpp"
#undef fileManager
#undef mainConsole

static MenuEquipment equipment(const std::string& name,const std::string& file="")
{
    MenuEquipment e;
    e.tweakFile=file.empty()?name+".tweak":file; e.templateName=name;
    e.displayName="Display "+name; e.description="Description "+name;
    e.stat70=11; e.stat74=22; e.stat78=33; e.unlockLevel=0; e.itemIndex=0; e.equipmentType=1;
    return e;
}
static void ownerTests(fixture::Console& console)
{
    CHECK(sizeof(void*)==4);
    MenuTeamManager owner;
    IMenuTeamManager* iface=&owner;
    CHECK(iface->getRef()==1 && iface->queryInterface(1)==iface);
    CHECK(iface->queryInterface(IID_IMenuTeamManager)==iface && !iface->queryInterface(0xFEED));
    iface->addRef(); CHECK(iface->getRef()==2 && iface->release()==1);
    CHECK(!iface->addKit("no-team.con") && iface->getTeamCount()==0);
    CHECK(iface->addTeam("First", "p1") && iface->addTeam("Second", "p2"));
    CHECK(iface->getTeamCount()==2 && iface->getTeam(0)->teamId==0 && iface->getTeam(1)->index==1);
    CHECK(iface->getTeam(0)->path=="p1" && iface->findTeamByName("fIrSt")==iface->getTeam(0));
    CHECK(!iface->findTeamByName("absent") && !iface->findTeamById(333));
    CHECK(iface->setTeamId(10) && iface->getTeam(1)->teamId==10);
    CHECK(iface->setTeamId(11) && iface->getTeam(1)->teamId==9);
    CHECK(iface->setTeamId(-7) && iface->findTeamById(-7)==iface->getTeam(1));
    MenuEquipment first=equipment("Rifle","Unique.TWEAK");
    iface->addEquipment(first); first.displayName="Changed local";
    CHECK(iface->findEquipment("rifle")->displayName=="Display Rifle");
    MenuEquipment duplicate=equipment("OtherName","unique.tweak"); iface->addEquipment(duplicate);
    CHECK(!iface->findEquipment("OtherName"));
    iface->addWeapon("RiFlE", -1, 999, 4);
    CHECK(iface->findEquipment("Rifle")->stat70==-1 && iface->findEquipment("Rifle")->stat74==999 && iface->findEquipment("Rifle")->stat78==4);
    iface->addWeapon("unknown",1,2,3); CHECK(!iface->findEquipment("unknown"));
    CHECK(iface->init() && iface->getTeamCount()==0);
    CHECK(console.calls==std::vector<std::string>({"menu/external/flashmenu/menuTeams.con","menu/external/flashmenu/weaponStats.con","menu/external/flashmenu/gameModes.con"}));
    CHECK(iface->findEquipment("Rifle")!=nullptr); // Init clears only teams.
    iface->addTeam("retained", "path"); CHECK(iface->init() && iface->getTeamCount()==1 && console.calls.size()==3);
    iface->visitEquipment();
    CHECK(!MenuTeamManager::create(IID_IMenuTeamManager,&owner));
    CHECK(!MenuTeamManager::create(0xFEED,nullptr));
    IBase* created=MenuTeamManager::create(IID_IMenuTeamManager,nullptr);
    CHECK(created && created->getRef()==1 && created->release()==0);
}
static void aggregationTests()
{
    MenuTeamManager owner;
    for(unsigned t=0;t<4;++t) owner.addTeam(std::to_string(t),"path");
    const MenuEquipment alpha=equipment("alpha"),zulu=equipment("Zulu"),fourth=equipment("Fourth");
    owner.addEquipment(alpha); owner.addEquipment(zulu); owner.addEquipment(fourth);
    MenuKit kit; kit.index=0; kit.sprintStaminaDissipationFactor=0; kit.equipment={alpha,zulu,alpha};
    owner.getTeam(0)->kits.push_back(kit);
    kit.equipment={alpha}; owner.getTeam(0)->kits.push_back(kit);
    owner.getTeam(1)->kits.push_back(kit);
    kit.equipment={equipment("missing")}; owner.getTeam(2)->kits.push_back(kit);
    kit.equipment={fourth}; owner.getTeam(3)->kits.push_back(kit);
    std::vector<MenuEquipmentWithKits> out(1); out[0].templateName="sentinel";
    CHECK(owner.getWeaponKitMap(out) && out.size()==3 && out[0].templateName=="sentinel");
    CHECK(out[1].templateName=="Zulu" && out[1].kitIndices==std::vector<unsigned>({0}));
    CHECK(out[2].templateName=="alpha" && out[2].kitIndices==std::vector<unsigned>({0,0,1,0}));
    CHECK(out[2].stat74==22 && out[2].displayName=="Display alpha");
    CHECK(owner.getWeaponKitMap(out) && out.size()==5); // Append, never clear.
}
static void parserTests(fixture::Files& files)
{
    char* result=reinterpret_cast<char*>(0x1234); unsigned size=0x5678;
    CHECK(!dice::hfe::readMenuFile("missing",result,size) && result==reinterpret_cast<char*>(0x1234) && size==0x5678);
    files.put("read.test","abc"); files.rejectRead=true;
    CHECK(!dice::hfe::readMenuFile("read.test",result,size) && result==reinterpret_cast<char*>(0x1234) && size==0x5678);
    files.rejectRead=false; files.reportShort=true;
    CHECK(dice::hfe::readMenuFile("read.test",result,size) && size==3 && std::strcmp(result,"abc")==0); delete[] result;
    files.reportShort=false;
    MenuEquipment weapon=equipment("test"); weapon.itemIndex=77;
    files.put("weapon.tweak","objecttemplate.itemIndex 1\r\nObjectTemplate.itemIndex\nObjectTemplate.itemIndex \"-4\"\rObjectTemplate.itemIndex 19junk\n");
    CHECK(dice::hfe::parseMenuWeapon("weapon.tweak",weapon) && weapon.itemIndex==19);
    CHECK(!dice::hfe::parseMenuWeapon("missing",weapon) && weapon.itemIndex==19);

    files.put("kit.con",
        "ObjectTemplate.create kit Named\nObjectTemplate.vehicleHud.hudName\n"
        "ObjectTemplate.create kit\nObjectTemplate.create KIT FinalKit\n"
        "ObjectTemplate.sprintStaminaDissipationFactor 1.25junk\n"
        "ObjectTemplate.addTemplate GasMask\nObjectTemplate.addTemplate NightVisionGoggles\n"
        "ObjectTemplate.addTemplate thing_mask\nObjectTemplate.addTemplate thing_nvg\n"
        "objecttemplate.addTemplate ignored_case\n"
        "ObjectTemplate.addTemplate rifle\nObjectTemplate.unlockLevel 2\n"
        "ObjectTemplate.addTemplate rifle\nObjectTemplate.unlockLevel 3\n"
        "ObjectTemplate.create ITEMCONTAINER RIFLE\n"
        "ObjectTemplate.addTemplate Thing_MASK\n"
        "ObjectTemplate.vehicleHud.hudName \"HUD Label\"\n");
    files.put("objects/weapons/handheld/rifle/Weapon.desc","<weapon eq=\"7\"><name locid=\"NAME_KEY\">name text</name><description locid=\"\">fallback text</description></weapon>");
    MenuKit kit; kit.sprintStaminaDissipationFactor=0;
    CHECK(dice::hfe::parseMenuKit("kit.con",kit));
    CHECK(kit.templateName=="FinalKit" && kit.hudName=="HUD" && kit.sprintStaminaDissipationFactor==1.25f);
    CHECK(kit.equipment.size()==2 && kit.equipment[0].templateName=="rifle" && kit.equipment[1].templateName=="Thing_MASK");
    CHECK(kit.equipment[0].unlockLevel==3 && kit.equipment[0].itemIndex==0 && kit.equipment[0].equipmentType==7);
    CHECK(kit.equipment[0].displayName=="NAME_KEY" && kit.equipment[0].description=="fallback text" && kit.equipment[0].tweakFile=="rifle.tweak");
    CHECK(kit.equipment[1].displayName=="Thing_MASK" && kit.equipment[1].description.empty());
    files.put("fallback.con","ObjectTemplate.create kit Name\nObjectTemplate.vehicleHud.hudName\n");
    MenuKit fallback; CHECK(dice::hfe::parseMenuKit("fallback.con",fallback) && fallback.hudName=="Name");
    files.put("clear.con","ObjectTemplate.create kit Name\nObjectTemplate.create kit\n");
    CHECK(dice::hfe::parseMenuKit("clear.con",fallback) && fallback.templateName.empty());

    files.put("p/kit.con","ObjectTemplate.create kit Test\nObjectTemplate.addTemplate rifle\nObjectTemplate.addTemplate lost\n");
    files.put("objects/weapons/handheld/rifle/rifle.tweak","ObjectTemplate.itemIndex 5\n");
    MenuTeamManager owner; owner.addTeam("Team","p");
    CHECK(!owner.addKit("missing.con") && owner.getTeam(0)->kits.empty());
    CHECK(owner.addKit("kit.con") && owner.getTeam(0)->kits.size()==1);
    CHECK(owner.getTeam(0)->kits[0].index==0 && owner.getTeam(0)->kits[0].file=="kit.con" && owner.getTeam(0)->kits[0].sprintStaminaDissipationFactor==0);
    CHECK(owner.getTeam(0)->kits[0].equipment.size()==2 && owner.getTeam(0)->kits[0].equipment[0].itemIndex==5);
    CHECK(owner.findEquipment("rifle") && !owner.findEquipment("lost"));
    CHECK(owner.addKit("kit.con") && owner.getTeam(0)->kits[1].index==1);
    CHECK(fixture::Stream::destroyed>0);
}
static void loadDirectory(fixture::Files& files,const std::string& root,const std::string& relative)
{
    WIN32_FIND_DATAA info; HANDLE find=FindFirstFileA((root+"/"+relative+"*").c_str(),&info);
    if(find==INVALID_HANDLE_VALUE) return;
    do {
        const std::string name=info.cFileName; if(name=="."||name=="..") continue;
        if(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) loadDirectory(files,root,relative+name+"/");
        else { std::ifstream file(root+"/"+relative+name,std::ios::binary); files.put(relative+name,std::string(std::istreambuf_iterator<char>(file),{})); }
    } while(FindNextFileA(find,&info));
    FindClose(find);
}
static void stockTests(fixture::Files& files,const char* directory)
{
    files.content.clear(); loadDirectory(files,directory,"");
    const auto script=files.content.find("menu/external/flashmenu/menuteams.con");
    CHECK(script!=files.content.end()); if(script==files.content.end()) return;
    MenuTeamManager owner; SimpleTokenizer lines(script->second,"\n\r"); std::string line;
    while(lines.next(line)) {
        SimpleTokenizer args(line," \t\""); std::string command,a,b; if(!args.next(command)) continue;
        if(command=="MenuTeamManager.addTeam") { CHECK(args.next(a)&&args.next(b)); CHECK(owner.addTeam(a,b)); }
        else if(command=="MenuTeamManager.setTeamId") { CHECK(args.next(a)); CHECK(owner.setTeamId(std::atoi(a.c_str()))); }
        else if(command=="MenuTeamManager.addKit") { CHECK(args.next(a)); CHECK(owner.addKit(a)); }
    }
    CHECK(owner.getTeamCount()==4);
    const int expectedIds[]={0,1,2,9}; unsigned kitCount=0,itemCount=0;
    for(unsigned t=0;t<4;++t) { MenuTeam* team=owner.getTeam(t); CHECK(team->teamId==expectedIds[t] && team->kits.size()==7);
        kitCount+=static_cast<unsigned>(team->kits.size());
        for(unsigned k=0;k<team->kits.size();++k) { CHECK(team->kits[k].index==k && !team->kits[k].templateName.empty()); itemCount+=static_cast<unsigned>(team->kits[k].equipment.size()); }
    }
    CHECK(kitCount==28 && itemCount>100);
    std::vector<MenuEquipmentWithKits> out; CHECK(owner.getWeaponKitMap(out) && out.size()>30);
    std::printf("Stock extracted-data fixture: %u teams, %u kits, %u kit equipment entries, %u catalogue records in first-three-team map\n",owner.getTeamCount(),kitCount,itemCount,static_cast<unsigned>(out.size()));
}
int main(int argc,char** argv)
{
    fixture::Files files; fixture::Console console;
    io::menuFixtureFiles=&files; io::menuFixtureConsole=&console;
    ownerTests(console); aggregationTests(); parserTests(files);
    if(argc>1) stockTests(files,argv[1]);
    std::printf("MenuTeamManager component fixture: %u/%u passed (typed collaborator seams; strict link)\n",checks-failures,checks);
    return failures?1:0;
}
