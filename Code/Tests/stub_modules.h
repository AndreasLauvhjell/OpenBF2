// Test scaffolding: provisional module declarations used by engine setup syntax checks.

#ifndef BF2_STUB_MODULES_H
#define BF2_STUB_MODULES_H

namespace dice {
namespace hfe {

class IModule
{
public:
    virtual ~IModule();
    virtual bool init();
    virtual void destroy();
    virtual void release();
};

// Deliberately a macro: 25 identical empty derivations, so it is obvious at a
// glance that none of them carries any recovered content.
#define BF2_STUB_MODULE(NAME) class NAME : public ::dice::hfe::IModule {}

BF2_STUB_MODULE(Module);
BF2_STUB_MODULE(ModuleBF2);
BF2_STUB_MODULE(ModuleGame);
BF2_STUB_MODULE(ModuleGameLogic);
BF2_STUB_MODULE(MathModule);
BF2_STUB_MODULE(ModuleNetworkables);
BF2_STUB_MODULE(BfSoundModule);
BF2_STUB_MODULE(SpawnSystemModule);

namespace io {
BF2_STUB_MODULE(ModuleIO);
BF2_STUB_MODULE(FileModule);
BF2_STUB_MODULE(NetworkModule);
} // namespace io

namespace world {
BF2_STUB_MODULE(Module);
BF2_STUB_MODULE(ObjectModule);
BF2_STUB_MODULE(ObjTemplPortalModule);
BF2_STUB_MODULE(ObjTemplBaseModule);
BF2_STUB_MODULE(PhysicsModule);
BF2_STUB_MODULE(BFMaterialSystemModule);
BF2_STUB_MODULE(ScoreSystemModule);
BF2_STUB_MODULE(GameLogicObjTemplBaseModule);
BF2_STUB_MODULE(GameLogicObjTemplFxModule);
BF2_STUB_MODULE(GameLogicObjTemplVehicleModule);
BF2_STUB_MODULE(GameLogicObjTemplBFModule);
BF2_STUB_MODULE(GameLogicCompModule);
} // namespace world

namespace tool {
BF2_STUB_MODULE(ModuleToolObject);
} // namespace tool

namespace animationTools {
BF2_STUB_MODULE(AnimModule);
} // namespace animationTools

namespace ai {
BF2_STUB_MODULE(AIInterfaceModule);   // operator new(0x10)
} // namespace ai

// Used by initModules() but declared nowhere in the recovered tree.
extern unsigned int IID_IMemoryPool;
extern unsigned int CID_MemoryPool;
extern unsigned int CID_SpawnManager;

void setSpawnManager(unsigned int);

} // namespace hfe
} // namespace dice

#endif // BF2_STUB_MODULES_H
