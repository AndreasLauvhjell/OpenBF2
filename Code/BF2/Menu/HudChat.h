// HUD chat controls. [INFERRED] Class, namespace and source placement.
#ifndef MENU_HUDCHAT_H
#define MENU_HUDCHAT_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../External/BFMeme/Quad.h"
#include <cstdint>
#include <list>
#include <string>
namespace dice { namespace meme { class System; class BfListBoxData; } }
namespace dice { namespace hfe { namespace world { class IPlayer; } } }
namespace dice { namespace hfe { namespace hud {

// Complete26-slot body surface. Publication remains blocked by the real System
// descriptor used in wrapping and the list-resource clear/copy dependency.
// Integers, method names, enum spelling and pointer/reference spelling inferred.
class HudChat
{
public:
    explicit HudChat(meme::System*);
    ~HudChat();                                      // 7A0AF0, nonvirtual
    virtual bool init(std::string name, int, int);
    virtual void update(float);
    virtual void setDuration(int kind, float);
    virtual void setColor(int kind, int variant, float, float, float, float);
    virtual std::string getHistoryInfo();
    virtual void addMessage(std::string text, int kind);
    virtual void addPlayerMessage(std::wstring, world::IPlayer*, int team);
    virtual void addGameMessage(std::wstring, world::IPlayer*, int team);
    virtual void addKillMessage(std::wstring, world::IPlayer*, int team);
    virtual void addRankMessage(std::wstring, world::IPlayer*, int team, int alternateCategory);
    virtual void addPersonalMessage(std::wstring, world::IPlayer*, int team);
    virtual void addNotice(std::wstring, int team);
    virtual void addRequest(std::wstring, int team);
    virtual void appendMessage(std::wstring, int kind, int team, int firstRow,
        int* count, int capacity, float* remaining, int historyKind, world::IPlayer*);
    virtual void wrapMessage(int kind, std::wstring, int team, int firstRow,
        int* count, int capacity, int row);
    virtual int nextRow(int firstRow, int* count, int capacity);
    virtual void resetRows();
    virtual void dropRow(int firstRow, int* count);
    virtual void advanceTimers(float);
    virtual void rebuildRows();
    virtual void clearMessages();
    virtual void setHistoryOutput(bool);
    virtual int getCapacity(int kind);
    virtual int getFirstRow(int kind);
    virtual int* getCount(int kind);
    virtual meme::BfListBoxData* getData();
    void setCapacity(int kind, int);
    void setSpacing(int kind, int);
    int getRowsUsed();
private:
    void selectPlayerColors(int, world::IPlayer*, int);
    void restoreColors(int);
    meme::System* m_system;                           // +04
    meme::Color m_field008[7];                        // +08..74, constructor white
    meme::BfListBoxData* m_data;                       // +78, owned
    std::uint32_t m_field07c[9];                      // [UNRESOLVED], untouched
    int m_firstRow[9];                               // +A0, only 0,1,2,7,8 assigned by rebuild
    int m_capacity[9];                               // +C4
    int m_spacing[9];                                // +E8
    int m_count[9];                                  // +10C, untouched until rebuild
    float m_remaining[9];                            // +130, untouched until rebuild
    float m_duration[9];                             // +154
    meme::Color m_colors[4][9];                       // +178
    bool m_field3b8;                                 // untouched until configuration
    // [UNRESOLVED] four-byte list payload; only empty construction/destruction
    // is used by the recovered table. No pointer or scalar meaning asserted.
    std::list<std::uint32_t> m_field3bc;
    bool m_field3c8, m_field3c9;                       // untouched
    bool m_historyOutput;                            // retail+3CA
    std::uint32_t m_field3cc;                         // [UNRESOLVED] allocation tail, untouched
};
} } }
#endif
#endif
