// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: GLOBAL-SETTINGS-FIXTURE]
// Actual GlobalSettings owner, lifetime, defaults, script, accessors and the name-prefix rules.
// The executable links only the class; nothing GlobalSettings owns is shadowed.
#include <cstdio>
#include <cstring>
#include <string>
#include "../BF2/Game/GlobalSettings.h"
#include "../BF2/Core/Ids.h"
using namespace dice::hfe;
static unsigned checks, failures;
static void check(bool value,const char* label)
{ ++checks; if(!value){++failures;std::printf("FAIL %s\n",label);} }

namespace dice { namespace hfe {
struct GlobalSettingsFixture {
    static int refs(IBase* b){ return static_cast<GlobalSettings*>(b)->m_refCount; }
    static const std::string& user(IBase* b){ return static_cast<GlobalSettings*>(b)->m_defaultUser; }
    static const std::string& prefix(IBase* b){ return static_cast<GlobalSettings*>(b)->m_namePrefix; }
};
} }

int main()
{
    check(!GlobalSettings::create(IID_IGlobalSettings,reinterpret_cast<IBase*>(1)),"aggregation is refused before allocation");
    check(!GlobalSettings::create(0xffffffff,nullptr),"unsupported factory id releases the fresh owner");
    IBase* object=GlobalSettings::create(IID_IGlobalSettings,nullptr);
    check(object!=nullptr,"factory returns the owner for its interface id");
    if(!object){ std::printf("GlobalSettings: %u/%u passed\n",checks-failures,checks); return 1; }
    IGlobalSettings* api=static_cast<IGlobalSettings*>(object);
    check(GlobalSettingsFixture::refs(object)==1 && api->getRef()==1,"refcount starts at one and queryInterface does not retain");
    check(api->queryInterface(IID_IBase)==object && api->queryInterface(IID_IGlobalSettings)==object && !api->queryInterface(0xffffffff),
        "queryInterface answers IBase and IGlobalSettings only");
    check(GlobalSettingsFixture::user(object).empty() && GlobalSettingsFixture::prefix(object).empty(),"both strings start empty");
    check(api->getFileName()=="Global.con","the settings file is Global.con");
    check(api->getSettingsScript()=="GlobalSettings.setDefaultUser \"\"\nGlobalSettings.setNamePrefix \"\"\n","the empty script has both rows");
    check(api->apply(),"apply always succeeds");
    api->setDefaultUser("0001");
    check(api->getDefaultUser()=="0001" && GlobalSettingsFixture::user(object)=="0001","default user round trip");
    api->setNamePrefix("=DICE=");
    check(api->getNamePrefix()=="=DICE=" && GlobalSettingsFixture::prefix(object)=="=DICE=","a six-character prefix is stored and returned whole");
    check(api->getSettingsScript()=="GlobalSettings.setDefaultUser \"0001\"\nGlobalSettings.setNamePrefix \"=DICE=\"\n","the script rows carry both values");
    api->setNamePrefix("ABCDEFGH");
    check(GlobalSettingsFixture::prefix(object)=="ABCDEFGH" && api->getNamePrefix()=="ABCDEF","a longer prefix is stored whole and returned truncated to six");
    check(api->getSettingsScript().find("setNamePrefix \"ABCDEFGH\"")!=std::string::npos,"the script writes the stored prefix, not the truncation");
    api->setNamePrefix("bad\xA7tag");
    check(GlobalSettingsFixture::prefix(object)=="ABCDEFGH","a prefix containing the section sign is refused");
    api->setNamePrefix(std::string("\xA7"));
    check(GlobalSettingsFixture::prefix(object)=="ABCDEFGH","a lone section sign is refused");
    api->setNamePrefix("");
    check(api->getNamePrefix().empty(),"an empty prefix is accepted");
    api->setNamePrefix("tag");
    api->setDefaults();
    check(api->getDefaultUser().empty() && api->getNamePrefix()=="tag","setDefaults clears only the default user");
    api->setDefaultUser(std::string(40,'u'));
    check(api->getDefaultUser()==std::string(40,'u'),"a long default user is stored");
    api->addRef();
    check(api->getRef()==2 && api->release()==1,"non-final release keeps the owner");
    check(api->release()==0,"final release reports zero");
    std::printf("GlobalSettings: %u/%u passed; native owner=%zu (retail 64)\n",checks-failures,checks,sizeof(GlobalSettings));
    return failures?1:0;
}
