// Code/Tools/BF2Editor/App/WizardPanel.h

// The "Object wizard" panel: the category (General, Car, Tank, Plane,
// Helicopter) and Run Wizard over the selected object's tree; the findings as ticked or read-only items under
// Errors, Warnings and Utils; Execute; and, while Execute waits, its prompt - text, a number, a choice - or a
// ModifyObjectList checklist. Every action runs its EditorWizard console line. Editor-only code (D3).
#ifndef BF2EDITOR_APP_WIZARDPANEL_H
#define BF2EDITOR_APP_WIZARDPANEL_H

#include "Framework/EditorConfig.h"

#include <string>

namespace bf2editor
{

class EditSession;

class WizardPanel
{
public:
    WizardPanel();
    void draw(EditSession& session, bool* open);

private:
    bool run(const std::string& line);

    int         m_category;
    char        m_answer[256];
    int         m_choice;
    std::string m_message;
    bool        m_failed;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_WIZARDPANEL_H
