// Animation-system browser and editor with an undoable 24-fps timeline. Changes use EditorAnimation
// commands and save with the level. Timeline scrubbing changes timing only, without pose preview.
#ifndef BF2EDITOR_APP_ANIMATIONPANEL_H
#define BF2EDITOR_APP_ANIMATIONPANEL_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

class EditSession;

class AnimationPanel
{
public:
    AnimationPanel();
    void draw(EditSession& session, bool* open);

private:
    bool run(const std::string& line);
    void drawPicker(EditSession& session);
    void drawTree(EditSession& session);
    void drawSelected(EditSession& session);
    void drawSystem(EditSession& session);
    void drawTimeline(EditSession& session);
    void select(const char* kind, const std::string& name);

    char        m_path[260];
    char        m_value[128];
    std::string m_editing;          // the property whose buffer holds the user's typing
    char        m_newName[96];
    int         m_newType;          // the trigger type for Add trigger
    char        m_newValues[96];    // a new value holder's "min normal max"
    char        m_bafPath[260];
    char        m_eventName[64];
    char        m_linkName[96];
    std::vector<std::string> m_systems;     // the engine's list, refreshed on demand
    bool        m_systemsRead;
    // The timeline.
    float       m_pixelsPerSecond;
    float       m_cursor;           // seconds
    bool        m_playing;
    int         m_dragKind;         // 0 none, 1 a bar's start, 2 a bar's length, 3 an event
    int         m_dragIndex;
    float       m_dragFrom;         // the item's time (or length) when the drag began
    float       m_dragMouse;        // the mouse's x then
    float       m_dragValue;        // what the drag would set
    std::string m_message;
    bool        m_failed;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_ANIMATIONPANEL_H
