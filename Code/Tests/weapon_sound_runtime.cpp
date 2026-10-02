// [TEMP] [CHOSEN, not recovered] Observers for the real template factory and weapon sound path.
// No audio device, retail DLL, game process, or server is started.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include "../BF2/Game/Objects/Base/SimpleObjectTemplate.h"
#include "../BF2/Game/Objects/GenericFireArmTemplate.h"
#include "../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../BF2/Game/Objects/Weapons/SoundComponents/DefaultSoundComp.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../BF2/Sound/BfSound.h"
#include "../BF2/Sound/SoundObjectTemplate.h"
#include "../BF2/Sound/ObjectSoundNode.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Game/Objects/IPlayer.h"

using namespace dice;
using namespace dice::hfe;
using namespace dice::hfe::world;
static_assert(sizeof(void*) == 4, "This fixture validates the x86 client and server contracts");

namespace {
int checks = 0, failures = 0, dedicatedReads = 0, nameReads = 0, lookups = 0;
bool runtimeDedicated = false;
std::string trace;
std::string weaponName("test_weapon");
std::vector<std::string> names;
int playerReads = 0;
IWeaponObject* expectedWeapon = 0;
void check(bool value, const char* message)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", message); }
}
[[noreturn]] void unexpectedSoundFixture()
{
    std::fprintf(stderr, "Unexpected weapon sound fixture collaborator\n");
    std::abort();
}

struct TemplateSpy : NullSoundTemplate {
    const char* filename = "weapons/test_fire.wav";
    int samples = 1;
    mutable int filenameReads = 0;
    const char* getFilename() const override
    { ++filenameReads; trace += 'F'; return filename; }
    const char** getFilenames(int* count) const override
    { *count = samples; return const_cast<const char**>(&filename); }
};

// Only the ISoundObjectTemplate face is called. Its placement uses the real header's base size.
// The primary ObjectTemplate storage has no lifetime or fake engine implementation.
struct TemplateFace : ISoundObjectTemplate {
    TemplateSpy* value = 0;
    mutable int reads = 0;
    ISoundTemplate* impl() const override { ++reads; trace += 'I'; return value; }
    void addRef() override { unexpectedSoundFixture(); }
    int getRef() override { unexpectedSoundFixture(); }
    int release() override { unexpectedSoundFixture(); }
    void* queryInterface(unsigned int) const override { unexpectedSoundFixture(); }
    void setIsUsed() override { unexpectedSoundFixture(); }
    void setFilename(const std::string&) override { unexpectedSoundFixture(); }
    std::string getFilename() const override { unexpectedSoundFixture(); }
    void setVolumeEnvelope(const std::string&) override { unexpectedSoundFixture(); }
    std::string getVolumeEnvelope() const override { unexpectedSoundFixture(); }
    void setPitchEnvelope(const std::string&) override { unexpectedSoundFixture(); }
    std::string getPitchEnvelope() const override { unexpectedSoundFixture(); }
    void setLoopCount(int) override { unexpectedSoundFixture(); }
    int getLoopCount() const override { unexpectedSoundFixture(); }
    void setVolume(float) override { unexpectedSoundFixture(); }
    float getVolume() const override { unexpectedSoundFixture(); }
    void setPitch(float) override { unexpectedSoundFixture(); }
    float getPitch() const override { unexpectedSoundFixture(); }
    void setMinDistance(float) override { unexpectedSoundFixture(); }
    float getMinDistance() const override { unexpectedSoundFixture(); }
    void setHalfVolumeDistance(float) override { unexpectedSoundFixture(); }
    float getHalfVolumeDistance() const override { unexpectedSoundFixture(); }
    void setReverbLevel(float) override { unexpectedSoundFixture(); }
    float getReverbLevel() const override { unexpectedSoundFixture(); }
    void setSoundRadius(float) override { unexpectedSoundFixture(); }
    float getSoundRadius() const override { unexpectedSoundFixture(); }
    void setLowSamples(int) override { unexpectedSoundFixture(); }
    int getLowSamples() const override { unexpectedSoundFixture(); }
    void setMediumSamples(int) override { unexpectedSoundFixture(); }
    int getMediumSamples() const override { unexpectedSoundFixture(); }
    void setRelativeTransformation(const Mat4*) override { unexpectedSoundFixture(); }
};
struct TemplateStorage {
    alignas(ObjectTemplate) unsigned char primary[sizeof(ObjectTemplate)] = {};
    TemplateFace face;
    IObjectTemplate* object() { return reinterpret_cast<IObjectTemplate*>(primary); }
};

struct SoundSpy : NullSound {
    TemplateSpy* soundTemplate = 0;
    ISoundNode* parent = 0;
    int parentCalls = 0, plays = 0, playCount = -1, stops = 0, releases = 0;
    int release() override { ++releases; return 1; }
    void setParent(ISoundNode* value) override { parent = value; ++parentCalls; }
    void play(int count) override { ++plays; playCount = count; }
    void stop() override { ++stops; }
    ISoundTemplate* getTemplate() const override { return soundTemplate; }
};
struct FacadeSpy : BfSoundNull {
    SoundSpy sounds[DefaultSoundComp::SoundCount];
    int creates = 0;
    IObject* expectedOwner = 0;
    TemplateSpy* expectedTemplate = 0;
    bool refuse = false;
    int release() override { unexpectedSoundFixture(); }
    ISound* createSound(ISoundTemplate* value, IObject* owner) override
    {
        trace += 'C';
        check(value == expectedTemplate && owner == expectedOwner,
              "facade receives the exact engine template and object owner");
        SoundSpy& sound = sounds[creates++ % DefaultSoundComp::SoundCount];
        sound.soundTemplate = expectedTemplate;
        return refuse ? 0 : &sound;
    }
};
IObjectTemplate* lookupResult = 0;
const std::string& __fastcall readName(void*, void*)
{ ++nameReads; trace += 'N'; return weaponName; }
IObjectTemplate* __fastcall lookup(void*, void*, const std::string& name, unsigned int classId)
{
    ++lookups; trace += 'L'; names.push_back(name);
    check(classId == 9986001u, "lookup uses the Sound template class");
    return lookupResult;
}
IPlayer* __fastcall readNoPlayer(void* self, void*)
{
    check(self == expectedWeapon, "postFire reads the firing weapon's player");
    ++playerReads;
    return 0;
}

void clearObservations(TemplateFace& face, TemplateSpy& soundTemplate)
{
    trace.clear(); names.clear(); dedicatedReads = nameReads = lookups = 0;
    face.reads = soundTemplate.filenameReads = 0;
}
}

// Test-only collaborators outside the factory/component under test. There is no player or world.
namespace dice { namespace hfe {
IBfSound* g_bfSound = 0;
Flag32Bits<world::IPlayer> g_playerFlagBit3;
int BfSoundNull::release() { unexpectedSoundFixture(); }
Debug::Debug(DebugType, const std::string&, int, const std::string&) { unexpectedSoundFixture(); }
Debug::~Debug() {}
namespace WorldPref { int getTickCount() { unexpectedSoundFixture(); } }
#if !DEDICATED
bool isDedicated() { ++dedicatedReads; trace += 'D'; return runtimeDedicated; }
#endif
namespace world {
ObjectTemplateManager* objectTemplateManager = 0;
extern const unsigned int IID_IObjectTemplateComponent = 50379;
extern const unsigned int IID_IObjectComponent = 50043;
IPlayer* GenericFireArm::getPlayerByForce() const { return 0; }
}
} }

int main()
{
    std::setvbuf(stdout, 0, _IONBF, 0);
    // Narrow x86 MSVC tables: offsets verified in the compiled callers, including overload order.
    void* ownerTable[12] = {};
    ownerTable[0x28 / 4] = reinterpret_cast<void*>(&readName);
    alignas(GenericFireArmTemplate) unsigned char ownerBytes[sizeof(GenericFireArmTemplate)] = {};
    *reinterpret_cast<void***>(ownerBytes) = ownerTable;
    GenericFireArmTemplate* ownerTemplate = reinterpret_cast<GenericFireArmTemplate*>(ownerBytes);
    void* managerTable[13] = {};
    managerTable[0x30 / 4] = reinterpret_cast<void*>(&lookup);
    void** manager = managerTable;
    objectTemplateManager = reinterpret_cast<ObjectTemplateManager*>(&manager);
    alignas(GenericFireArm) unsigned char gunBytes[sizeof(GenericFireArm)] = {};
    GenericFireArm* gun = reinterpret_cast<GenericFireArm*>(gunBytes);
    IObject* owner = static_cast<IObject*>(gun);
    // postFire calls getPlayer through the compiler-adjusted IWeaponObject face.
    void* gunTable[26];
    for (void*& slot : gunTable) slot = reinterpret_cast<void*>(&unexpectedSoundFixture);
    gunTable[0x64 / 4] = reinterpret_cast<void*>(&readNoPlayer);
    expectedWeapon = static_cast<IWeaponObject*>(gun);
    *reinterpret_cast<void***>(expectedWeapon) = gunTable;

    TemplateSpy soundTemplate;
    TemplateStorage wrapper;
    wrapper.face.value = &soundTemplate;
    check(static_cast<ISoundObjectTemplate*>(reinterpret_cast<SoundObjectTemplate*>(wrapper.primary))
              == &wrapper.face, "fixture keeps the real secondary sound-template face offset");
    lookupResult = wrapper.object();
    FacadeSpy facade;
    facade.expectedOwner = owner;
    facade.expectedTemplate = &soundTemplate;
    g_bfSound = &facade;

#if DEDICATED
    for (int dedicated = 0; dedicated < 2; ++dedicated) {
        runtimeDedicated = dedicated != 0;
        check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0,
              "dedicated image retains its null sound factory");
    }
    check(trace.empty() && lookups == 0 && facade.creates == 0,
          "dedicated image reads neither runtime mode nor sound services");
#else
    void* sound = ownerTemplate->createSoundFromTemplate("Fire1P", owner);
    check(sound == &facade.sounds[0], "valid weapon sound template reaches the sound facade");
    if (!sound) return 1; // The old unconditional-null implementation fails here without a crash.
    check(names.size() == 1 && names[0] == "S_test_weapon_Fire1P", "factory resolves the authored bundle sound name");
    check(trace == "DNLIIFIFIC" && wrapper.face.reads == 4 && soundTemplate.filenameReads == 2,
          "factory preserves retail lookup, four impl reads, and two filename reads");

    clearObservations(wrapper.face, soundTemplate);
    runtimeDedicated = true;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "D",
          "client runtime dedicated mode refuses sound without querying services");
    runtimeDedicated = false;
    clearObservations(wrapper.face, soundTemplate);
    g_bfSound = 0;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "D",
          "absent sound facade returns null before lookup");
    g_bfSound = &facade;
    clearObservations(wrapper.face, soundTemplate);
    lookupResult = 0;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "DNL",
          "missing named template returns null");
    lookupResult = wrapper.object();
    clearObservations(wrapper.face, soundTemplate);
    wrapper.face.value = 0;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "DNLI",
          "missing template implementation returns null");
    wrapper.face.value = &soundTemplate;
    clearObservations(wrapper.face, soundTemplate);
    soundTemplate.filename = 0;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "DNLIIF",
          "null sample filename returns null");
    clearObservations(wrapper.face, soundTemplate);
    soundTemplate.filename = "";
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "DNLIIFIF",
          "empty sample filename returns null");
    soundTemplate.filename = "weapons/test_fire.wav";
    clearObservations(wrapper.face, soundTemplate);
    facade.refuse = true;
    check(ownerTemplate->createSoundFromTemplate("Fire1P", owner) == 0 && trace == "DNLIIFIFIC",
          "sound-engine creation failure remains null");
    facade.refuse = false;
    facade.creates = 0;

    clearObservations(wrapper.face, soundTemplate);
    {
        DefaultSoundComp component(0);
        component.setObject(owner);
        component.init(owner);
        WeaponInitEvent init = { ownerTemplate, 600 };
        component.initComponent(init);
        check(facade.creates == 11 && names.size() == 11,
              "actual weapon init creates all eleven sound slots through the factory");
        const char* suffixes[] = { "Fire1P", "Fire1P_Outdoor", "Fire3P", "BoltClick",
            "TriggerClick", "SwitchFireRate", "Reload1P", "Reload3P", "Deploy1P", "Deploy3P", "Zoom" };
        for (int i = 0; i < 11; ++i) {
            check(names[i] == "S_test_weapon_" + std::string(suffixes[i]), "weapon init preserves authored sound order");
            check(facade.sounds[i].parentCalls == 1 && facade.sounds[i].parent != 0,
                  "weapon sound is attached to its object sound node");
            check(static_cast<ObjectSoundNode*>(facade.sounds[i].parent)->m_object == owner,
                  "sound node retains the weapon owner");
        }
        WeaponFireEvent fire = {};
        fire.fireRate = static_cast<FireRate>(0);
        fire.maxProjectilesToFire = 1;
        component.postFire(fire);
        check(facade.sounds[DefaultSoundComp::Fire3P].plays == 1
                  && facade.sounds[DefaultSoundComp::Fire3P].playCount == 1,
              "actual weapon postFire plays the created third-person sample");
        fire.fireRate = static_cast<FireRate>(2);
        component.postFire(fire);
        component.postFire(fire);
        check(facade.sounds[DefaultSoundComp::Fire3P].plays == 2
                  && facade.sounds[DefaultSoundComp::Fire3P].playCount == 0,
              "automatic fire starts one continuous sound without retriggering");
        component.fireStop(false);
        check(playerReads == 2, "only the two new playback requests read the player");
        check(facade.sounds[0].stops == 1 && facade.sounds[1].stops == 1 && facade.sounds[2].stops == 1,
              "release of automatic fire stops the three firing sounds");
    }
    for (int i = 0; i < 11; ++i)
        check(facade.sounds[i].releases == 1, "weapon destruction releases each created sound once");
#endif
    std::printf("weapon sound runtime: %d/%d checks passed (DEDICATED=%d)\n", checks - failures, checks, DEDICATED);
    return failures ? 1 : 0;
}
