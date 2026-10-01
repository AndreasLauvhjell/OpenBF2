// [TEMP] Generated from the real IPlayer/IGameLogic declarations.
// Only the specifically overridden observer methods are callable.
// Every unrelated call emits a diagnostic and aborts; no defaults.
namespace dice { namespace hfe { namespace world {
struct PlayerBoundary : IPlayer {
void addRef() override { fixtureUnsupported("addRef"); }
int getRef() override { fixtureUnsupported("getRef"); }
int release() override { fixtureUnsupported("release"); }
void* queryInterface(unsigned int) const override { fixtureUnsupported("queryInterface"); }
unsigned int getClassID() const override { fixtureUnsupported("PlayerBoundary::getClassID"); }
void setName(const std::string& name) override { fixtureUnsupported("PlayerBoundary::setName"); }
const std::string& getName() const override { fixtureUnsupported("PlayerBoundary::getName"); }
void setFlags(const Flag32Bits<IPlayer>& flags) override { fixtureUnsupported("PlayerBoundary::setFlags"); }
bool testFlags(const Flag32Bits<IPlayer>& flags) const override { fixtureUnsupported("PlayerBoundary::testFlags"); }
void handleUpdate(float dt, unsigned int) override { fixtureUnsupported("PlayerBoundary::handleUpdate"); }
void handleFrameUpdate(float dt) override { fixtureUnsupported("PlayerBoundary::handleFrameUpdate"); }
UpdateFreqencyType getUpdateFrequencyType() const override { fixtureUnsupported("PlayerBoundary::getUpdateFrequencyType"); }
void setUpdateFrequencyType(UpdateFreqencyType type) override { fixtureUnsupported("PlayerBoundary::setUpdateFrequencyType"); }
void handleInput(io::PlayerInput& input, float dt, unsigned int) override { fixtureUnsupported("PlayerBoundary::handleInput"); }
void getAutoInput(io::PlayerInput& out, bool) override { fixtureUnsupported("PlayerBoundary::getAutoInput"); }
IObject* getVehicle() const override { fixtureUnsupported("PlayerBoundary::getVehicle"); }
bool setVehicle(IObject* vehicle, int seat) override { fixtureUnsupported("PlayerBoundary::setVehicle"); }
int getInputId() const override { fixtureUnsupported("PlayerBoundary::getInputId"); }
int getId() const override { fixtureUnsupported("PlayerBoundary::getId"); }
void setIsAlive(bool alive) override { fixtureUnsupported("PlayerBoundary::setIsAlive"); }
void setSuicide(bool) override { fixtureUnsupported("PlayerBoundary::setSuicide"); }
bool getSuicide() const override { fixtureUnsupported("PlayerBoundary::getSuicide"); }
bool getIsConnected() const override { fixtureUnsupported("PlayerBoundary::getIsConnected"); }
void setIsConnected(bool) override { fixtureUnsupported("PlayerBoundary::setIsConnected"); }
float getCrossHairFadeValue() const override { fixtureUnsupported("PlayerBoundary::getCrossHairFadeValue"); }
void setViewDistance(float) override { fixtureUnsupported("PlayerBoundary::setViewDistance"); }
float getViewDistance() const override { fixtureUnsupported("PlayerBoundary::getViewDistance"); }
void setFov(float fov, float time) override { fixtureUnsupported("PlayerBoundary::setFov"); }
float getFov() const override { fixtureUnsupported("PlayerBoundary::getFov"); }
float getEndFov() const override { fixtureUnsupported("PlayerBoundary::getEndFov"); }
ActionBuffer* getActionBuffer() override { fixtureUnsupported("PlayerBoundary::getActionBuffer"); }
void* getAIPlayerInput() const override { fixtureUnsupported("PlayerBoundary::getAIPlayerInput"); }
void setDefaultVehicle(IObject*) override { fixtureUnsupported("PlayerBoundary::setDefaultVehicle"); }
IObject* getDefaultVehicle() override { fixtureUnsupported("PlayerBoundary::getDefaultVehicle"); }
IObject* getFreeCamera() const override { fixtureUnsupported("PlayerBoundary::getFreeCamera"); }
void setFreeCameraEnabled(bool) override { fixtureUnsupported("PlayerBoundary::setFreeCameraEnabled"); }
bool getFreeCameraEnabled() const override { fixtureUnsupported("PlayerBoundary::getFreeCameraEnabled"); }
void toggleFreeCameraEnabled() override { fixtureUnsupported("PlayerBoundary::toggleFreeCameraEnabled"); }
void setOverheadCameraEnabled(bool) override { fixtureUnsupported("PlayerBoundary::setOverheadCameraEnabled"); }
bool getOverheadCameraEnabled() const override { fixtureUnsupported("PlayerBoundary::getOverheadCameraEnabled"); }
void setOverheadCameraTopDownView(bool) override { fixtureUnsupported("PlayerBoundary::setOverheadCameraTopDownView"); }
bool getOverheadCameraTopDownView() const override { fixtureUnsupported("PlayerBoundary::getOverheadCameraTopDownView"); }
void setCommanderCamOverhead(bool) override { fixtureUnsupported("PlayerBoundary::setCommanderCamOverhead"); }
bool getCommanderCamOverhead() const override { fixtureUnsupported("PlayerBoundary::getCommanderCamOverhead"); }
int getTickCount() const override { fixtureUnsupported("PlayerBoundary::getTickCount"); }
void addTick() override { fixtureUnsupported("PlayerBoundary::addTick"); }
void subTick() override { fixtureUnsupported("PlayerBoundary::subTick"); }
void resetTickCounter() override { fixtureUnsupported("PlayerBoundary::resetTickCounter"); }
float getTimeOutSideWorld() const override { fixtureUnsupported("PlayerBoundary::getTimeOutSideWorld"); }
void setTeam(int team, bool) override { fixtureUnsupported("PlayerBoundary::setTeam"); }
unsigned int getSpawnAtTick() const override { fixtureUnsupported("PlayerBoundary::getSpawnAtTick"); }
void setSpawnAtTick(unsigned int tick) override { fixtureUnsupported("PlayerBoundary::setSpawnAtTick"); }
float getTimeToSpawn() const override { fixtureUnsupported("PlayerBoundary::getTimeToSpawn"); }
float getManDownTimeLeft() const override { fixtureUnsupported("PlayerBoundary::getManDownTimeLeft"); }
void setIsManDown(bool) override { fixtureUnsupported("PlayerBoundary::setIsManDown"); }
bool getIsManDown() override { fixtureUnsupported("PlayerBoundary::getIsManDown"); }
unsigned int getManDownAtTick() override { fixtureUnsupported("PlayerBoundary::getManDownAtTick"); }
void setSquadId(unsigned int id) override { fixtureUnsupported("PlayerBoundary::setSquadId"); }
void setIsCommander(bool) override { fixtureUnsupported("PlayerBoundary::setIsCommander"); }
bool getIsCommander() const override { fixtureUnsupported("PlayerBoundary::getIsCommander"); }
void setUAVIsReady(bool) override { fixtureUnsupported("PlayerBoundary::setUAVIsReady"); }
bool getUAVIsReady() const override { fixtureUnsupported("PlayerBoundary::getUAVIsReady"); }
void setSatelliteIsReady(bool) override { fixtureUnsupported("PlayerBoundary::setSatelliteIsReady"); }
bool getSatelliteIsReady() const override { fixtureUnsupported("PlayerBoundary::getSatelliteIsReady"); }
void setLastTimeCommander(double t) override { fixtureUnsupported("PlayerBoundary::setLastTimeCommander"); }
double getLastTimeCommander() const override { fixtureUnsupported("PlayerBoundary::getLastTimeCommander"); }
void setCommanderReapplyWaitTime(double t) override { fixtureUnsupported("PlayerBoundary::setCommanderReapplyWaitTime"); }
double getCommanderReapplyWaitTime() const override { fixtureUnsupported("PlayerBoundary::getCommanderReapplyWaitTime"); }
void setIsSquadLeader(bool) override { fixtureUnsupported("PlayerBoundary::setIsSquadLeader"); }
bool getIsGassed() override { fixtureUnsupported("PlayerBoundary::getIsGassed"); }
void setIsInsideCP(bool) override { fixtureUnsupported("PlayerBoundary::setIsInsideCP"); }
bool getIsInsideCP() override { fixtureUnsupported("PlayerBoundary::getIsInsideCP"); }
void setHasSimulatedFrame(bool) override { fixtureUnsupported("PlayerBoundary::setHasSimulatedFrame"); }
bool getHasSimulatedFrame() const override { fixtureUnsupported("PlayerBoundary::getHasSimulatedFrame"); }
void setSpawnGroup(int group) override { fixtureUnsupported("PlayerBoundary::setSpawnGroup"); }
int getSpawnGroup() const override { fixtureUnsupported("PlayerBoundary::getSpawnGroup"); }
void setInterpolationTime(unsigned int ms) override { fixtureUnsupported("PlayerBoundary::setInterpolationTime"); }
unsigned int getInterpolationTime() const override { fixtureUnsupported("PlayerBoundary::getInterpolationTime"); }
void setPing(unsigned int ping) override { fixtureUnsupported("PlayerBoundary::setPing"); }
unsigned int getPing() const override { fixtureUnsupported("PlayerBoundary::getPing"); }
void setKit(int kit) override { fixtureUnsupported("PlayerBoundary::setKit"); }
PlayerScoreData* getScoreData() override { fixtureUnsupported("PlayerBoundary::getScoreData"); }
void setFlagHolder(bool) override { fixtureUnsupported("PlayerBoundary::setFlagHolder"); }
bool getFlagHolder() const override { fixtureUnsupported("PlayerBoundary::getFlagHolder"); }
ICameraObject* getCamera() const override { fixtureUnsupported("PlayerBoundary::getCamera"); }
void setCamera(ICameraObject* camera) override { fixtureUnsupported("PlayerBoundary::setCamera"); }
bool hasFirstPersonView() const override { fixtureUnsupported("PlayerBoundary::hasFirstPersonView"); }
double getTimeConnected() const override { fixtureUnsupported("PlayerBoundary::getTimeConnected"); }
io::INetworkable* getNetworkable() override { fixtureUnsupported("PlayerBoundary::getNetworkable"); }
bool getStateSync(io::BitStream* stream, IObject* object) override { fixtureUnsupported("PlayerBoundary::getStateSync"); }
bool setStateSync(io::BitStream* stream, IObject* object, double) override { fixtureUnsupported("PlayerBoundary::setStateSync"); }
void reset() override { fixtureUnsupported("PlayerBoundary::reset"); }
void setFiringState(bool) override { fixtureUnsupported("PlayerBoundary::setFiringState"); }
bool getFiringState() const override { fixtureUnsupported("PlayerBoundary::getFiringState"); }
void setAltFiringState(bool) override { fixtureUnsupported("PlayerBoundary::setAltFiringState"); }
bool getAltFiringState() const override { fixtureUnsupported("PlayerBoundary::getAltFiringState"); }
void setFlareFiringState(bool) override { fixtureUnsupported("PlayerBoundary::setFlareFiringState"); }
bool getFlareFiringState() const override { fixtureUnsupported("PlayerBoundary::getFlareFiringState"); }
void setSprintState(bool) override { fixtureUnsupported("PlayerBoundary::setSprintState"); }
bool getSprintState() const override { fixtureUnsupported("PlayerBoundary::getSprintState"); }
void setSprintStateLastTick(bool) override { fixtureUnsupported("PlayerBoundary::setSprintStateLastTick"); }
bool getSprintStateLastTick() const override { fixtureUnsupported("PlayerBoundary::getSprintStateLastTick"); }
void setReloadState() override { fixtureUnsupported("PlayerBoundary::setReloadState"); }
bool getAndResetReloadState() override { fixtureUnsupported("PlayerBoundary::getAndResetReloadState"); }
void updateFiringState(unsigned int, IObject*) override { fixtureUnsupported("PlayerBoundary::updateFiringState"); }
void updateGhostFiringState() override { fixtureUnsupported("PlayerBoundary::updateGhostFiringState"); }
void setHitIndicator(bool) override { fixtureUnsupported("PlayerBoundary::setHitIndicator"); }
bool getHitIndicator() const override { fixtureUnsupported("PlayerBoundary::getHitIndicator"); }
float updateHitFromDirection(const Vec3& direction) override { fixtureUnsupported("PlayerBoundary::updateHitFromDirection"); }
float getHitFromDirection() const override { fixtureUnsupported("PlayerBoundary::getHitFromDirection"); }
void updateCameraShake(float amount) override { fixtureUnsupported("PlayerBoundary::updateCameraShake"); }
float getCameraShake() const override { fixtureUnsupported("PlayerBoundary::getCameraShake"); }
unsigned int getSquadInviteMask() const override { fixtureUnsupported("PlayerBoundary::getSquadInviteMask"); }
void setSquadInviteMask(unsigned int mask) override { fixtureUnsupported("PlayerBoundary::setSquadInviteMask"); }
bool getIsInvitedToSquad(int squad) const override { fixtureUnsupported("PlayerBoundary::getIsInvitedToSquad"); }
void setKitTimeStamp(double t) override { fixtureUnsupported("PlayerBoundary::setKitTimeStamp"); }
double getKitTimeStamp() const override { fixtureUnsupported("PlayerBoundary::getKitTimeStamp"); }
void setCurrentFiringButton(io::PlayerInputMap button) override { fixtureUnsupported("PlayerBoundary::setCurrentFiringButton"); }
io::PlayerInputMap getCurrentFiringButton() const override { fixtureUnsupported("PlayerBoundary::getCurrentFiringButton"); }
void setPersonalDecalTemplate(const IObjectTemplate* decal) override { fixtureUnsupported("PlayerBoundary::setPersonalDecalTemplate"); }
const IObjectTemplate* getPersonalDecalTemplate() const override { fixtureUnsupported("PlayerBoundary::getPersonalDecalTemplate"); }
void enableVOHelp(bool) override { fixtureUnsupported("PlayerBoundary::enableVOHelp"); }
void enableVOHelpMessage(int) override { fixtureUnsupported("PlayerBoundary::enableVOHelpMessage"); }
void disableVOHelpMessage(int) override { fixtureUnsupported("PlayerBoundary::disableVOHelpMessage"); }
bool getVOHelpMessagePlayed(int) override { fixtureUnsupported("PlayerBoundary::getVOHelpMessagePlayed"); }
void doAddTouchItem(IPlayer* other) override { fixtureUnsupported("PlayerBoundary::doAddTouchItem"); }
void setHealRepairedAtTick(unsigned int tick, bool giving, bool repairing) override { fixtureUnsupported("PlayerBoundary::setHealRepairedAtTick"); }
unsigned int getHealRepairedAtTick() const override { fixtureUnsupported("PlayerBoundary::getHealRepairedAtTick"); }
void setGaveAmmoAtTick(unsigned int tick, bool giving) override { fixtureUnsupported("PlayerBoundary::setGaveAmmoAtTick"); }
unsigned int getGaveAmmoAtTick() const override { fixtureUnsupported("PlayerBoundary::getGaveAmmoAtTick"); }
void setFailedExitAtTick(unsigned int tick) override { fixtureUnsupported("PlayerBoundary::setFailedExitAtTick"); }
unsigned int getFailedExitAtTick() const override { fixtureUnsupported("PlayerBoundary::getFailedExitAtTick"); }
bool getIsGivingAbility() const override { fixtureUnsupported("PlayerBoundary::getIsGivingAbility"); }
bool getIsRepairing() const override { fixtureUnsupported("PlayerBoundary::getIsRepairing"); }
bool getIsGivingReplenish() const override { fixtureUnsupported("PlayerBoundary::getIsGivingReplenish"); }
int getUnlockLevel(int kit, int slot) override { fixtureUnsupported("PlayerBoundary::getUnlockLevel"); }
void setUnlocks(std::map<int, int>* unlocks) override { fixtureUnsupported("PlayerBoundary::setUnlocks"); }
std::map<int, int>* getUnlocks() override { fixtureUnsupported("PlayerBoundary::getUnlocks"); }
void setPeerAddress(const std::string& address) override { fixtureUnsupported("PlayerBoundary::setPeerAddress"); }
const std::string& getPeerAddress() const override { fixtureUnsupported("PlayerBoundary::getPeerAddress"); }
bool getVoipEnabled() override { fixtureUnsupported("PlayerBoundary::getVoipEnabled"); }
unsigned short getVoipSessionId() override { fixtureUnsupported("PlayerBoundary::getVoipSessionId"); }
void setVoipSessionId(unsigned short id) override { fixtureUnsupported("PlayerBoundary::setVoipSessionId"); }
bool getIsMuted() override { fixtureUnsupported("PlayerBoundary::getIsMuted"); }
bool getIsTalking() override { fixtureUnsupported("PlayerBoundary::getIsTalking"); }
double getLastTalkTime() override { fixtureUnsupported("PlayerBoundary::getLastTalkTime"); }
double getStopTalkingThreshold() override { fixtureUnsupported("PlayerBoundary::getStopTalkingThreshold"); }
void setIsMuted(bool) override { fixtureUnsupported("PlayerBoundary::setIsMuted"); }
void setIsTalking(bool) override { fixtureUnsupported("PlayerBoundary::setIsTalking"); }
void setVoipEnabled(bool) override { fixtureUnsupported("PlayerBoundary::setVoipEnabled"); }
void setVotingStatus(PlayerVotingStatus status) override { fixtureUnsupported("PlayerBoundary::setVotingStatus"); }
PlayerVotingStatus getVotingStatus() override { fixtureUnsupported("PlayerBoundary::getVotingStatus"); }
void setParticipateInVoting(bool) override { fixtureUnsupported("PlayerBoundary::setParticipateInVoting"); }
bool getParticipateInVoting() override { fixtureUnsupported("PlayerBoundary::getParticipateInVoting"); }
void setTargetOfVote(bool) override { fixtureUnsupported("PlayerBoundary::setTargetOfVote"); }
bool getTargetOfVote() override { fixtureUnsupported("PlayerBoundary::getTargetOfVote"); }
int getProfileId() override { fixtureUnsupported("PlayerBoundary::getProfileId"); }
void setProfileId(int id) override { fixtureUnsupported("PlayerBoundary::setProfileId"); }
void setBlurryVision(float, float, float) override { fixtureUnsupported("PlayerBoundary::setBlurryVision"); }
float getBlurValue() const override { fixtureUnsupported("PlayerBoundary::getBlurValue"); }
bool getIsNearGasCloud() const override { fixtureUnsupported("PlayerBoundary::getIsNearGasCloud"); }
bool getIsGasMaskOn() const override { fixtureUnsupported("PlayerBoundary::getIsGasMaskOn"); }
void setIsGasMaskOn(bool, bool) override { fixtureUnsupported("PlayerBoundary::setIsGasMaskOn"); }
void setGasMaskHudItem(bool) override { fixtureUnsupported("PlayerBoundary::setGasMaskHudItem"); }
float getPlayerGasMaskChangeTimer() const override { fixtureUnsupported("PlayerBoundary::getPlayerGasMaskChangeTimer"); }
float getPlayerNightVisionChangeTimer() const override { fixtureUnsupported("PlayerBoundary::getPlayerNightVisionChangeTimer"); }
bool getIsUnderwater() const override { fixtureUnsupported("PlayerBoundary::getIsUnderwater"); }
float getNormalizedNightVisionPower() override { fixtureUnsupported("PlayerBoundary::getNormalizedNightVisionPower"); }
void setNightVision(bool, bool) override { fixtureUnsupported("PlayerBoundary::setNightVision"); }
void rechargeNightVision() override { fixtureUnsupported("PlayerBoundary::rechargeNightVision"); }
void setVisualNightVision(bool) override { fixtureUnsupported("PlayerBoundary::setVisualNightVision"); }
bool getNightVision() const override { fixtureUnsupported("PlayerBoundary::getNightVision"); }
void disableFlashBangEffect() override { fixtureUnsupported("PlayerBoundary::disableFlashBangEffect"); }
void disableBlurryVision() override { fixtureUnsupported("PlayerBoundary::disableBlurryVision"); }
io::PlayerInput getPlayerInput() override { fixtureUnsupported("PlayerBoundary::getPlayerInput"); }
void resetOnDeath() override { fixtureUnsupported("PlayerBoundary::resetOnDeath"); }
bool isPlayerInNightMap() const override { fixtureUnsupported("PlayerBoundary::isPlayerInNightMap"); }
void setTracePlayerId(int id) override { fixtureUnsupported("PlayerBoundary::setTracePlayerId"); }
int getTracePlayerId() override { fixtureUnsupported("PlayerBoundary::getTracePlayerId"); }
void allocateFreeCamera() override { fixtureUnsupported("PlayerBoundary::allocateFreeCamera"); }
bool getIsThisASpam(bool) override { fixtureUnsupported("PlayerBoundary::getIsThisASpam"); }
void setRadioSpamFlagCount(int) override { fixtureUnsupported("PlayerBoundary::setRadioSpamFlagCount"); }
int getRadioSpamFlagCount() override { fixtureUnsupported("PlayerBoundary::getRadioSpamFlagCount"); }
void setRadioSpamBlockedFlagCount(int) override { fixtureUnsupported("PlayerBoundary::setRadioSpamBlockedFlagCount"); }
int getRadioSpamBlockedFlagCount() override { fixtureUnsupported("PlayerBoundary::getRadioSpamBlockedFlagCount"); }
void setRadioSpamFilterFlagTimer(float) override { fixtureUnsupported("PlayerBoundary::setRadioSpamFilterFlagTimer"); }
float getRadioSpamFilterFlagTimer() override { fixtureUnsupported("PlayerBoundary::getRadioSpamFilterFlagTimer"); }
void resetSpamFilter() override { fixtureUnsupported("PlayerBoundary::resetSpamFilter"); }
void getCloseMines(std::vector<IObject*>& out) const override { fixtureUnsupported("PlayerBoundary::getCloseMines"); }
bool getAutoReload() const override { fixtureUnsupported("PlayerBoundary::getAutoReload"); }
void setAutoReload(bool) override { fixtureUnsupported("PlayerBoundary::setAutoReload"); }
void updateClientSettings() override { fixtureUnsupported("PlayerBoundary::updateClientSettings"); }
};
}}}
namespace dice { namespace hfe {
struct LogicBoundary : IGameLogic {
void addRef() override { fixtureUnsupported("LogicBoundary::addRef"); }
int getRef() override { fixtureUnsupported("LogicBoundary::getRef"); }
int release() override { fixtureUnsupported("LogicBoundary::release"); }
void* queryInterface(unsigned int) const override { fixtureUnsupported("LogicBoundary::queryInterface"); }
bool init() override { fixtureUnsupported("LogicBoundary::init"); }
bool load(const std::string& level) override { fixtureUnsupported("LogicBoundary::load"); }
bool unload() override { fixtureUnsupported("LogicBoundary::unload"); }
bool close() override { fixtureUnsupported("LogicBoundary::close"); }
void update(float deltaTime) override { fixtureUnsupported("LogicBoundary::update"); }
void reset() override { fixtureUnsupported("LogicBoundary::reset"); }
void restartDemoPlayback() override { fixtureUnsupported("LogicBoundary::restartDemoPlayback"); }
void runNextLevel() override { fixtureUnsupported("LogicBoundary::runNextLevel"); }
void setNextLevel(int) override { fixtureUnsupported("LogicBoundary::setNextLevel"); }
void addPreStoredTemplate(unsigned int, unsigned int, unsigned int) override { fixtureUnsupported("LogicBoundary::addPreStoredTemplate"); }
void createPreStoredObjects() override { fixtureUnsupported("LogicBoundary::createPreStoredObjects"); }
unsigned int getStartPreStoredNetID() const override { fixtureUnsupported("LogicBoundary::getStartPreStoredNetID"); }
unsigned int getEndPreStoredNetID() const override { fixtureUnsupported("LogicBoundary::getEndPreStoredNetID"); }
void setTeamName(int team, const std::string&) override { fixtureUnsupported("LogicBoundary::setTeamName"); }
const std::string& getTeamName(int team) override { fixtureUnsupported("LogicBoundary::getTeamName"); }
int getTeamFromName(const std::string&) override { fixtureUnsupported("LogicBoundary::getTeamFromName"); }
const std::string& getTeamDropVehicle(int team) const override { fixtureUnsupported("LogicBoundary::getTeamDropVehicle"); }
void setNumDropVehiclesForTeam(int team, int n) override { fixtureUnsupported("LogicBoundary::setNumDropVehiclesForTeam"); }
int getNumDropVehiclesForTeam(int team) override { fixtureUnsupported("LogicBoundary::getNumDropVehiclesForTeam"); }
void setDropVehicleDeviationRadius(int, int) override { fixtureUnsupported("LogicBoundary::setDropVehicleDeviationRadius"); }
int getDropVehicleDeviationRadius() override { fixtureUnsupported("LogicBoundary::getDropVehicleDeviationRadius"); }
int getDropVehicleDeviationMinRadius() override { fixtureUnsupported("LogicBoundary::getDropVehicleDeviationMinRadius"); }
bool killPlayer(world::IPlayer*, bool, bool) override { fixtureUnsupported("LogicBoundary::killPlayer"); }
bool reSpawnPlayer(world::IPlayer*) override { fixtureUnsupported("LogicBoundary::reSpawnPlayer"); }
bool selectSpawnGroup(world::IPlayer*, int) override { fixtureUnsupported("LogicBoundary::selectSpawnGroup"); }
bool isLevelLoadedXP() override { fixtureUnsupported("LogicBoundary::isLevelLoadedXP"); }
bool getNoVehiclesMode() override { fixtureUnsupported("LogicBoundary::getNoVehiclesMode"); }
void setNoVehiclesMode(bool) override { fixtureUnsupported("LogicBoundary::setNoVehiclesMode"); }
void setTicketState(unsigned int team, int) override { fixtureUnsupported("LogicBoundary::setTicketState"); }
int getTicketState(unsigned int team) override { fixtureUnsupported("LogicBoundary::getTicketState"); }
void setDefaultNumberOfTickets(unsigned int, unsigned int) override { fixtureUnsupported("LogicBoundary::setDefaultNumberOfTickets"); }
unsigned int getDefaultNumberOfTickets(unsigned int) override { fixtureUnsupported("LogicBoundary::getDefaultNumberOfTickets"); }
void setDefaultNumberOfTicketsEx(unsigned int, unsigned int, unsigned int) override { fixtureUnsupported("LogicBoundary::setDefaultNumberOfTicketsEx"); }
unsigned int getDefaultNumberOfTicketsEx(unsigned int, unsigned int) override { fixtureUnsupported("LogicBoundary::getDefaultNumberOfTicketsEx"); }
void setDefaultTimeToNextAIWave(float) override { fixtureUnsupported("LogicBoundary::setDefaultTimeToNextAIWave"); }
float getDefaultTimeToNextAIWave() override { fixtureUnsupported("LogicBoundary::getDefaultTimeToNextAIWave"); }
void setTicketLossAtEndPerMin(float) override { fixtureUnsupported("LogicBoundary::setTicketLossAtEndPerMin"); }
float getTicketLossAtEndPerMin() override { fixtureUnsupported("LogicBoundary::getTicketLossAtEndPerMin"); }
void setTicketLossPerMin(unsigned int team, float) override { fixtureUnsupported("LogicBoundary::setTicketLossPerMin"); }
float getTicketLossPerMin(unsigned int team) override { fixtureUnsupported("LogicBoundary::getTicketLossPerMin"); }
void setMaximumLevelViewDistance(float) override { fixtureUnsupported("LogicBoundary::setMaximumLevelViewDistance"); }
float getMaximumLevelViewDistance() const override { fixtureUnsupported("LogicBoundary::getMaximumLevelViewDistance"); }
void setExtraUAVVehicleFlightHeight(float) override { fixtureUnsupported("LogicBoundary::setExtraUAVVehicleFlightHeight"); }
float getExtraUAVVehicleFlightHeight() const override { fixtureUnsupported("LogicBoundary::getExtraUAVVehicleFlightHeight"); }
void setFogStartEndAndBase(const Vec4&) override { fixtureUnsupported("LogicBoundary::setFogStartEndAndBase"); }
const Vec4& getFogStartEndAndBase() const override { fixtureUnsupported("LogicBoundary::getFogStartEndAndBase"); }
void setDefaultLevel(const std::string&) override { fixtureUnsupported("LogicBoundary::setDefaultLevel"); }
const std::string& getDefaultLevel() const override { fixtureUnsupported("LogicBoundary::getDefaultLevel"); }
void refreshTriggerables() override { fixtureUnsupported("LogicBoundary::refreshTriggerables"); }
void playEffect(Vec3, const world::IObjectTemplate*, Vec3) override { fixtureUnsupported("LogicBoundary::playEffect"); }
bool giveDamage(world::IObject*, float, world::IPlayer*, std::list<WeakPtr<world::IPlayer> >*, world::IObject*, int, world::IObject*, world::IObject*, Vec3, Vec3, int, bool, bool, bool) override { fixtureUnsupported("LogicBoundary::giveDamage"); }
void handleExplosion(Vec3, float, int, float, world::IPlayer*, world::IArmorGLComp*, int, world::IObject*, world::IObject*, float, world::IObject*, world::IObject*, float, bool, bool) override { fixtureUnsupported("LogicBoundary::handleExplosion"); }
void handlePickup(world::IObject*, world::IPlayer*, world::IObject*, bool) override { fixtureUnsupported("LogicBoundary::handlePickup"); }
void handleDrop(world::IObject*, world::IPlayer*, world::IObject*, bool) override { fixtureUnsupported("LogicBoundary::handleDrop"); }
bool handleCollisionForProjectile(world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int, world::CollisionFaceInfo*, world::CollisionFaceInfo*) override { fixtureUnsupported("LogicBoundary::handleCollisionForProjectile"); }
bool handleCollisionForReplenisher(bool, world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int) override { fixtureUnsupported("LogicBoundary::handleCollisionForReplenisher"); }
bool handleCollisionForResurrecter(world::IObject*, world::IObject*, float, float, const Vec3&, const Vec3&, const Vec3&, int, int) override { fixtureUnsupported("LogicBoundary::handleCollisionForResurrecter"); }
void handleCollision(world::IObject*, world::IObject*, const Vec3&, const Vec3&, const Vec3&, int, int, world::CollisionFaceInfo*, world::CollisionFaceInfo*) override { fixtureUnsupported("LogicBoundary::handleCollision"); }
bool heal(world::IObject*, float, world::IPlayer*, int, int, float*, bool, bool) override { fixtureUnsupported("LogicBoundary::heal"); }
bool replenishAmmo(world::IObject*, float, world::IPlayer*, int, int, float*, bool) override { fixtureUnsupported("LogicBoundary::replenishAmmo"); }
void doImpact(int, int, world::MaterialCollisionType, const Vec3&, const Vec3&, const Vec3&, world::IObject*, world::IObject*, world::IProjectileObject*, world::CollisionFaceInfo*, Vec3, bool) override { fixtureUnsupported("LogicBoundary::doImpact"); }
void playCollisionEffect(Vec3, const world::IObjectTemplate*, Vec3, Vec3) override { fixtureUnsupported("LogicBoundary::playCollisionEffect"); }
void spawnMultipleObjects(const world::IObjectTemplate*, int) override { fixtureUnsupported("LogicBoundary::spawnMultipleObjects"); }
world::IObject* spawnObject(const std::string&, Vec3, Vec3, int) override { fixtureUnsupported("LogicBoundary::spawnObject"); }
world::IObject* spawnObject(const world::IObjectTemplate*, Vec3, Vec3, int) override { fixtureUnsupported("LogicBoundary::spawnObject"); }
void triggerSpecialGameEvent(SpecialGameEventType, void*) override { fixtureUnsupported("LogicBoundary::triggerSpecialGameEvent"); }
bool remoteCommand(int, int, int, int, int) override { fixtureUnsupported("LogicBoundary::remoteCommand"); }
void setUseActiveCombatArea(bool) override { fixtureUnsupported("LogicBoundary::setUseActiveCombatArea"); }
bool getUseActiveCombatArea() override { fixtureUnsupported("LogicBoundary::getUseActiveCombatArea"); }
void setActiveCombatArea(float, float, float, float) override { fixtureUnsupported("LogicBoundary::setActiveCombatArea"); }
void getActiveCombatArea(float*, float*, float*, float*) override { fixtureUnsupported("LogicBoundary::getActiveCombatArea"); }
float getTimeUntilRestartMap() override { fixtureUnsupported("LogicBoundary::getTimeUntilRestartMap"); }
bool isServer() const override { fixtureUnsupported("LogicBoundary::isServer"); }
void setKit(int, int, const std::string&, const std::string&) override { fixtureUnsupported("LogicBoundary::setKit"); }
world::IObjectTemplate* getKitTemplate(int, int) override { fixtureUnsupported("LogicBoundary::getKitTemplate"); }
std::string getKitTemplateString(int, int) override { fixtureUnsupported("LogicBoundary::getKitTemplateString"); }
std::string getSoldierSkin(int, int) override { fixtureUnsupported("LogicBoundary::getSoldierSkin"); }
void* getKits() const override { fixtureUnsupported("LogicBoundary::getKits"); }
void checkPlayerTriggers(world::IPlayer*, world::IObject*, io::PlayerInput, bool) override { fixtureUnsupported("LogicBoundary::checkPlayerTriggers"); }
int getGameStatus() override { fixtureUnsupported("LogicBoundary::getGameStatus"); }
BeginRoundState getBeginRoundState() override { fixtureUnsupported("LogicBoundary::getBeginRoundState"); }
void setBeginRoundState(BeginRoundState) override { fixtureUnsupported("LogicBoundary::setBeginRoundState"); }
unsigned int getNextStateChangeAtTick() override { fixtureUnsupported("LogicBoundary::getNextStateChangeAtTick"); }
void setNextStateChangeAtTick(unsigned int) override { fixtureUnsupported("LogicBoundary::setNextStateChangeAtTick"); }
float getTimeToNextStateChange() override { fixtureUnsupported("LogicBoundary::getTimeToNextStateChange"); }
void addControlPoint(world::IObject*, int) override { fixtureUnsupported("LogicBoundary::addControlPoint"); }
std::map<unsigned int, world::IObject*>* getControlPointMap() const override { fixtureUnsupported("LogicBoundary::getControlPointMap"); }
world::IObject* getControlPoint(unsigned int) override { fixtureUnsupported("LogicBoundary::getControlPoint"); }
void removeControlPoint(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeControlPoint"); }
bool getControlPointsInGroup(std::vector<world::IObject*>&, int group) override { fixtureUnsupported("LogicBoundary::getControlPointsInGroup"); }
int getNumberOfControlPointGroups() override { fixtureUnsupported("LogicBoundary::getNumberOfControlPointGroups"); }
void addFlag(world::IObject*) override { fixtureUnsupported("LogicBoundary::addFlag"); }
void* getFlagMap() const override { fixtureUnsupported("LogicBoundary::getFlagMap"); }
void removeFlag(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeFlag"); }
void addPco(world::IObject*) override { fixtureUnsupported("LogicBoundary::addPco"); }
std::map<unsigned int, world::IObject*>* getPcoMap() const override { fixtureUnsupported("LogicBoundary::getPcoMap"); }
void removePco(world::IObject*) override { fixtureUnsupported("LogicBoundary::removePco"); }
void addStrategicObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::addStrategicObject"); }
std::map<std::string, world::StrategicObjectHolder*>* getStrategicObjectMap() override { fixtureUnsupported("LogicBoundary::getStrategicObjectMap"); }
void removeStrategicObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeStrategicObject"); }
void addTargetObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::addTargetObject"); }
void* getTargetObjectMap() const override { fixtureUnsupported("LogicBoundary::getTargetObjectMap"); }
void removeTargetObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeTargetObject"); }
world::IObject* getTargetObject(unsigned int) const override { fixtureUnsupported("LogicBoundary::getTargetObject"); }
world::IObject* findBestTargetObject(const Vec3&, const Vec3&, world::TargetType, float, float, world::IObject*, world::IObject*, float, const world::IObject*, bool, bool, bool) override { fixtureUnsupported("LogicBoundary::findBestTargetObject"); }
void addRemoteControlledObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::addRemoteControlledObject"); }
void* getRemoteControlledObjectMap() const override { fixtureUnsupported("LogicBoundary::getRemoteControlledObjectMap"); }
void removeRemoteControlledObject(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeRemoteControlledObject"); }
world::IObject* getRemoteControlledObject(unsigned int) const override { fixtureUnsupported("LogicBoundary::getRemoteControlledObject"); }
void addProjectile(world::IObject*) override { fixtureUnsupported("LogicBoundary::addProjectile"); }
void* getProjectileMap() const override { fixtureUnsupported("LogicBoundary::getProjectileMap"); }
void removeProjectile(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeProjectile"); }
void addObjectSpawner(world::IObject*) override { fixtureUnsupported("LogicBoundary::addObjectSpawner"); }
void* getObjectSpawnerMap() const override { fixtureUnsupported("LogicBoundary::getObjectSpawnerMap"); }
void removeObjectSpawner(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeObjectSpawner"); }
world::IObject* getObjectSpawner(unsigned int) const override { fixtureUnsupported("LogicBoundary::getObjectSpawner"); }
void addSupplyDepot(world::IObject*) override { fixtureUnsupported("LogicBoundary::addSupplyDepot"); }
void* getSupplyDepotMap() const override { fixtureUnsupported("LogicBoundary::getSupplyDepotMap"); }
void removeSupplyDepot(world::IObject*) override { fixtureUnsupported("LogicBoundary::removeSupplyDepot"); }
void setTeamLanguage(int, const std::string&) override { fixtureUnsupported("LogicBoundary::setTeamLanguage"); }
const std::string& getTeamLanguage(int) const override { fixtureUnsupported("LogicBoundary::getTeamLanguage"); }
void setTeamFlag(int, const std::string&) override { fixtureUnsupported("LogicBoundary::setTeamFlag"); }
const std::string& getTeamFlag(int) const override { fixtureUnsupported("LogicBoundary::getTeamFlag"); }
void toggleLocalFreeCamera() override { fixtureUnsupported("LogicBoundary::toggleLocalFreeCamera"); }
Vec3 getBeforeSpawnCameraPosition() const override { fixtureUnsupported("LogicBoundary::getBeforeSpawnCameraPosition"); }
Vec3 getBeforeSpawnCameraDirection() const override { fixtureUnsupported("LogicBoundary::getBeforeSpawnCameraDirection"); }
void resetPlayerFreeCameraPosition(world::IPlayer*) override { fixtureUnsupported("LogicBoundary::resetPlayerFreeCameraPosition"); }
bool isLevelLoaded() const override { fixtureUnsupported("LogicBoundary::isLevelLoaded"); }
bool isPrecaching() const override { fixtureUnsupported("LogicBoundary::isPrecaching"); }
bool getEnableVoHelp() override { fixtureUnsupported("LogicBoundary::getEnableVoHelp"); }
void stopPlayingVoMessages() override { fixtureUnsupported("LogicBoundary::stopPlayingVoMessages"); }
void setSupplyDropHeight(float) override { fixtureUnsupported("LogicBoundary::setSupplyDropHeight"); }
float getSupplyDropHeight() override { fixtureUnsupported("LogicBoundary::getSupplyDropHeight"); }
void setVehicleDropHeight(float) override { fixtureUnsupported("LogicBoundary::setVehicleDropHeight"); }
float getVehicleDropHeight() override { fixtureUnsupported("LogicBoundary::getVehicleDropHeight"); }
void setSupplyDropNumSecsToLive(float) override { fixtureUnsupported("LogicBoundary::setSupplyDropNumSecsToLive"); }
float getSupplyDropNumSecsToLive() override { fixtureUnsupported("LogicBoundary::getSupplyDropNumSecsToLive"); }
void setFeslEnvironment(int) override { fixtureUnsupported("LogicBoundary::setFeslEnvironment"); }
int getFeslEnvironment() override { fixtureUnsupported("LogicBoundary::getFeslEnvironment"); }
void storeVoipServerInfo(bool, unsigned long, unsigned short, unsigned short, int) override { fixtureUnsupported("LogicBoundary::storeVoipServerInfo"); }
void getVoipServerInfo(bool&, unsigned int&, unsigned short&, unsigned short&, int&) override { fixtureUnsupported("LogicBoundary::getVoipServerInfo"); }
void startupVoipClient() override { fixtureUnsupported("LogicBoundary::startupVoipClient"); }
bool isVoipClientRunning() override { fixtureUnsupported("LogicBoundary::isVoipClientRunning"); }
bool getAirMouseInverted() override { fixtureUnsupported("LogicBoundary::getAirMouseInverted"); }
void setAirMouseSensitivity(float) override { fixtureUnsupported("LogicBoundary::setAirMouseSensitivity"); }
float getAirMouseSensitivity() override { fixtureUnsupported("LogicBoundary::getAirMouseSensitivity"); }
void becomeSquadLeader(bool) override { fixtureUnsupported("LogicBoundary::becomeSquadLeader"); }
TicketHistory& getTicketHistory() override { fixtureUnsupported("LogicBoundary::getTicketHistory"); }
void setTicketHistory(const TicketHistory&) override { fixtureUnsupported("LogicBoundary::setTicketHistory"); }
void setHitIndicationThreshold(float) override { fixtureUnsupported("LogicBoundary::setHitIndicationThreshold"); }
float getHitIndicationThreshold() override { fixtureUnsupported("LogicBoundary::getHitIndicationThreshold"); }
float getUAVActiveTime() const override { fixtureUnsupported("LogicBoundary::getUAVActiveTime"); }
float getSatelliteScanActiveTime() const override { fixtureUnsupported("LogicBoundary::getSatelliteScanActiveTime"); }
float getUAVReloadTime() const override { fixtureUnsupported("LogicBoundary::getUAVReloadTime"); }
float getUAVRadius() const override { fixtureUnsupported("LogicBoundary::getUAVRadius"); }
float getSatelliteScanReloadTime() const override { fixtureUnsupported("LogicBoundary::getSatelliteScanReloadTime"); }
float getSatelliteScanVisibleTime() const override { fixtureUnsupported("LogicBoundary::getSatelliteScanVisibleTime"); }
world::IObject* getGroundDecalObject() override { fixtureUnsupported("LogicBoundary::getGroundDecalObject"); }
void noVehiclesOmit(std::string) override { fixtureUnsupported("LogicBoundary::noVehiclesOmit"); }
bool isOmitted(std::string) override { fixtureUnsupported("LogicBoundary::isOmitted"); }
void AddInvalidDropVehicleObject(std::string, bool) override { fixtureUnsupported("LogicBoundary::AddInvalidDropVehicleObject"); }
std::list<std::pair<std::string, bool> >* GetInvalidVehicleDropObjectList() override { fixtureUnsupported("LogicBoundary::GetInvalidVehicleDropObjectList"); }
void setDebugMissiles(int) override { fixtureUnsupported("LogicBoundary::setDebugMissiles"); }
int getDebugMissiles() override { fixtureUnsupported("LogicBoundary::getDebugMissiles"); }
void setDebugReplenish(int) override { fixtureUnsupported("LogicBoundary::setDebugReplenish"); }
int getDebugReplenish() override { fixtureUnsupported("LogicBoundary::getDebugReplenish"); }
void setTeamDropVehicle(int, const std::string&) override { fixtureUnsupported("LogicBoundary::setTeamDropVehicle"); }
void ClearDropVehicles() override { fixtureUnsupported("LogicBoundary::ClearDropVehicles"); }
void handleEvent(EventCategory, unsigned int, EventNode*, void*) override { fixtureUnsupported("LogicBoundary::handleEvent"); }
void killAllPlayers() override { fixtureUnsupported("LogicBoundary::killAllPlayers"); }
bool preCacheSpawnedTemplates() override { fixtureUnsupported("LogicBoundary::preCacheSpawnedTemplates"); }
void clearConsole() override { fixtureUnsupported("LogicBoundary::clearConsole"); }
bool loadUpdateComponents() override { fixtureUnsupported("LogicBoundary::loadUpdateComponents"); }
bool loadInitFile(const std::string&) override { fixtureUnsupported("LogicBoundary::loadInitFile"); }
bool loadAllConFiles(const std::string&) override { fixtureUnsupported("LogicBoundary::loadAllConFiles"); }
void loadVoiceMessages() override { fixtureUnsupported("LogicBoundary::loadVoiceMessages"); }
bool setCreatedOnLoadFlag() override { fixtureUnsupported("LogicBoundary::setCreatedOnLoadFlag"); }
void createSunTemplate() override { fixtureUnsupported("LogicBoundary::createSunTemplate"); }
void resetAllPlayers() override { fixtureUnsupported("LogicBoundary::resetAllPlayers"); }
void resetAllObjects() override { fixtureUnsupported("LogicBoundary::resetAllObjects"); }
};
}}
