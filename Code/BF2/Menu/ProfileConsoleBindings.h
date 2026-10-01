// [INFERRED] names/path. Client Profile console storage and registration only.
#ifndef MENU_PROFILECONSOLEBINDINGS_H
#define MENU_PROFILECONSOLEBINDINGS_H
#include "../Core/ConsoleObjects.h"
#include <cstdint>
#if !DEDICATED
namespace dice { namespace hfe { namespace hud {

class CharacterProfile; // [INFERRED] object allocated by7C0440, ctor7C1270
struct ProfileConsoleFixture;

// Complete native storage proved by7C0530/7C0440/7C0370. This declaration permits
// typed reads of the selected/active character pointers; it publishes no profile
// constructor or destructor. The two word names/signedness remain unresolved.
class PlayerProfile {
public:
    CharacterProfile* getSelectedCharacter() const { return m_selectedCharacter; }
    CharacterProfile* getActiveCharacter() const { return m_activeCharacter; }
private:
    PlayerProfile(const PlayerProfile&) = delete; // construction withheld
    ~PlayerProfile();                            // [FRONTIER] lifetime withheld
    std::string m_name;                          // retail+00, native string
    std::uint32_t m_field1c;
    std::uint32_t m_field20;
    CharacterProfile* m_selectedCharacter;       // retail+24
    CharacterProfile* m_activeCharacter;         // retail+28
    std::map<std::string, CharacterProfile*> m_characters; // retail+2C
};

class ProfileConsoleOwner {
public:
    ProfileConsoleOwner();
    ~ProfileConsoleOwner();
    PlayerProfile* getSelectedProfile() const { return m_selectedProfile; }
    PlayerProfile* getActiveProfile() const { return m_activeProfile; }
private:
    friend struct ProfileConsoleFixture;
    std::string m_emptyName;                     // retail+00
    PlayerProfile* m_selectedProfile;            // retail+1C
    PlayerProfile* m_activeProfile;              // retail+20
    std::map<std::string, PlayerProfile*> m_profiles; // retail+24; pointer values not deleted
};

extern ProfileConsoleOwner* profileConsoleOwner; // A22158, initially null
ConsoleObjects::ConsoleObjectList& profileConsoleBindingList();

} } }
#endif
#endif
