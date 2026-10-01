// Code/Tools/BF2Editor/App/MaterialPanel.h

// The "Materials" panel: the physics material
// database of Common/Material/ - a filtered material list, the selected material's properties, the pair matrix
// for any attacker and defender types (no retail restriction, census §7.5) showing the damage modifier or one
// template slot, and the selected cell's damage modifier and templates. Every change runs its EditorMaterial
// console line, so each is one undoable edit; the files save with the level. Read-only while the Common
// archives are mounted (the panel says how to edit). Editor-only code (D3).
#ifndef BF2EDITOR_APP_MATERIALPANEL_H
#define BF2EDITOR_APP_MATERIALPANEL_H

#include "Framework/EditorConfig.h"

#include <string>

namespace bf2editor
{

class EditSession;

class MaterialPanel
{
public:
    MaterialPanel();
    void draw(EditSession& session, bool* open);

private:
    bool run(const std::string& line);

    int         m_material;         // the selected material, -1 for none
    int         m_cellA;            // the selected cell
    int         m_cellB;
    int         m_typeFilter;       // -1 every type
    int         m_attackerType;     // the matrix's rows and columns, by type
    int         m_defenderType;
    int         m_show;             // the matrix shows: 0 damageMod, then effect / decal / sound 0..4
    char        m_filter[64];
    char        m_newName[64];
    char        m_value[128];
    std::string m_editing;          // the property being edited
    char        m_templateName[128];
    int         m_templateKind;
    int         m_templateIndex;
    std::string m_message;
    bool        m_failed;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_MATERIALPANEL_H
