// [TEMP] Real-interface callback boundaries. Every unexercised method diagnoses and aborts.
namespace dice { namespace hfe { namespace world {
struct ActionPlayerBoundary : IPlayer {
void addRef() override { actionUnsupported("ActionPlayerBoundary::IBase"); }
int getRef() override { actionUnsupported("ActionPlayerBoundary::IBase"); }
int release() override { actionUnsupported("ActionPlayerBoundary::IBase"); }
void* queryInterface(unsigned int) const override { actionUnsupported("ActionPlayerBoundary::IBase"); }
unsigned int getClassID() const override { actionUnsupported("ActionPlayerBoundary::getClassID"); }
void setName(const std::string& name) override { actionUnsupported("ActionPlayerBoundary::setName"); }
const std::string& getName() const override { actionUnsupported("ActionPlayerBoundary::getName"); }
void setFlags(const Flag32Bits<IPlayer>& flags) override { actionUnsupported("ActionPlayerBoundary::setFlags"); }
bool testFlags(const Flag32Bits<IPlayer>& flags) const override { actionUnsupported("ActionPlayerBoundary::testFlags"); }
void handleUpdate(float dt, unsigned int) override { actionUnsupported("ActionPlayerBoundary::handleUpdate"); }
void handleFrameUpdate(float dt) override { actionUnsupported("ActionPlayerBoundary::handleFrameUpdate"); }
UpdateFreqencyType getUpdateFrequencyType() const override { actionUnsupported("ActionPlayerBoundary::getUpdateFrequencyType"); }
void setUpdateFrequencyType(UpdateFreqencyType type) override { actionUnsupported("ActionPlayerBoundary::setUpdateFrequencyType"); }
void handleInput(io::PlayerInput& input, float dt, unsigned int) override { actionUnsupported("ActionPlayerBoundary::handleInput"); }
void getAutoInput(io::PlayerInput& out, bool) override { actionUnsupported("ActionPlayerBoundary::getAutoInput"); }
IObject* getVehicle() const override { actionUnsupported("ActionPlayerBoundary::getVehicle"); }
bool setVehicle(IObject* vehicle, int seat) override { actionUnsupported("ActionPlayerBoundary::setVehicle"); }
int getInputId() const override { actionUnsupported("ActionPlayerBoundary::getInputId"); }
void setIsAlive(bool alive) override { actionUnsupported("ActionPlayerBoundary::setIsAlive"); }
bool getIsAlive() const override { actionUnsupported("ActionPlayerBoundary::getIsAlive"); }
void setSuicide(bool) override { actionUnsupported("ActionPlayerBoundary::setSuicide"); }
bool getSuicide() const override { actionUnsupported("ActionPlayerBoundary::getSuicide"); }
bool getIsConnected() const override { actionUnsupported("ActionPlayerBoundary::getIsConnected"); }
void setIsConnected(bool) override { actionUnsupported("ActionPlayerBoundary::setIsConnected"); }
float getCrossHairFadeValue() const override { actionUnsupported("ActionPlayerBoundary::getCrossHairFadeValue"); }
void setViewDistance(float) override { actionUnsupported("ActionPlayerBoundary::setViewDistance"); }
float getViewDistance() const override { actionUnsupported("ActionPlayerBoundary::getViewDistance"); }
void setFov(float fov, float time) override { actionUnsupported("ActionPlayerBoundary::setFov"); }
float getFov() const override { actionUnsupported("ActionPlayerBoundary::getFov"); }
float getEndFov() const override { actionUnsupported("ActionPlayerBoundary::getEndFov"); }
ActionBuffer* getActionBuffer() override { actionUnsupported("ActionPlayerBoundary::getActionBuffer"); }
void* getAIPlayerInput() const override { actionUnsupported("ActionPlayerBoundary::getAIPlayerInput"); }
void setDefaultVehicle(IObject*) override { actionUnsupported("ActionPlayerBoundary::setDefaultVehicle"); }
IObject* getDefaultVehicle() override { actionUnsupported("ActionPlayerBoundary::getDefaultVehicle"); }
IObject* getFreeCamera() const override { actionUnsupported("ActionPlayerBoundary::getFreeCamera"); }
void setFreeCameraEnabled(bool) override { actionUnsupported("ActionPlayerBoundary::setFreeCameraEnabled"); }
bool getFreeCameraEnabled() const override { actionUnsupported("ActionPlayerBoundary::getFreeCameraEnabled"); }
void toggleFreeCameraEnabled() override { actionUnsupported("ActionPlayerBoundary::toggleFreeCameraEnabled"); }
void setOverheadCameraEnabled(bool) override { actionUnsupported("ActionPlayerBoundary::setOverheadCameraEnabled"); }
bool getOverheadCameraEnabled() const override { actionUnsupported("ActionPlayerBoundary::getOverheadCameraEnabled"); }
void setOverheadCameraTopDownView(bool) override { actionUnsupported("ActionPlayerBoundary::setOverheadCameraTopDownView"); }
bool getOverheadCameraTopDownView() const override { actionUnsupported("ActionPlayerBoundary::getOverheadCameraTopDownView"); }
void setCommanderCamOverhead(bool) override { actionUnsupported("ActionPlayerBoundary::setCommanderCamOverhead"); }
bool getCommanderCamOverhead() const override { actionUnsupported("ActionPlayerBoundary::getCommanderCamOverhead"); }
int getTickCount() const override { actionUnsupported("ActionPlayerBoundary::getTickCount"); }
void addTick() override { actionUnsupported("ActionPlayerBoundary::addTick"); }
void subTick() override { actionUnsupported("ActionPlayerBoundary::subTick"); }
void resetTickCounter() override { actionUnsupported("ActionPlayerBoundary::resetTickCounter"); }
float getTimeOutSideWorld() const override { actionUnsupported("ActionPlayerBoundary::getTimeOutSideWorld"); }
void setTeam(int team, bool) override { actionUnsupported("ActionPlayerBoundary::setTeam"); }
unsigned int getSpawnAtTick() const override { actionUnsupported("ActionPlayerBoundary::getSpawnAtTick"); }
void setSpawnAtTick(unsigned int tick) override { actionUnsupported("ActionPlayerBoundary::setSpawnAtTick"); }
float getTimeToSpawn() const override { actionUnsupported("ActionPlayerBoundary::getTimeToSpawn"); }
float getManDownTimeLeft() const override { actionUnsupported("ActionPlayerBoundary::getManDownTimeLeft"); }
void setIsManDown(bool) override { actionUnsupported("ActionPlayerBoundary::setIsManDown"); }
bool getIsManDown() override { actionUnsupported("ActionPlayerBoundary::getIsManDown"); }
unsigned int getManDownAtTick() override { actionUnsupported("ActionPlayerBoundary::getManDownAtTick"); }
void setSquadId(unsigned int id) override { actionUnsupported("ActionPlayerBoundary::setSquadId"); }
void setIsCommander(bool) override { actionUnsupported("ActionPlayerBoundary::setIsCommander"); }
bool getIsCommander() const override { actionUnsupported("ActionPlayerBoundary::getIsCommander"); }
void setUAVIsReady(bool) override { actionUnsupported("ActionPlayerBoundary::setUAVIsReady"); }
bool getUAVIsReady() const override { actionUnsupported("ActionPlayerBoundary::getUAVIsReady"); }
void setSatelliteIsReady(bool) override { actionUnsupported("ActionPlayerBoundary::setSatelliteIsReady"); }
bool getSatelliteIsReady() const override { actionUnsupported("ActionPlayerBoundary::getSatelliteIsReady"); }
void setLastTimeCommander(double t) override { actionUnsupported("ActionPlayerBoundary::setLastTimeCommander"); }
double getLastTimeCommander() const override { actionUnsupported("ActionPlayerBoundary::getLastTimeCommander"); }
void setCommanderReapplyWaitTime(double t) override { actionUnsupported("ActionPlayerBoundary::setCommanderReapplyWaitTime"); }
double getCommanderReapplyWaitTime() const override { actionUnsupported("ActionPlayerBoundary::getCommanderReapplyWaitTime"); }
void setIsSquadLeader(bool) override { actionUnsupported("ActionPlayerBoundary::setIsSquadLeader"); }
bool getIsSquadLeader() const override { actionUnsupported("ActionPlayerBoundary::getIsSquadLeader"); }
bool getIsGassed() override { actionUnsupported("ActionPlayerBoundary::getIsGassed"); }
void setIsInsideCP(bool) override { actionUnsupported("ActionPlayerBoundary::setIsInsideCP"); }
bool getIsInsideCP() override { actionUnsupported("ActionPlayerBoundary::getIsInsideCP"); }
void setHasSimulatedFrame(bool) override { actionUnsupported("ActionPlayerBoundary::setHasSimulatedFrame"); }
bool getHasSimulatedFrame() const override { actionUnsupported("ActionPlayerBoundary::getHasSimulatedFrame"); }
void setSpawnGroup(int group) override { actionUnsupported("ActionPlayerBoundary::setSpawnGroup"); }
int getSpawnGroup() const override { actionUnsupported("ActionPlayerBoundary::getSpawnGroup"); }
void setInterpolationTime(unsigned int ms) override { actionUnsupported("ActionPlayerBoundary::setInterpolationTime"); }
unsigned int getInterpolationTime() const override { actionUnsupported("ActionPlayerBoundary::getInterpolationTime"); }
void setPing(unsigned int ping) override { actionUnsupported("ActionPlayerBoundary::setPing"); }
unsigned int getPing() const override { actionUnsupported("ActionPlayerBoundary::getPing"); }
void setKit(int kit) override { actionUnsupported("ActionPlayerBoundary::setKit"); }
PlayerScoreData* getScoreData() override { actionUnsupported("ActionPlayerBoundary::getScoreData"); }
void setFlagHolder(bool) override { actionUnsupported("ActionPlayerBoundary::setFlagHolder"); }
bool getFlagHolder() const override { actionUnsupported("ActionPlayerBoundary::getFlagHolder"); }
ICameraObject* getCamera() const override { actionUnsupported("ActionPlayerBoundary::getCamera"); }
void setCamera(ICameraObject* camera) override { actionUnsupported("ActionPlayerBoundary::setCamera"); }
bool hasFirstPersonView() const override { actionUnsupported("ActionPlayerBoundary::hasFirstPersonView"); }
double getTimeConnected() const override { actionUnsupported("ActionPlayerBoundary::getTimeConnected"); }
io::INetworkable* getNetworkable() override { actionUnsupported("ActionPlayerBoundary::getNetworkable"); }
bool getStateSync(io::BitStream* stream, IObject* object) override { actionUnsupported("ActionPlayerBoundary::getStateSync"); }
bool setStateSync(io::BitStream* stream, IObject* object, double) override { actionUnsupported("ActionPlayerBoundary::setStateSync"); }
void reset() override { actionUnsupported("ActionPlayerBoundary::reset"); }
void setFiringState(bool) override { actionUnsupported("ActionPlayerBoundary::setFiringState"); }
bool getFiringState() const override { actionUnsupported("ActionPlayerBoundary::getFiringState"); }
void setAltFiringState(bool) override { actionUnsupported("ActionPlayerBoundary::setAltFiringState"); }
bool getAltFiringState() const override { actionUnsupported("ActionPlayerBoundary::getAltFiringState"); }
void setFlareFiringState(bool) override { actionUnsupported("ActionPlayerBoundary::setFlareFiringState"); }
bool getFlareFiringState() const override { actionUnsupported("ActionPlayerBoundary::getFlareFiringState"); }
void setSprintState(bool) override { actionUnsupported("ActionPlayerBoundary::setSprintState"); }
bool getSprintState() const override { actionUnsupported("ActionPlayerBoundary::getSprintState"); }
void setSprintStateLastTick(bool) override { actionUnsupported("ActionPlayerBoundary::setSprintStateLastTick"); }
bool getSprintStateLastTick() const override { actionUnsupported("ActionPlayerBoundary::getSprintStateLastTick"); }
void setReloadState() override { actionUnsupported("ActionPlayerBoundary::setReloadState"); }
bool getAndResetReloadState() override { actionUnsupported("ActionPlayerBoundary::getAndResetReloadState"); }
void updateFiringState(unsigned int, IObject*) override { actionUnsupported("ActionPlayerBoundary::updateFiringState"); }
void updateGhostFiringState() override { actionUnsupported("ActionPlayerBoundary::updateGhostFiringState"); }
void setHitIndicator(bool) override { actionUnsupported("ActionPlayerBoundary::setHitIndicator"); }
bool getHitIndicator() const override { actionUnsupported("ActionPlayerBoundary::getHitIndicator"); }
float updateHitFromDirection(const Vec3& direction) override { actionUnsupported("ActionPlayerBoundary::updateHitFromDirection"); }
float getHitFromDirection() const override { actionUnsupported("ActionPlayerBoundary::getHitFromDirection"); }
void updateCameraShake(float amount) override { actionUnsupported("ActionPlayerBoundary::updateCameraShake"); }
float getCameraShake() const override { actionUnsupported("ActionPlayerBoundary::getCameraShake"); }
unsigned int getSquadInviteMask() const override { actionUnsupported("ActionPlayerBoundary::getSquadInviteMask"); }
void setSquadInviteMask(unsigned int mask) override { actionUnsupported("ActionPlayerBoundary::setSquadInviteMask"); }
bool getIsInvitedToSquad(int squad) const override { actionUnsupported("ActionPlayerBoundary::getIsInvitedToSquad"); }
void setKitTimeStamp(double t) override { actionUnsupported("ActionPlayerBoundary::setKitTimeStamp"); }
double getKitTimeStamp() const override { actionUnsupported("ActionPlayerBoundary::getKitTimeStamp"); }
void setCurrentFiringButton(io::PlayerInputMap button) override { actionUnsupported("ActionPlayerBoundary::setCurrentFiringButton"); }
io::PlayerInputMap getCurrentFiringButton() const override { actionUnsupported("ActionPlayerBoundary::getCurrentFiringButton"); }
void setPersonalDecalTemplate(const IObjectTemplate* decal) override { actionUnsupported("ActionPlayerBoundary::setPersonalDecalTemplate"); }
const IObjectTemplate* getPersonalDecalTemplate() const override { actionUnsupported("ActionPlayerBoundary::getPersonalDecalTemplate"); }
void enableVOHelp(bool) override { actionUnsupported("ActionPlayerBoundary::enableVOHelp"); }
void enableVOHelpMessage(int) override { actionUnsupported("ActionPlayerBoundary::enableVOHelpMessage"); }
void disableVOHelpMessage(int) override { actionUnsupported("ActionPlayerBoundary::disableVOHelpMessage"); }
bool getVOHelpMessagePlayed(int) override { actionUnsupported("ActionPlayerBoundary::getVOHelpMessagePlayed"); }
void doAddTouchItem(IPlayer* other) override { actionUnsupported("ActionPlayerBoundary::doAddTouchItem"); }
void setHealRepairedAtTick(unsigned int tick, bool giving, bool repairing) override { actionUnsupported("ActionPlayerBoundary::setHealRepairedAtTick"); }
unsigned int getHealRepairedAtTick() const override { actionUnsupported("ActionPlayerBoundary::getHealRepairedAtTick"); }
void setGaveAmmoAtTick(unsigned int tick, bool giving) override { actionUnsupported("ActionPlayerBoundary::setGaveAmmoAtTick"); }
unsigned int getGaveAmmoAtTick() const override { actionUnsupported("ActionPlayerBoundary::getGaveAmmoAtTick"); }
void setFailedExitAtTick(unsigned int tick) override { actionUnsupported("ActionPlayerBoundary::setFailedExitAtTick"); }
unsigned int getFailedExitAtTick() const override { actionUnsupported("ActionPlayerBoundary::getFailedExitAtTick"); }
bool getIsGivingAbility() const override { actionUnsupported("ActionPlayerBoundary::getIsGivingAbility"); }
bool getIsRepairing() const override { actionUnsupported("ActionPlayerBoundary::getIsRepairing"); }
bool getIsGivingReplenish() const override { actionUnsupported("ActionPlayerBoundary::getIsGivingReplenish"); }
int getChosenUnlockLevel(int kit, int slot) const override { actionUnsupported("ActionPlayerBoundary::getChosenUnlockLevel"); }
void setChosenUnlockLevel(int kit, int slot, int level) override { actionUnsupported("ActionPlayerBoundary::setChosenUnlockLevel"); }
bool isUnlocked(int kit, int slot, int level) override { actionUnsupported("ActionPlayerBoundary::isUnlocked"); }
int getUnlockLevel(int kit, int slot) override { actionUnsupported("ActionPlayerBoundary::getUnlockLevel"); }
void setUnlocks(std::map<int, int>* unlocks) override { actionUnsupported("ActionPlayerBoundary::setUnlocks"); }
std::map<int, int>* getUnlocks() override { actionUnsupported("ActionPlayerBoundary::getUnlocks"); }
void setPeerAddress(const std::string& address) override { actionUnsupported("ActionPlayerBoundary::setPeerAddress"); }
const std::string& getPeerAddress() const override { actionUnsupported("ActionPlayerBoundary::getPeerAddress"); }
bool getVoipEnabled() override { actionUnsupported("ActionPlayerBoundary::getVoipEnabled"); }
unsigned short getVoipSessionId() override { actionUnsupported("ActionPlayerBoundary::getVoipSessionId"); }
void setVoipSessionId(unsigned short id) override { actionUnsupported("ActionPlayerBoundary::setVoipSessionId"); }
bool getIsMuted() override { actionUnsupported("ActionPlayerBoundary::getIsMuted"); }
bool getIsTalking() override { actionUnsupported("ActionPlayerBoundary::getIsTalking"); }
double getLastTalkTime() override { actionUnsupported("ActionPlayerBoundary::getLastTalkTime"); }
double getStopTalkingThreshold() override { actionUnsupported("ActionPlayerBoundary::getStopTalkingThreshold"); }
void setIsMuted(bool) override { actionUnsupported("ActionPlayerBoundary::setIsMuted"); }
void setIsTalking(bool) override { actionUnsupported("ActionPlayerBoundary::setIsTalking"); }
void setVoipEnabled(bool) override { actionUnsupported("ActionPlayerBoundary::setVoipEnabled"); }
void setVotingStatus(PlayerVotingStatus status) override { actionUnsupported("ActionPlayerBoundary::setVotingStatus"); }
PlayerVotingStatus getVotingStatus() override { actionUnsupported("ActionPlayerBoundary::getVotingStatus"); }
void setParticipateInVoting(bool) override { actionUnsupported("ActionPlayerBoundary::setParticipateInVoting"); }
bool getParticipateInVoting() override { actionUnsupported("ActionPlayerBoundary::getParticipateInVoting"); }
void setTargetOfVote(bool) override { actionUnsupported("ActionPlayerBoundary::setTargetOfVote"); }
bool getTargetOfVote() override { actionUnsupported("ActionPlayerBoundary::getTargetOfVote"); }
int getProfileId() override { actionUnsupported("ActionPlayerBoundary::getProfileId"); }
void setProfileId(int id) override { actionUnsupported("ActionPlayerBoundary::setProfileId"); }
void setBlurryVision(float, float, float) override { actionUnsupported("ActionPlayerBoundary::setBlurryVision"); }
float getBlurValue() const override { actionUnsupported("ActionPlayerBoundary::getBlurValue"); }
bool getIsNearGasCloud() const override { actionUnsupported("ActionPlayerBoundary::getIsNearGasCloud"); }
bool getIsGasMaskOn() const override { actionUnsupported("ActionPlayerBoundary::getIsGasMaskOn"); }
void setIsGasMaskOn(bool, bool) override { actionUnsupported("ActionPlayerBoundary::setIsGasMaskOn"); }
void setGasMaskHudItem(bool) override { actionUnsupported("ActionPlayerBoundary::setGasMaskHudItem"); }
float getPlayerGasMaskChangeTimer() const override { actionUnsupported("ActionPlayerBoundary::getPlayerGasMaskChangeTimer"); }
float getPlayerNightVisionChangeTimer() const override { actionUnsupported("ActionPlayerBoundary::getPlayerNightVisionChangeTimer"); }
bool getIsUnderwater() const override { actionUnsupported("ActionPlayerBoundary::getIsUnderwater"); }
float getNormalizedNightVisionPower() override { actionUnsupported("ActionPlayerBoundary::getNormalizedNightVisionPower"); }
void setNightVision(bool, bool) override { actionUnsupported("ActionPlayerBoundary::setNightVision"); }
void rechargeNightVision() override { actionUnsupported("ActionPlayerBoundary::rechargeNightVision"); }
void setVisualNightVision(bool) override { actionUnsupported("ActionPlayerBoundary::setVisualNightVision"); }
bool getNightVision() const override { actionUnsupported("ActionPlayerBoundary::getNightVision"); }
void disableFlashBangEffect() override { actionUnsupported("ActionPlayerBoundary::disableFlashBangEffect"); }
void disableBlurryVision() override { actionUnsupported("ActionPlayerBoundary::disableBlurryVision"); }
io::PlayerInput getPlayerInput() override { actionUnsupported("ActionPlayerBoundary::getPlayerInput"); }
void resetOnDeath() override { actionUnsupported("ActionPlayerBoundary::resetOnDeath"); }
bool isPlayerInNightMap() const override { actionUnsupported("ActionPlayerBoundary::isPlayerInNightMap"); }
void setTracePlayerId(int id) override { actionUnsupported("ActionPlayerBoundary::setTracePlayerId"); }
int getTracePlayerId() override { actionUnsupported("ActionPlayerBoundary::getTracePlayerId"); }
void allocateFreeCamera() override { actionUnsupported("ActionPlayerBoundary::allocateFreeCamera"); }
bool getIsThisASpam(bool) override { actionUnsupported("ActionPlayerBoundary::getIsThisASpam"); }
void setRadioSpamFlagCount(int) override { actionUnsupported("ActionPlayerBoundary::setRadioSpamFlagCount"); }
int getRadioSpamFlagCount() override { actionUnsupported("ActionPlayerBoundary::getRadioSpamFlagCount"); }
void setRadioSpamBlockedFlagCount(int) override { actionUnsupported("ActionPlayerBoundary::setRadioSpamBlockedFlagCount"); }
int getRadioSpamBlockedFlagCount() override { actionUnsupported("ActionPlayerBoundary::getRadioSpamBlockedFlagCount"); }
void setRadioSpamFilterFlagTimer(float) override { actionUnsupported("ActionPlayerBoundary::setRadioSpamFilterFlagTimer"); }
float getRadioSpamFilterFlagTimer() override { actionUnsupported("ActionPlayerBoundary::getRadioSpamFilterFlagTimer"); }
void resetSpamFilter() override { actionUnsupported("ActionPlayerBoundary::resetSpamFilter"); }
void getCloseMines(std::vector<IObject*>& out) const override { actionUnsupported("ActionPlayerBoundary::getCloseMines"); }
bool getAutoReload() const override { actionUnsupported("ActionPlayerBoundary::getAutoReload"); }
void setAutoReload(bool) override { actionUnsupported("ActionPlayerBoundary::setAutoReload"); }
void updateClientSettings() override { actionUnsupported("ActionPlayerBoundary::updateClientSettings"); }
};
}}}
namespace dice { namespace hfe {
struct ActionLogicBoundary : IGameLogic {
void addRef() override { actionUnsupported("ActionLogicBoundary::addRef"); }
int getRef() override { actionUnsupported("ActionLogicBoundary::getRef"); }
int release() override { actionUnsupported("ActionLogicBoundary::release"); }
void* queryInterface(unsigned int) const override { actionUnsupported("ActionLogicBoundary::queryInterface"); }
bool init() override { actionUnsupported("ActionLogicBoundary::init"); }
bool load(const std::string& level) override { actionUnsupported("ActionLogicBoundary::load"); }
bool unload() override { actionUnsupported("ActionLogicBoundary::unload"); }
bool close() override { actionUnsupported("ActionLogicBoundary::close"); }
void update(float deltaTime) override { actionUnsupported("ActionLogicBoundary::update"); }
void reset() override { actionUnsupported("ActionLogicBoundary::reset"); }
void restartDemoPlayback() override { actionUnsupported("ActionLogicBoundary::restartDemoPlayback"); }
void runNextLevel() override { actionUnsupported("ActionLogicBoundary::runNextLevel"); }
void setNextLevel(int) override { actionUnsupported("ActionLogicBoundary::setNextLevel"); }
void addPreStoredTemplate(unsigned int, unsigned int, unsigned int) override { actionUnsupported("ActionLogicBoundary::addPreStoredTemplate"); }
void createPreStoredObjects() override { actionUnsupported("ActionLogicBoundary::createPreStoredObjects"); }
unsigned int getStartPreStoredNetID() const override { actionUnsupported("ActionLogicBoundary::getStartPreStoredNetID"); }
unsigned int getEndPreStoredNetID() const override { actionUnsupported("ActionLogicBoundary::getEndPreStoredNetID"); }
void setTeamName(int team, const std::string&) override { actionUnsupported("ActionLogicBoundary::setTeamName"); }
const std::string& getTeamName(int team) override { actionUnsupported("ActionLogicBoundary::getTeamName"); }
int getTeamFromName(const std::string&) override { actionUnsupported("ActionLogicBoundary::getTeamFromName"); }
const std::string& getTeamDropVehicle(int team) const override { actionUnsupported("ActionLogicBoundary::getTeamDropVehicle"); }
void setNumDropVehiclesForTeam(int team, int n) override { actionUnsupported("ActionLogicBoundary::setNumDropVehiclesForTeam"); }
int getNumDropVehiclesForTeam(int team) override { actionUnsupported("ActionLogicBoundary::getNumDropVehiclesForTeam"); }
void setDropVehicleDeviationRadius(int, int) override { actionUnsupported("ActionLogicBoundary::setDropVehicleDeviationRadius"); }
int getDropVehicleDeviationRadius() override { actionUnsupported("ActionLogicBoundary::getDropVehicleDeviationRadius"); }
int getDropVehicleDeviationMinRadius() override { actionUnsupported("ActionLogicBoundary::getDropVehicleDeviationMinRadius"); }
void suicide() override { actionUnsupported("ActionLogicBoundary::suicide"); }
bool killPlayer(world::IPlayer*, bool, bool) override { actionUnsupported("ActionLogicBoundary::killPlayer"); }
bool reSpawnPlayer(world::IPlayer*) override { actionUnsupported("ActionLogicBoundary::reSpawnPlayer"); }
bool selectSpawnGroup(world::IPlayer*, int) override { actionUnsupported("ActionLogicBoundary::selectSpawnGroup"); }
bool selectTeam(world::IPlayer*, int) override { actionUnsupported("ActionLogicBoundary::selectTeam"); }
bool selectKitAndUnlockLevel(world::IPlayer*, int, int) override { actionUnsupported("ActionLogicBoundary::selectKitAndUnlockLevel"); }
bool isLevelLoadedXP() override { actionUnsupported("ActionLogicBoundary::isLevelLoadedXP"); }
bool getNoVehiclesMode() override { actionUnsupported("ActionLogicBoundary::getNoVehiclesMode"); }
void setNoVehiclesMode(bool) override { actionUnsupported("ActionLogicBoundary::setNoVehiclesMode"); }
void setTicketState(unsigned int team, int) override { actionUnsupported("ActionLogicBoundary::setTicketState"); }
int getTicketState(unsigned int team) override { actionUnsupported("ActionLogicBoundary::getTicketState"); }
void setDefaultNumberOfTickets(unsigned int, unsigned int) override { actionUnsupported("ActionLogicBoundary::setDefaultNumberOfTickets"); }
unsigned int getDefaultNumberOfTickets(unsigned int) override { actionUnsupported("ActionLogicBoundary::getDefaultNumberOfTickets"); }
void setDefaultNumberOfTicketsEx(unsigned int, unsigned int, unsigned int) override { actionUnsupported("ActionLogicBoundary::setDefaultNumberOfTicketsEx"); }
unsigned int getDefaultNumberOfTicketsEx(unsigned int, unsigned int) override { actionUnsupported("ActionLogicBoundary::getDefaultNumberOfTicketsEx"); }
void setDefaultTimeToNextAIWave(float) override { actionUnsupported("ActionLogicBoundary::setDefaultTimeToNextAIWave"); }
float getDefaultTimeToNextAIWave() override { actionUnsupported("ActionLogicBoundary::getDefaultTimeToNextAIWave"); }
void setTicketLossAtEndPerMin(float) override { actionUnsupported("ActionLogicBoundary::setTicketLossAtEndPerMin"); }
float getTicketLossAtEndPerMin() override { actionUnsupported("ActionLogicBoundary::getTicketLossAtEndPerMin"); }
void setTicketLossPerMin(unsigned int team, float) override { actionUnsupported("ActionLogicBoundary::setTicketLossPerMin"); }
float getTicketLossPerMin(unsigned int team) override { actionUnsupported("ActionLogicBoundary::getTicketLossPerMin"); }
void setMaximumLevelViewDistance(float) override { actionUnsupported("ActionLogicBoundary::setMaximumLevelViewDistance"); }
float getMaximumLevelViewDistance() const override { actionUnsupported("ActionLogicBoundary::getMaximumLevelViewDistance"); }
void setExtraUAVVehicleFlightHeight(float) override { actionUnsupported("ActionLogicBoundary::setExtraUAVVehicleFlightHeight"); }
float getExtraUAVVehicleFlightHeight() const override { actionUnsupported("ActionLogicBoundary::getExtraUAVVehicleFlightHeight"); }
void setFogStartEndAndBase(const Vec4&) override { actionUnsupported("ActionLogicBoundary::setFogStartEndAndBase"); }
const Vec4& getFogStartEndAndBase() const override { actionUnsupported("ActionLogicBoundary::getFogStartEndAndBase"); }
void setDefaultLevel(const std::string&) override { actionUnsupported("ActionLogicBoundary::setDefaultLevel"); }
const std::string& getDefaultLevel() const override { actionUnsupported("ActionLogicBoundary::getDefaultLevel"); }
void refreshTriggerables() override { actionUnsupported("ActionLogicBoundary::refreshTriggerables"); }
void playEffect(Vec3, const world::IObjectTemplate*, Vec3) override { actionUnsupported("ActionLogicBoundary::playEffect"); }
bool giveDamage(world::IObject*, float, world::IPlayer*, std::list<WeakPtr<world::IPlayer> >*, world::IObject*, int, world::IObject*, world::IObject*, Vec3, Vec3, int, bool, bool, bool) override { actionUnsupported("ActionLogicBoundary::giveDamage"); }
void handleExplosion(Vec3, float, int, float, world::IPlayer*, world::IArmorGLComp*, int, world::IObject*, world::IObject*, float, world::IObject*, world::IObject*, float, bool, bool) override { actionUnsupported("ActionLogicBoundary::handleExplosion"); }
void handlePickup(world::IObject*, world::IPlayer*, world::IObject*, bool) override { actionUnsupported("ActionLogicBoundary::handlePickup"); }
void handleDrop(world::IObject*, world::IPlayer*, world::IObject*, bool) override { actionUnsupported("ActionLogicBoundary::handleDrop"); }
bool handleCollisionForProjectile(world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int, world::CollisionFaceInfo*, world::CollisionFaceInfo*) override { actionUnsupported("ActionLogicBoundary::handleCollisionForProjectile"); }
bool handleCollisionForReplenisher(bool, world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int) override { actionUnsupported("ActionLogicBoundary::handleCollisionForReplenisher"); }
bool handleCollisionForResurrecter(world::IObject*, world::IObject*, float, float, const Vec3&, const Vec3&, const Vec3&, int, int) override { actionUnsupported("ActionLogicBoundary::handleCollisionForResurrecter"); }
void handleCollision(world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int, world::CollisionFaceInfo*, world::CollisionFaceInfo*) override { actionUnsupported("ActionLogicBoundary::handleCollision"); }
bool heal(world::IObject*, float, world::IPlayer*, int, int, float*, bool, bool) override { actionUnsupported("ActionLogicBoundary::heal"); }
bool replenishAmmo(world::IObject*, float, world::IPlayer*, int, int, float*, bool) override { actionUnsupported("ActionLogicBoundary::replenishAmmo"); }
void doImpact(int, int, world::MaterialCollisionType, const Vec3&, const Vec3&, const Vec3&, world::IObject*, world::IObject*, world::IProjectileObject*, world::CollisionFaceInfo*, Vec3, bool) override { actionUnsupported("ActionLogicBoundary::doImpact"); }
void playCollisionEffect(Vec3, const world::IObjectTemplate*, Vec3, Vec3) override { actionUnsupported("ActionLogicBoundary::playCollisionEffect"); }
void spawnMultipleObjects(const world::IObjectTemplate*, int) override { actionUnsupported("ActionLogicBoundary::spawnMultipleObjects"); }
world::IObject* spawnObject(const std::string&, Vec3, Vec3, int) override { actionUnsupported("ActionLogicBoundary::spawnObject"); }
world::IObject* spawnObject(const world::IObjectTemplate*, Vec3, Vec3, int) override { actionUnsupported("ActionLogicBoundary::spawnObject"); }
void triggerSpecialGameEvent(SpecialGameEventType, void*) override { actionUnsupported("ActionLogicBoundary::triggerSpecialGameEvent"); }
bool remoteCommand(int, int, int, int, int) override { actionUnsupported("ActionLogicBoundary::remoteCommand"); }
void setUseActiveCombatArea(bool) override { actionUnsupported("ActionLogicBoundary::setUseActiveCombatArea"); }
bool getUseActiveCombatArea() override { actionUnsupported("ActionLogicBoundary::getUseActiveCombatArea"); }
void setActiveCombatArea(float, float, float, float) override { actionUnsupported("ActionLogicBoundary::setActiveCombatArea"); }
void getActiveCombatArea(float*, float*, float*, float*) override { actionUnsupported("ActionLogicBoundary::getActiveCombatArea"); }
float getTimeUntilRestartMap() override { actionUnsupported("ActionLogicBoundary::getTimeUntilRestartMap"); }
void setKit(int, int, const std::string&, const std::string&) override { actionUnsupported("ActionLogicBoundary::setKit"); }
world::IObjectTemplate* getKitTemplate(int, int) override { actionUnsupported("ActionLogicBoundary::getKitTemplate"); }
std::string getKitTemplateString(int, int) override { actionUnsupported("ActionLogicBoundary::getKitTemplateString"); }
std::string getSoldierSkin(int, int) override { actionUnsupported("ActionLogicBoundary::getSoldierSkin"); }
void* getKits() const override { actionUnsupported("ActionLogicBoundary::getKits"); }
void checkPlayerTriggers(world::IPlayer*, world::IObject*, io::PlayerInput, bool) override { actionUnsupported("ActionLogicBoundary::checkPlayerTriggers"); }
int getGameStatus() override { actionUnsupported("ActionLogicBoundary::getGameStatus"); }
BeginRoundState getBeginRoundState() override { actionUnsupported("ActionLogicBoundary::getBeginRoundState"); }
void setBeginRoundState(BeginRoundState) override { actionUnsupported("ActionLogicBoundary::setBeginRoundState"); }
unsigned int getNextStateChangeAtTick() override { actionUnsupported("ActionLogicBoundary::getNextStateChangeAtTick"); }
void setNextStateChangeAtTick(unsigned int) override { actionUnsupported("ActionLogicBoundary::setNextStateChangeAtTick"); }
float getTimeToNextStateChange() override { actionUnsupported("ActionLogicBoundary::getTimeToNextStateChange"); }
void addControlPoint(world::IObject*, int) override { actionUnsupported("ActionLogicBoundary::addControlPoint"); }
std::map<unsigned int, world::IObject*>* getControlPointMap() const override { actionUnsupported("ActionLogicBoundary::getControlPointMap"); }
world::IObject* getControlPoint(unsigned int) override { actionUnsupported("ActionLogicBoundary::getControlPoint"); }
void removeControlPoint(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeControlPoint"); }
bool getControlPointsInGroup(std::vector<world::IObject*>&, int group) override { actionUnsupported("ActionLogicBoundary::getControlPointsInGroup"); }
int getNumberOfControlPointGroups() override { actionUnsupported("ActionLogicBoundary::getNumberOfControlPointGroups"); }
void addFlag(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addFlag"); }
void* getFlagMap() const override { actionUnsupported("ActionLogicBoundary::getFlagMap"); }
void removeFlag(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeFlag"); }
void addPco(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addPco"); }
std::map<unsigned int, world::IObject*>* getPcoMap() const override { actionUnsupported("ActionLogicBoundary::getPcoMap"); }
void removePco(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removePco"); }
void addStrategicObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addStrategicObject"); }
std::map<std::string, world::StrategicObjectHolder*>* getStrategicObjectMap() override { actionUnsupported("ActionLogicBoundary::getStrategicObjectMap"); }
void removeStrategicObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeStrategicObject"); }
void addTargetObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addTargetObject"); }
void* getTargetObjectMap() const override { actionUnsupported("ActionLogicBoundary::getTargetObjectMap"); }
void removeTargetObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeTargetObject"); }
world::IObject* getTargetObject(unsigned int) const override { actionUnsupported("ActionLogicBoundary::getTargetObject"); }
world::IObject* findBestTargetObject(const Vec3&, const Vec3&, world::TargetType, float, float, world::IObject*, world::IObject*, float, const world::IObject*, bool, bool, bool) override { actionUnsupported("ActionLogicBoundary::findBestTargetObject"); }
void addRemoteControlledObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addRemoteControlledObject"); }
void* getRemoteControlledObjectMap() const override { actionUnsupported("ActionLogicBoundary::getRemoteControlledObjectMap"); }
void removeRemoteControlledObject(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeRemoteControlledObject"); }
world::IObject* getRemoteControlledObject(unsigned int) const override { actionUnsupported("ActionLogicBoundary::getRemoteControlledObject"); }
void addProjectile(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addProjectile"); }
void* getProjectileMap() const override { actionUnsupported("ActionLogicBoundary::getProjectileMap"); }
void removeProjectile(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeProjectile"); }
void addObjectSpawner(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addObjectSpawner"); }
void* getObjectSpawnerMap() const override { actionUnsupported("ActionLogicBoundary::getObjectSpawnerMap"); }
void removeObjectSpawner(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeObjectSpawner"); }
world::IObject* getObjectSpawner(unsigned int) const override { actionUnsupported("ActionLogicBoundary::getObjectSpawner"); }
void addSupplyDepot(world::IObject*) override { actionUnsupported("ActionLogicBoundary::addSupplyDepot"); }
void* getSupplyDepotMap() const override { actionUnsupported("ActionLogicBoundary::getSupplyDepotMap"); }
void removeSupplyDepot(world::IObject*) override { actionUnsupported("ActionLogicBoundary::removeSupplyDepot"); }
void setTeamLanguage(int, const std::string&) override { actionUnsupported("ActionLogicBoundary::setTeamLanguage"); }
const std::string& getTeamLanguage(int) const override { actionUnsupported("ActionLogicBoundary::getTeamLanguage"); }
void setTeamFlag(int, const std::string&) override { actionUnsupported("ActionLogicBoundary::setTeamFlag"); }
const std::string& getTeamFlag(int) const override { actionUnsupported("ActionLogicBoundary::getTeamFlag"); }
void toggleLocalFreeCamera() override { actionUnsupported("ActionLogicBoundary::toggleLocalFreeCamera"); }
Vec3 getBeforeSpawnCameraPosition() const override { actionUnsupported("ActionLogicBoundary::getBeforeSpawnCameraPosition"); }
Vec3 getBeforeSpawnCameraDirection() const override { actionUnsupported("ActionLogicBoundary::getBeforeSpawnCameraDirection"); }
void resetPlayerFreeCameraPosition(world::IPlayer*) override { actionUnsupported("ActionLogicBoundary::resetPlayerFreeCameraPosition"); }
bool isLevelLoaded() const override { actionUnsupported("ActionLogicBoundary::isLevelLoaded"); }
bool isPrecaching() const override { actionUnsupported("ActionLogicBoundary::isPrecaching"); }
bool getEnableVoHelp() override { actionUnsupported("ActionLogicBoundary::getEnableVoHelp"); }
void stopPlayingVoMessages() override { actionUnsupported("ActionLogicBoundary::stopPlayingVoMessages"); }
void setSupplyDropHeight(float) override { actionUnsupported("ActionLogicBoundary::setSupplyDropHeight"); }
float getSupplyDropHeight() override { actionUnsupported("ActionLogicBoundary::getSupplyDropHeight"); }
void setVehicleDropHeight(float) override { actionUnsupported("ActionLogicBoundary::setVehicleDropHeight"); }
float getVehicleDropHeight() override { actionUnsupported("ActionLogicBoundary::getVehicleDropHeight"); }
void setSupplyDropNumSecsToLive(float) override { actionUnsupported("ActionLogicBoundary::setSupplyDropNumSecsToLive"); }
float getSupplyDropNumSecsToLive() override { actionUnsupported("ActionLogicBoundary::getSupplyDropNumSecsToLive"); }
void setFeslEnvironment(int) override { actionUnsupported("ActionLogicBoundary::setFeslEnvironment"); }
int getFeslEnvironment() override { actionUnsupported("ActionLogicBoundary::getFeslEnvironment"); }
void storeVoipServerInfo(bool, unsigned long, unsigned short, unsigned short, int) override { actionUnsupported("ActionLogicBoundary::storeVoipServerInfo"); }
void getVoipServerInfo(bool&, unsigned int&, unsigned short&, unsigned short&, int&) override { actionUnsupported("ActionLogicBoundary::getVoipServerInfo"); }
void startupVoipClient() override { actionUnsupported("ActionLogicBoundary::startupVoipClient"); }
bool isVoipClientRunning() override { actionUnsupported("ActionLogicBoundary::isVoipClientRunning"); }
bool getAirMouseInverted() override { actionUnsupported("ActionLogicBoundary::getAirMouseInverted"); }
void setAirMouseSensitivity(float) override { actionUnsupported("ActionLogicBoundary::setAirMouseSensitivity"); }
float getAirMouseSensitivity() override { actionUnsupported("ActionLogicBoundary::getAirMouseSensitivity"); }
void becomeSquadLeader(bool) override { actionUnsupported("ActionLogicBoundary::becomeSquadLeader"); }
TicketHistory& getTicketHistory() override { actionUnsupported("ActionLogicBoundary::getTicketHistory"); }
void setTicketHistory(const TicketHistory&) override { actionUnsupported("ActionLogicBoundary::setTicketHistory"); }
void setHitIndicationThreshold(float) override { actionUnsupported("ActionLogicBoundary::setHitIndicationThreshold"); }
float getHitIndicationThreshold() override { actionUnsupported("ActionLogicBoundary::getHitIndicationThreshold"); }
float getUAVActiveTime() const override { actionUnsupported("ActionLogicBoundary::getUAVActiveTime"); }
float getSatelliteScanActiveTime() const override { actionUnsupported("ActionLogicBoundary::getSatelliteScanActiveTime"); }
float getUAVReloadTime() const override { actionUnsupported("ActionLogicBoundary::getUAVReloadTime"); }
float getUAVRadius() const override { actionUnsupported("ActionLogicBoundary::getUAVRadius"); }
float getSatelliteScanReloadTime() const override { actionUnsupported("ActionLogicBoundary::getSatelliteScanReloadTime"); }
float getSatelliteScanVisibleTime() const override { actionUnsupported("ActionLogicBoundary::getSatelliteScanVisibleTime"); }
world::IObject* getGroundDecalObject() override { actionUnsupported("ActionLogicBoundary::getGroundDecalObject"); }
void noVehiclesOmit(std::string) override { actionUnsupported("ActionLogicBoundary::noVehiclesOmit"); }
bool isOmitted(std::string) override { actionUnsupported("ActionLogicBoundary::isOmitted"); }
void AddInvalidDropVehicleObject(std::string, bool) override { actionUnsupported("ActionLogicBoundary::AddInvalidDropVehicleObject"); }
std::list<std::pair<std::string, bool> >* GetInvalidVehicleDropObjectList() override { actionUnsupported("ActionLogicBoundary::GetInvalidVehicleDropObjectList"); }
void setDebugMissiles(int) override { actionUnsupported("ActionLogicBoundary::setDebugMissiles"); }
int getDebugMissiles() override { actionUnsupported("ActionLogicBoundary::getDebugMissiles"); }
void setDebugReplenish(int) override { actionUnsupported("ActionLogicBoundary::setDebugReplenish"); }
int getDebugReplenish() override { actionUnsupported("ActionLogicBoundary::getDebugReplenish"); }
void setTeamDropVehicle(int, const std::string&) override { actionUnsupported("ActionLogicBoundary::setTeamDropVehicle"); }
void ClearDropVehicles() override { actionUnsupported("ActionLogicBoundary::ClearDropVehicles"); }
void handleEvent(EventCategory, unsigned int, EventNode*, void*) override { actionUnsupported("ActionLogicBoundary::handleEvent"); }
void killAllPlayers() override { actionUnsupported("ActionLogicBoundary::killAllPlayers"); }
bool preCacheSpawnedTemplates() override { actionUnsupported("ActionLogicBoundary::preCacheSpawnedTemplates"); }
void clearConsole() override { actionUnsupported("ActionLogicBoundary::clearConsole"); }
bool loadUpdateComponents() override { actionUnsupported("ActionLogicBoundary::loadUpdateComponents"); }
bool loadInitFile(const std::string&) override { actionUnsupported("ActionLogicBoundary::loadInitFile"); }
bool loadAllConFiles(const std::string&) override { actionUnsupported("ActionLogicBoundary::loadAllConFiles"); }
void loadVoiceMessages() override { actionUnsupported("ActionLogicBoundary::loadVoiceMessages"); }
bool setCreatedOnLoadFlag() override { actionUnsupported("ActionLogicBoundary::setCreatedOnLoadFlag"); }
void createSunTemplate() override { actionUnsupported("ActionLogicBoundary::createSunTemplate"); }
void resetAllPlayers() override { actionUnsupported("ActionLogicBoundary::resetAllPlayers"); }
void resetAllObjects() override { actionUnsupported("ActionLogicBoundary::resetAllObjects"); }
};
}}
namespace dice { namespace hfe {
struct ActionGameBoundary : IGame {
void addRef() override { actionUnsupported("ActionGameBoundary::addRef"); }
int getRef() override { actionUnsupported("ActionGameBoundary::getRef"); }
int release() override { actionUnsupported("ActionGameBoundary::release"); }
void* queryInterface(unsigned int) const override { actionUnsupported("ActionGameBoundary::queryInterface"); }
bool init() override { actionUnsupported("ActionGameBoundary::init"); }
void shutdown() override { actionUnsupported("ActionGameBoundary::shutdown"); }
bool load() override { actionUnsupported("ActionGameBoundary::load"); }
bool loadPath(const std::string&) override { actionUnsupported("ActionGameBoundary::loadPath"); }
bool unload() override { actionUnsupported("ActionGameBoundary::unload"); }
StatsMonitor* getStatsMonitor() const override { actionUnsupported("ActionGameBoundary::getStatsMonitor"); }
bool update(int steps, float dt) override { actionUnsupported("ActionGameBoundary::update"); }
bool say(const std::string&) override { actionUnsupported("ActionGameBoundary::say"); }
TargetChannel getCurrentChannel() override { actionUnsupported("ActionGameBoundary::getCurrentChannel"); }
void setCurrentChannel(TargetChannel) override { actionUnsupported("ActionGameBoundary::setCurrentChannel"); }
int getCurrentFlags() override { actionUnsupported("ActionGameBoundary::getCurrentFlags"); }
void setCurrentFlags(int) override { actionUnsupported("ActionGameBoundary::setCurrentFlags"); }
ChannelMessageType getCurrentType() override { actionUnsupported("ActionGameBoundary::getCurrentType"); }
void setCurrentType(ChannelMessageType) override { actionUnsupported("ActionGameBoundary::setCurrentType"); }
MapInfo getMapInfo() const override { actionUnsupported("ActionGameBoundary::getMapInfo"); }
bool radioMessageFromPlayer(TargetChannel, const std::string&, unsigned int, unsigned int, bool, bool) override { actionUnsupported("ActionGameBoundary::radioMessageFromPlayer"); }
bool localRadioMessage(TargetChannel, const std::string&) override { actionUnsupported("ActionGameBoundary::localRadioMessage"); }
bool gameInfoMessage(TargetChannel, const std::string&, unsigned int, unsigned int, bool) override { actionUnsupported("ActionGameBoundary::gameInfoMessage"); }
bool localGameInfoMessage(TargetChannel, const std::string&) override { actionUnsupported("ActionGameBoundary::localGameInfoMessage"); }
void addPlayerInput(const io::PlayerInput&) override { actionUnsupported("ActionGameBoundary::addPlayerInput"); }
int getCurrentInputIndex() override { actionUnsupported("ActionGameBoundary::getCurrentInputIndex"); }
void confirmGhost(int, unsigned int) override { actionUnsupported("ActionGameBoundary::confirmGhost"); }
void confirmReset(int) override { actionUnsupported("ActionGameBoundary::confirmReset"); }
world::IObject* createObjectOnAllClients(const world::IObjectTemplate*, Math::Vec3, Math::Vec3, bool, int, int) override { actionUnsupported("ActionGameBoundary::createObjectOnAllClients"); }
world::IObject* getObjectFromNetworkableId(const unsigned short&) const override { actionUnsupported("ActionGameBoundary::getObjectFromNetworkableId"); }
void createAndDestroyObject(const world::IObjectTemplate*) override { actionUnsupported("ActionGameBoundary::createAndDestroyObject"); }
void sendGameEventToAllPlaying(GameEvent*) override { actionUnsupported("ActionGameBoundary::sendGameEventToAllPlaying"); }
void sendGameEventIfPlaying(unsigned int, GameEvent*) override { actionUnsupported("ActionGameBoundary::sendGameEventIfPlaying"); }
void sendGameEventToAll(GameEvent*) override { actionUnsupported("ActionGameBoundary::sendGameEventToAll"); }
void sendGameEventToAllConnected(GameEvent*) override { actionUnsupported("ActionGameBoundary::sendGameEventToAllConnected"); }
void sendGameEventIfConnected(unsigned int, GameEvent*) override { actionUnsupported("ActionGameBoundary::sendGameEventIfConnected"); }
void sendMissileMessageToOccupyingPlayers(world::IObject*, world::MissileLockMessage) override { actionUnsupported("ActionGameBoundary::sendMissileMessageToOccupyingPlayers"); }
bool destroyObject(world::IObject*) override { actionUnsupported("ActionGameBoundary::destroyObject"); }
bool enterVehicle(world::IPlayer*, world::IObject*, bool) override { actionUnsupported("ActionGameBoundary::enterVehicle"); }
bool exitVehicle(world::IPlayer*, bool, bool) override { actionUnsupported("ActionGameBoundary::exitVehicle"); }
std::string listPlayers() const override { actionUnsupported("ActionGameBoundary::listPlayers"); }
void joinSquad(int) override { actionUnsupported("ActionGameBoundary::joinSquad"); }
void leaveSquad(int) override { actionUnsupported("ActionGameBoundary::leaveSquad"); }
std::string listSquadMembers(int, int) override { actionUnsupported("ActionGameBoundary::listSquadMembers"); }
std::string listSquads(int) override { actionUnsupported("ActionGameBoundary::listSquads"); }
void changeSquadName(const std::string&, bool) override { actionUnsupported("ActionGameBoundary::changeSquadName"); }
void makeMeCommander() override { actionUnsupported("ActionGameBoundary::makeMeCommander"); }
void popNextCommanderApplicant(int) override { actionUnsupported("ActionGameBoundary::popNextCommanderApplicant"); }
std::string getSpottedMessage(unsigned short) override { actionUnsupported("ActionGameBoundary::getSpottedMessage"); }
void sendSpottedMessage(world::IPlayer*, unsigned short, Vec2, world::VehicleType) override { actionUnsupported("ActionGameBoundary::sendSpottedMessage"); }
void callForSpecialist(world::IPlayer*, HudNodeTypes) override { actionUnsupported("ActionGameBoundary::callForSpecialist"); }
void recordDemo(DemoRecorder*) override { actionUnsupported("ActionGameBoundary::recordDemo"); }
void stopRecording() override { actionUnsupported("ActionGameBoundary::stopRecording"); }
bool isPlayingDemo() override { actionUnsupported("ActionGameBoundary::isPlayingDemo"); }
void setSimulationRate(int) override { actionUnsupported("ActionGameBoundary::setSimulationRate"); }
int getSimulationRate() override { actionUnsupported("ActionGameBoundary::getSimulationRate"); }
void debugShowActiveGhosts(int) override { actionUnsupported("ActionGameBoundary::debugShowActiveGhosts"); }
const std::string& getCurrentMapName() const override { actionUnsupported("ActionGameBoundary::getCurrentMapName"); }
const std::string& getCurrentMapGameMode() const override { actionUnsupported("ActionGameBoundary::getCurrentMapGameMode"); }
const std::string& getCurrentMapLevelPath() const override { actionUnsupported("ActionGameBoundary::getCurrentMapLevelPath"); }
int getCurrentMapMaxPlayers() const override { actionUnsupported("ActionGameBoundary::getCurrentMapMaxPlayers"); }
int getCurrentMapSize() const override { actionUnsupported("ActionGameBoundary::getCurrentMapSize"); }
void setCurrentMapSize(unsigned int) override { actionUnsupported("ActionGameBoundary::setCurrentMapSize"); }
const io::BasicNetworkStats* getNetworkStats() const override { actionUnsupported("ActionGameBoundary::getNetworkStats"); }
void sendUDPPacket(const char*, unsigned short, void*, int) override { actionUnsupported("ActionGameBoundary::sendUDPPacket"); }
void addTextToChatWindow(const char*) override { actionUnsupported("ActionGameBoundary::addTextToChatWindow"); }
void failedExitVehicle(int) override { actionUnsupported("ActionGameBoundary::failedExitVehicle"); }
bool isPunkBusterEnabled() const override { actionUnsupported("ActionGameBoundary::isPunkBusterEnabled"); }
bool getAbortLoadLevel() const override { actionUnsupported("ActionGameBoundary::getAbortLoadLevel"); }
};
}}
namespace dice { namespace hfe {
struct ActionSquadBoundary : ISquadManager {
void addRef() override { actionUnsupported("ActionSquadBoundary::IBase"); }
int getRef() override { actionUnsupported("ActionSquadBoundary::IBase"); }
int release() override { actionUnsupported("ActionSquadBoundary::IBase"); }
void* queryInterface(unsigned int) const override { actionUnsupported("ActionSquadBoundary::IBase"); }
void reset() override { actionUnsupported("ActionSquadBoundary::reset"); }
void reAddDynamicSpawnGroups() override { actionUnsupported("ActionSquadBoundary::reAddDynamicSpawnGroups"); }
void sendSquadManagerState(int) override { actionUnsupported("ActionSquadBoundary::sendSquadManagerState"); }
void setIsPrivate(int, int, bool, bool) override { actionUnsupported("ActionSquadBoundary::setIsPrivate"); }
bool setIsAccepted(int, int, bool, OrderType, SquadOrderType, bool) override { actionUnsupported("ActionSquadBoundary::setIsAccepted"); }
ISquadInfo* getSquadInfo(int, int) override { actionUnsupported("ActionSquadBoundary::getSquadInfo"); }
bool addToSquad(int, int, int) override { actionUnsupported("ActionSquadBoundary::addToSquad"); }
void setSquadLeader(int, int, int, bool) override { actionUnsupported("ActionSquadBoundary::setSquadLeader"); }
int getFirstUnusedSquad(int) const override { actionUnsupported("ActionSquadBoundary::getFirstUnusedSquad"); }
void issueSquadOrder(int, int, SquadOrder&, bool) override { actionUnsupported("ActionSquadBoundary::issueSquadOrder"); }
void issueCommanderOrder(int, int, SquadOrder&, bool) override { actionUnsupported("ActionSquadBoundary::issueCommanderOrder"); }
void issueRequest(int, int, SquadOrder&, bool) override { actionUnsupported("ActionSquadBoundary::issueRequest"); }
void resetSquad(int, int) override { actionUnsupported("ActionSquadBoundary::resetSquad"); }
void findNewSquadLeader(int, int, int) override { actionUnsupported("ActionSquadBoundary::findNewSquadLeader"); }
void applyForCommander(int, int) override { actionUnsupported("ActionSquadBoundary::applyForCommander"); }
int popNextCommanderApplicant(int) override { actionUnsupported("ActionSquadBoundary::popNextCommanderApplicant"); }
void acceptCommander(int, int) override { actionUnsupported("ActionSquadBoundary::acceptCommander"); }
void declineCommander(int, int) override { actionUnsupported("ActionSquadBoundary::declineCommander"); }
void resignCommander(int) override { actionUnsupported("ActionSquadBoundary::resignCommander"); }
void setCommander(int, int) override { actionUnsupported("ActionSquadBoundary::setCommander"); }
void* getCommanderCam(int) override { actionUnsupported("ActionSquadBoundary::getCommanderCam"); }
void setCommanderToSoldierMode(int) override { actionUnsupported("ActionSquadBoundary::setCommanderToSoldierMode"); }
void setCommanderToOverheadCamMode(int) override { actionUnsupported("ActionSquadBoundary::setCommanderToOverheadCamMode"); }
void setCommanderOverheadCamZoomMode(int, bool) override { actionUnsupported("ActionSquadBoundary::setCommanderOverheadCamZoomMode"); }
void removeCommanderApplicant(int, int) override { actionUnsupported("ActionSquadBoundary::removeCommanderApplicant"); }
void addOrderMarker(int, SquadOrder&) override { actionUnsupported("ActionSquadBoundary::addOrderMarker"); }
void removeAllOrderMarkers() override { actionUnsupported("ActionSquadBoundary::removeAllOrderMarkers"); }
void removeOrderMarker(int) override { actionUnsupported("ActionSquadBoundary::removeOrderMarker"); }
void updateOrderMarkers(world::IPlayer*) override { actionUnsupported("ActionSquadBoundary::updateOrderMarkers"); }
};
}}
