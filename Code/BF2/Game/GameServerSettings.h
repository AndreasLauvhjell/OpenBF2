// Client mirror of ServerSettings. reset loads configuration into the record and copies it; apply writes it back.
#ifndef GAME_GAMESERVERSETTINGS_H
#define GAME_GAMESERVERSETTINGS_H
#include "../Core/IBase.h"
#include <string>
#if !DEDICATED
namespace dice { namespace hfe {

// Every slot of table 8A3DB0 except the deleting destructor at +1B8. Accessor names follow the
// script literals ("GameServerSettings.setXxx") and the IDA names of the tiny bodies; string
// getters return a copy (sret) and string setters take their argument by value (retn 1Ch).
class IGameServerSettings : public IBase {
public:
    virtual std::string getFileName() = 0;        //+10 4B3EA0 "ServerSettings.con"
    virtual std::string getSettingsScript() = 0;
    virtual bool apply() = 0;
    virtual void reset() = 0;                     //+1C 4B4280; createDefaultProfile 4A80ED
    virtual std::string getServerName() = 0;      //+20
    virtual void setServerName(std::string) = 0;  //+24
    virtual std::string getPassword() = 0;        //+28
    virtual void setPassword(std::string) = 0;    //+2C
    virtual bool getInternet() = 0;               //+30
    virtual void setInternet(bool) = 0;           //+34
    virtual int getBandwidthChoke() = 0;          //+38
    virtual void setBandwidthChoke(int) = 0;      //+3C
    virtual int getMaxPlayers() = 0;              //+40
    virtual void setMaxPlayers(int) = 0;          //+44
    virtual std::string getGameMode() = 0;        //+48
    virtual void setGameMode(std::string) = 0;    //+4C
    virtual int getStartDelay() = 0;              //+50
    virtual void setStartDelay(int) = 0;          //+54
    virtual int getEndDelay() = 0;                //+58
    virtual void setEndDelay(int) = 0;            //+5C
    virtual float getSpawnTime() = 0;             //+60
    virtual void setSpawnTime(float) = 0;         //+64
    virtual float getManDownTime() = 0;           //+68
    virtual void setManDownTime(float) = 0;       //+6C
    virtual int getTicketRatio() = 0;             //+70
    virtual void setTicketRatio(int) = 0;         //+74
    virtual int getRoundsPerMap() = 0;            //+78
    virtual void setRoundsPerMap(int) = 0;        //+7C
    virtual int getTimeLimit() = 0;               //+80
    virtual void setTimeLimit(int) = 0;           //+84
    virtual int getScoreLimit() = 0;              //+88
    virtual void setScoreLimit(int) = 0;          //+8C
    virtual int getSoldierFF() = 0;               //+90
    virtual void setSoldierFF(int) = 0;           //+94
    virtual int getVehicleFF() = 0;               //+98
    virtual void setVehicleFF(int) = 0;           //+9C
    virtual int getSoldierSplashFF() = 0;         //+A0
    virtual void setSoldierSplashFF(int) = 0;     //+A4
    virtual int getVehicleSplashFF() = 0;         //+A8
    virtual void setVehicleSplashFF(int) = 0;     //+AC
    virtual bool getPunishTeamKills() = 0;        //+B0
    virtual void setPunishTeamKills(bool) = 0;    //+B4
    virtual int getVoteTime() = 0;                //+B8
    virtual void setVoteTime(int) = 0;            //+BC
    virtual int getMinPlayersForVoting() = 0;     //+C0
    virtual void setMinPlayersForVoting(int) = 0; //+C4
    virtual bool getTeamVoteOnly() = 0;           //+C8
    virtual void setTeamVoteOnly(bool) = 0;       //+CC
    virtual bool getVoipEnabled() = 0;            //+D0
    virtual void setVoipEnabled(bool) = 0;        //+D4
    virtual int getVoipQuality() = 0;             //+D8
    virtual void setVoipQuality(int) = 0;         //+DC; createDefaultProfile 4A80FA passes 5
    virtual bool getVoipServerRemote() = 0;       //+E0
    virtual void setVoipServerRemote(bool) = 0;   //+E4
    virtual std::string getVoipServerRemoteIP() = 0;     //+E8
    virtual void setVoipServerRemoteIP(std::string) = 0; //+EC
    virtual int getVoipServerPort() = 0;          //+F0
    virtual void setVoipServerPort(int) = 0;      //+F4
    virtual int getVoipBFClientPort() = 0;        //+F8
    virtual void setVoipBFClientPort(int) = 0;    //+FC
    virtual int getVoipBFServerPort() = 0;        //+100
    virtual void setVoipBFServerPort(int) = 0;    //+104
    virtual std::string getVoipSharedPassword() = 0;     //+108
    virtual void setVoipSharedPassword(std::string) = 0; //+10C
    virtual int getGameSpyPort() = 0;             //+110
    virtual void setGameSpyPort(int) = 0;         //+114
    virtual bool getAllowNATNegotiation() = 0;    //+118
    virtual void setAllowNATNegotiation(bool) = 0;//+11C
    virtual std::string getInterfaceIP() = 0;     //+120
    virtual void setInterfaceIP(std::string) = 0; //+124
    virtual bool getAutoRecord() = 0;             //+128
    virtual void setAutoRecord(bool) = 0;         //+12C
    virtual std::string getDemoIndexURL() = 0;    //+130
    virtual void setDemoIndexURL(std::string) = 0;//+134
    virtual std::string getDemoDownloadURL() = 0; //+138
    virtual void setDemoDownloadURL(std::string) = 0; //+13C
    virtual std::string getDemoHook() = 0;        //+140
    virtual void setDemoHook(std::string) = 0;    //+144
    virtual std::string getAdminScript() = 0;     //+148
    virtual void setAdminScript(std::string) = 0; //+14C
    virtual float getTimeBeforeRestartMap() = 0;  //+150
    virtual void setTimeBeforeRestartMap(float) = 0; //+154
    virtual std::string getSponsorText() = 0;     //+158
    virtual void setSponsorText(std::string) = 0; //+15C
    virtual std::string getSponsorLogoURL() = 0;  //+160
    virtual void setSponsorLogoURL(std::string) = 0; //+164
    virtual bool getAutoBalanceTeam() = 0;        //+168
    virtual void setAutoBalanceTeam(bool) = 0;    //+16C
    virtual int getAutoRecordRounds() = 0;        //+170 [INFERRED, IDA]
    virtual void setAutoRecordRounds(int) = 0;    //+174
    virtual float getTeamRatio() = 0;             //+178
    virtual void setTeamRatio(float) = 0;         //+17C
    virtual bool getSvPunkBuster() = 0;           //+180
    virtual void setSvPunkBuster(bool) = 0;       //+184
    // The string at retail+1D8 has no script row, is never reset and never mirrored; only this
    // pair touches it. [UNRESOLVED] role.
    virtual std::string getField1D8() = 0;
    virtual void setField1D8(std::string) = 0;    //+18C 4B3E10
    virtual bool getFriendlyFireWithMines() = 0;  //+190
    virtual void setFriendlyFireWithMines(bool) = 0; //+194
    virtual float getCoopBotRatio() = 0;          //+198
    virtual void setCoopBotRatio(float) = 0;      //+19C
    virtual int getCoopBotCount() = 0;            //+1A0
    virtual void setCoopBotCount(int) = 0;        //+1A4
    virtual float getCoopBotDifficulty() = 0;     //+1A8
    virtual void setCoopBotDifficulty(float) = 0; //+1AC
    virtual bool getNoVehicles() = 0;             //+1B0
    virtual void setNoVehicles(bool) = 0;         //+1B4
    // The deleting destructor at +1B8 is the only withheld slot.
};

struct GameServerSettingsFixture;
class GameServerSettings final : public IGameServerSettings {
public:
    GameServerSettings();                         // retail 520 bytes
    // [CHOSEN, not recovered] [RETIRE-WHEN: game-server-settings-prefix-publication]
    // Direct final-owner deletion; the deleting destructor lives at the withheld +1B8 (4B3EE0).
    ~GameServerSettings();                        // body
    static IBase* create(unsigned int iid, IBase* outer);
    void addRef() override;                       // shared
    int getRef() override;                        // shared
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    std::string getFileName() override;
    std::string getSettingsScript() override;
    bool apply() override;
    void reset() override;
    std::string getServerName() override;            void setServerName(std::string value) override;
    std::string getPassword() override;              void setPassword(std::string value) override;
    bool getInternet() override;                     void setInternet(bool value) override;
    int getBandwidthChoke() override;                void setBandwidthChoke(int value) override;
    int getMaxPlayers() override;                    void setMaxPlayers(int value) override;
    std::string getGameMode() override;              void setGameMode(std::string value) override;
    int getStartDelay() override;                    void setStartDelay(int value) override;
    int getEndDelay() override;                      void setEndDelay(int value) override;
    float getSpawnTime() override;                   void setSpawnTime(float value) override;
    float getManDownTime() override;                 void setManDownTime(float value) override;
    int getTicketRatio() override;                   void setTicketRatio(int value) override;
    int getRoundsPerMap() override;                  void setRoundsPerMap(int value) override;
    int getTimeLimit() override;                     void setTimeLimit(int value) override;
    int getScoreLimit() override;                    void setScoreLimit(int value) override;
    int getSoldierFF() override;                     void setSoldierFF(int value) override;
    int getVehicleFF() override;                     void setVehicleFF(int value) override;
    int getSoldierSplashFF() override;               void setSoldierSplashFF(int value) override;
    int getVehicleSplashFF() override;               void setVehicleSplashFF(int value) override;
    bool getPunishTeamKills() override;              void setPunishTeamKills(bool value) override;
    int getVoteTime() override;                      void setVoteTime(int value) override;
    int getMinPlayersForVoting() override;           void setMinPlayersForVoting(int value) override;
    bool getTeamVoteOnly() override;                 void setTeamVoteOnly(bool value) override;
    bool getVoipEnabled() override;                  void setVoipEnabled(bool value) override;
    int getVoipQuality() override;                   void setVoipQuality(int value) override;
    bool getVoipServerRemote() override;             void setVoipServerRemote(bool value) override;
    std::string getVoipServerRemoteIP() override;    void setVoipServerRemoteIP(std::string value) override;
    int getVoipServerPort() override;                void setVoipServerPort(int value) override;
    int getVoipBFClientPort() override;              void setVoipBFClientPort(int value) override;
    int getVoipBFServerPort() override;              void setVoipBFServerPort(int value) override;
    std::string getVoipSharedPassword() override;    void setVoipSharedPassword(std::string value) override;
    int getGameSpyPort() override;                   void setGameSpyPort(int value) override;
    bool getAllowNATNegotiation() override;          void setAllowNATNegotiation(bool value) override;
    std::string getInterfaceIP() override;           void setInterfaceIP(std::string value) override;
    bool getAutoRecord() override;                   void setAutoRecord(bool value) override;
    std::string getDemoIndexURL() override;          void setDemoIndexURL(std::string value) override;
    std::string getDemoDownloadURL() override;       void setDemoDownloadURL(std::string value) override;
    std::string getDemoHook() override;              void setDemoHook(std::string value) override;
    std::string getAdminScript() override;           void setAdminScript(std::string value) override;
    float getTimeBeforeRestartMap() override;        void setTimeBeforeRestartMap(float value) override;
    std::string getSponsorText() override;           void setSponsorText(std::string value) override;
    std::string getSponsorLogoURL() override;        void setSponsorLogoURL(std::string value) override;
    bool getAutoBalanceTeam() override;              void setAutoBalanceTeam(bool value) override;
    int getAutoRecordRounds() override;              void setAutoRecordRounds(int value) override;
    float getTeamRatio() override;                   void setTeamRatio(float value) override;
    bool getSvPunkBuster() override;                 void setSvPunkBuster(bool value) override;
    std::string getField1D8() override;              void setField1D8(std::string value) override;
    bool getFriendlyFireWithMines() override;        void setFriendlyFireWithMines(bool value) override;
    float getCoopBotRatio() override;                void setCoopBotRatio(float value) override;
    int getCoopBotCount() override;                  void setCoopBotCount(int value) override;
    float getCoopBotDifficulty() override;           void setCoopBotDifficulty(float value) override;
    bool getNoVehicles() override;                   void setNoVehicles(bool value) override;
private:
    friend struct GameServerSettingsFixture;
    int m_refCount;                       // retail+04
    std::string m_serverName;             //+08
    std::string m_password;               //+24
    bool m_internet;                      //+40
    int m_bandwidthChoke;                 //+44
    int m_maxPlayers;                     //+48
    std::string m_gameMode;               //+4C
    int m_startDelay;                     //+68
    int m_endDelay;                       //+6C
    float m_spawnTime;                    //+70
    float m_manDownTime;                  //+74
    int m_ticketRatio;                    //+78
    int m_roundsPerMap;                   //+7C
    int m_timeLimit;                      //+80
    int m_scoreLimit;                     //+84
    int m_soldierFF;                      //+88
    int m_vehicleFF;                      //+8C
    int m_soldierSplashFF;                //+90
    int m_vehicleSplashFF;                //+94
    bool m_punishTeamKills;               //+98
    bool m_votingEnabled;                 //+99, no accessor pair; reset/apply/script only
    int m_voteTime;                       //+9C
    int m_minPlayersForVoting;            //+A0
    bool m_teamVoteOnly;                  //+A4
    bool m_voipEnabled;                   //+A5
    int m_voipQuality;                    //+A8
    bool m_voipServerRemote;              //+AC
    std::string m_voipServerRemoteIP;     //+B0
    int m_voipServerPort;                 //+CC
    int m_voipBFClientPort;               //+D0
    int m_voipBFServerPort;               //+D4
    std::string m_voipSharedPassword;     //+D8
    int m_gameSpyPort;                    //+F4
    bool m_allowNATNegotiation;           //+F8
    std::string m_interfaceIP;            //+FC
    bool m_autoRecord;                    //+118
    std::string m_demoIndexURL;           //+11C
    std::string m_demoDownloadURL;        //+138
    std::string m_demoHook;               //+154
    std::string m_adminScript;            //+170
    float m_timeBeforeRestartMap;         //+18C
    std::string m_sponsorText;            //+190
    std::string m_sponsorLogoURL;         //+1AC
    bool m_autoBalanceTeam;               //+1C8
    int m_autoRecordRounds;               //+1CC, assigned by the fallback branch only
    float m_teamRatio;                    //+1D0
    bool m_svPunkBuster;                  //+1D4
    std::string m_field1D8;               //+1D8 [UNRESOLVED]
    bool m_friendlyFireWithMines;         //+1F4, assigned by neither reset branch (see the ctor)
    float m_coopBotRatio;                 //+1F8
    int m_coopBotCount;                   //+1FC
    float m_coopBotDifficulty;            //+200
    bool m_noVehicles;                    //+204
};

} }
#endif
#endif
