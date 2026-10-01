// [TEMP] Complete existing IPCO interface; unsupported fixture calls stop.
namespace dice { namespace hfe { namespace world {
struct RadioPcoBoundary : IPlayerControlObject {
void addRef() override {actionUnsupported("PCO addRef");}
int getRef() override {actionUnsupported("PCO getRef");}
int release() override {actionUnsupported("PCO release");}
void* queryInterface(unsigned) const override {actionUnsupported("PCO queryInterface");}
bool isOccupied() const override {actionUnsupported("PCO isOccupied");}
bool enter(IPlayer* player) override {actionUnsupported("PCO enter");}
void exit() override {actionUnsupported("PCO exit");}
void exitPlayer() override {actionUnsupported("PCO exitPlayer");}
void onEnter(IObject*) override {actionUnsupported("PCO onEnter");}
void onExit() override {actionUnsupported("PCO onExit");}
int getPcoId() const override {actionUnsupported("PCO getPcoId");}
void setPcoId(int id) override {actionUnsupported("PCO setPcoId");}
std::map<int, ICameraObject*>* getCameras() override {actionUnsupported("PCO getCameras");}
std::map<int, ICameraObject*>* getOwnCameras() override {actionUnsupported("PCO getOwnCameras");}
void nextCamera() override {actionUnsupported("PCO nextCamera");}
ICameraObject* getActiveCamera() override {actionUnsupported("PCO getActiveCamera");}
void setActiveCamera(int) override {actionUnsupported("PCO setActiveCamera");}
const std::vector<int>* getPcoPosId() override {actionUnsupported("PCO getPcoPosId");}
void addWeapon(IObject* weapon) override {actionUnsupported("PCO addWeapon");}
void* getWeapons() override {actionUnsupported("PCO getWeapons");}
IObject* getActiveWeapon(int) const override {actionUnsupported("PCO getActiveWeapon");}
void* getWeapons(int) override {actionUnsupported("PCO getWeapons");}
void setActiveWeapon(IObject* weapon, int) override {actionUnsupported("PCO setActiveWeapon");}
void toggleOnWeaponLock(IObject*) override {actionUnsupported("PCO toggleOnWeaponLock");}
int getArtPos() override {actionUnsupported("PCO getArtPos");}
void setTeam(int team) override {actionUnsupported("PCO setTeam");}
void setDefaultTeam(int team) override {actionUnsupported("PCO setDefaultTeam");}
int getTeam() override {actionUnsupported("PCO getTeam");}
void setTimeToLiveUnused(float t) override {actionUnsupported("PCO setTimeToLiveUnused");}
void addPlayerInputObject(IObject*) override {actionUnsupported("PCO addPlayerInputObject");}
void removePlayerInputObject(IObject*) override {actionUnsupported("PCO removePlayerInputObject");}
void setObjectSpawnerId(int id) override {actionUnsupported("PCO setObjectSpawnerId");}
int getObjectSpawnerId() const override {actionUnsupported("PCO getObjectSpawnerId");}
void setObjectSpawnerHolding(bool holding) override {actionUnsupported("PCO setObjectSpawnerHolding");}
bool getObjectSpawnerHolding() const override {actionUnsupported("PCO getObjectSpawnerHolding");}
const Vec3& getNameTagOffset() override {actionUnsupported("PCO getNameTagOffset");}
const Vec3& getRidingWithOffset() override {actionUnsupported("PCO getRidingWithOffset");}
float getRidingWithDistMod() override {actionUnsupported("PCO getRidingWithDistMod");}
void addVehicleMod(VehicleMod mod) override {actionUnsupported("PCO addVehicleMod");}
void removeVehicleMod(VehicleMod mod) override {actionUnsupported("PCO removeVehicleMod");}
int getVehicleMod(VehicleMod mod) override {actionUnsupported("PCO getVehicleMod");}
const std::string& getSoundFilter() const override {actionUnsupported("PCO getSoundFilter");}
float getShakeFactor() const override {actionUnsupported("PCO getShakeFactor");}
bool getIsSprinting() const override {actionUnsupported("PCO getIsSprinting");}
float getSprintStamina() const override {actionUnsupported("PCO getSprintStamina");}
void setSprintStamina(float) override {actionUnsupported("PCO setSprintStamina");}
void setSprintStaminaDissipationFactor(float) override {actionUnsupported("PCO setSprintStaminaDissipationFactor");}
float getSprintFactor() override {actionUnsupported("PCO getSprintFactor");}
bool getIsUnderwater() const override {actionUnsupported("PCO getIsUnderwater");}
float getNormalizedUnderwaterTime() const override {actionUnsupported("PCO getNormalizedUnderwaterTime");}
};
} } }
