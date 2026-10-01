// Code/Tools/BF2Editor/App/MaterialPanel.cpp - see MaterialPanel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/MaterialPanel.h"

#include "App/EditSession.h"
#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bf2editor
{

namespace
{

const char* const kTypes[] = { "Ground", "World", "Projectiles", "Vehicles", "Misc" };
const ImVec4 kWarning(1.0f, 0.45f, 0.4f, 1.0f);

// What the matrix shows: the damage modifier, or one template slot (kind, index).
struct ShowRow
{
    const char* label;
    int         kind;       // -1 damageMod
    int         index;
};
const ShowRow kShows[] = {
    { "Damage modifier", -1, 0 },
    { "Effect 0 (default)", 0, 0 }, { "Effect 1 (ricochet / crouch / roll)", 0, 1 }, { "Effect 2 (exit / prone / skid)", 0, 2 },
    { "Decal 0 (default)", 1, 0 }, { "Decal 1 (ricochet)", 1, 1 }, { "Decal 2 (exit)", 1, 2 },
    { "Sound 0 (default / stand)", 2, 0 }, { "Sound 1 (crouch)", 2, 1 }, { "Sound 2 (prone)", 2, 2 },
    { "Sound 3 (sprint)", 2, 3 }, { "Sound 4 (death impact)", 2, 4 },
};
const int kShowCount = static_cast<int>(sizeof(kShows) / sizeof(kShows[0]));

std::string quoted(const std::string& text)
{
    return text.find_first_of(" \t") == std::string::npos && !text.empty() ? text : "\"" + text + "\"";
}

std::string lower(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

int typeOf(const MaterialDefineFile& defines, int id)
{
    std::string type;
    defines.value(id, "type", type);
    return std::atoi(type.c_str());
}

} // namespace

MaterialPanel::MaterialPanel()
    : m_material(-1)
    , m_cellA(-1)
    , m_cellB(-1)
    , m_typeFilter(-1)
    , m_attackerType(2)
    , m_defenderType(1)
    , m_show(0)
    , m_templateKind(0)
    , m_templateIndex(0)
    , m_failed(false)
{
    m_filter[0] = m_newName[0] = m_value[0] = m_templateName[0] = 0;
}

bool MaterialPanel::run(const std::string& line)
{
    const ConsoleLineResult r = runConsoleLine(line);
    logLine(r.failed ? LogLevel::Warning : LogLevel::Info, "materials: %s", r.output.c_str());
    m_message = r.output;
    m_failed = r.failed;
    return !r.failed;
}

void MaterialPanel::draw(EditSession& session, bool* open)
{
    ImGuiWindow* neighbour = ImGui::FindWindowByName("Object templates");
    if (!neighbour)
        neighbour = ImGui::FindWindowByName("Properties");
    if (neighbour && neighbour->DockId != 0)
        ImGui::SetNextWindowDockID(neighbour->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Materials", open))
    {
        ImGui::End();
        return;
    }
    std::string error;
    if (!session.loadMaterials(error))
    {
        ImGui::TextColored(kWarning, "%s", error.c_str());
        ImGui::End();
        return;
    }
    MaterialDocuments& documents = session.materials();
    const MaterialDefineFile& defines = documents.defines();
    const MaterialCellFile& cells = documents.cells();
    const std::vector<int> ids = defines.ids();
    const bool readOnly = documents.archived();

    // The files
    ImGui::TextDisabled("%u materials, %u cells; %s%s", static_cast<unsigned>(ids.size()),
                        static_cast<unsigned>(cells.cells().size()),
                        readOnly ? "read from the mounted Common archives" : "loose files", documents.modified() ? "; modified" : "");
    if (readOnly)
    {
        ImGui::TextColored(kWarning, "Read-only: the Common archives shadow any loose copy.");
        if (ImGui::Button("Extract the Common archives"))
            run("EditorMaterial.extract");
        ImGui::SameLine();
        ImGui::TextDisabled("then start the editor with +looseCommon 1");
    }
    else
    {
        ImGui::BeginDisabled(!documents.modified());
        if (ImGui::Button("Save the material files"))
            run("EditorMaterial.save");
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("They also save with the level (File > Save).");
    }
    if (!m_message.empty())
        ImGui::TextColored(m_failed ? kWarning : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s", m_message.c_str());

    // The list
    ImGui::SeparatorText("Materials");
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##filter", "filter by name", m_filter, sizeof(m_filter));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    const char* const typeChoices[] = { "every type", "Ground", "World", "Projectiles", "Vehicles", "Misc" };
    int typeChoice = m_typeFilter + 1;
    if (ImGui::Combo("##type", &typeChoice, typeChoices, 6))
        m_typeFilter = typeChoice - 1;
    const std::string filter = lower(m_filter);
    if (ImGui::BeginChild("##list", ImVec2(0.0f, 160.0f), ImGuiChildFlags_Borders))
    {
        for (size_t i = 0; i < ids.size(); ++i)
        {
            std::string name;
            defines.value(ids[i], "name", name);
            const int type = typeOf(defines, ids[i]);
            if ((m_typeFilter >= 0 && type != m_typeFilter) || (!filter.empty() && lower(name).find(filter) == std::string::npos))
                continue;
            char label[160];
            std::snprintf(label, sizeof(label), "%d  %s  (%s)", ids[i], name.c_str(), type >= 0 && type < 5 ? kTypes[type] : "?");
            if (ImGui::Selectable(label, m_material == ids[i]))
            {
                m_material = ids[i];
                m_editing.clear();
            }
        }
    }
    ImGui::EndChild();
    ImGui::BeginDisabled(readOnly);
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##newName", "new material's name", m_newName, sizeof(m_newName));
    ImGui::SameLine();
    if (ImGui::Button("Add") && m_newName[0])
        run(std::string("EditorMaterial.add ") + quoted(m_newName));
    ImGui::SetItemTooltip("At the first free id of 0..254, as the retail Add; EditorMaterial.addAs gives an id and a type.");
    ImGui::SameLine();
    if (ImGui::Button("Remove") && m_material >= 0)
    {
        char line[64];
        std::snprintf(line, sizeof(line), "EditorMaterial.remove %d", m_material);
        if (run(line))
            m_material = -1;
    }
    ImGui::SetItemTooltip("The selected material and every cell it is in (one undoable edit).");
    ImGui::EndDisabled();

    // The selected material
    if (m_material >= 0 && defines.has(m_material))
    {
        std::string name;
        defines.value(m_material, "name", name);
        char title[128];
        std::snprintf(title, sizeof(title), "Material %d %s", m_material, name.c_str());
        ImGui::SeparatorText(title);
        const std::vector<std::string>& properties = MaterialDefineFile::properties();
        if (ImGui::BeginTable("##properties", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            for (size_t i = 0; i < properties.size(); ++i)
            {
                std::string value;
                defines.value(m_material, properties[i], value);
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(properties[i].c_str());
                ImGui::TableNextColumn();
                if (readOnly)
                    ImGui::TextUnformatted(value.c_str());
                else if (properties[i] == "type")
                {
                    int type = std::atoi(value.c_str());
                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::Combo("##value", &type, kTypes, 5))
                    {
                        char line[96];
                        std::snprintf(line, sizeof(line), "EditorMaterial.set %d type %d", m_material, type);
                        run(line);
                    }
                }
                else
                {
                    const bool editing = m_editing == properties[i];
                    char buffer[128];
                    std::snprintf(buffer, sizeof(buffer), "%s", editing ? m_value : value.c_str());
                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::InputText("##value", buffer, sizeof(buffer)))
                    {
                        std::snprintf(m_value, sizeof(m_value), "%s", buffer);
                        m_editing = properties[i];
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit() && editing)
                    {
                        char line[96];
                        std::snprintf(line, sizeof(line), "EditorMaterial.set %d %s ", m_material, properties[i].c_str());
                        run(line + quoted(m_value));
                        m_editing.clear();
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    // The pair matrix
    ImGui::SeparatorText("Pairs (attacker rows, defender columns)");
    ImGui::SetNextItemWidth(110.0f);
    ImGui::Combo("##attackers", &m_attackerType, kTypes, 5);
    ImGui::SameLine();
    ImGui::TextUnformatted("on");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(110.0f);
    ImGui::Combo("##defenders", &m_defenderType, kTypes, 5);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(210.0f);
    if (ImGui::BeginCombo("##show", kShows[m_show].label))
    {
        for (int i = 0; i < kShowCount; ++i)
            if (ImGui::Selectable(kShows[i].label, m_show == i))
                m_show = i;
        ImGui::EndCombo();
    }
    std::vector<int> rows, columns;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        const int type = typeOf(defines, ids[i]);
        if (type == m_attackerType)
            rows.push_back(ids[i]);
        if (type == m_defenderType)
            columns.push_back(ids[i]);
    }
    if (!rows.empty() && !columns.empty() && columns.size() < 63
        && ImGui::BeginTable("##matrix", static_cast<int>(columns.size()) + 1,
                             ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY
                                 | ImGuiTableFlags_SizingFixedFit,
                             ImVec2(0.0f, 260.0f)))
    {
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableSetupColumn("attacker");
        for (size_t c = 0; c < columns.size(); ++c)
        {
            char header[16];
            std::snprintf(header, sizeof(header), "%d", columns[c]);
            ImGui::TableSetupColumn(header);
        }
        ImGui::TableHeadersRow();
        const ShowRow& show = kShows[m_show];
        for (size_t r = 0; r < rows.size(); ++r)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            std::string rowName;
            defines.value(rows[r], "name", rowName);
            ImGui::Text("%d %s", rows[r], rowName.c_str());
            for (size_t c = 0; c < columns.size(); ++c)
            {
                ImGui::TableNextColumn();
                const int a = rows[r];
                const int b = columns[c];
                std::string text;
                const bool has = show.kind < 0 ? cells.damageMod(a, b, text)
                                               : cells.templateAt(a, b, static_cast<MaterialCellFile::Kind>(show.kind), show.index, text);
                if (!has)
                    text = " ";
                else if (text.empty())
                    text = "-";
                ImGui::PushID(static_cast<int>(r * 1000 + c));
                if (ImGui::Selectable(text.c_str(), m_cellA == a && m_cellB == b))
                {
                    m_cellA = a;
                    m_cellB = b;
                }
                if (ImGui::IsItemHovered())
                {
                    std::string defender;
                    defines.value(b, "name", defender);
                    ImGui::SetTooltip("%s on %s%s", rowName.c_str(), defender.c_str(), has ? "" : " (no cell)");
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    else if (columns.size() >= 63)
        ImGui::TextDisabled("too many defenders of that type to show at once");

    // the selected cell
    if (m_cellA >= 0 && m_cellB >= 0)
    {
        char title[96];
        std::snprintf(title, sizeof(title), "Cell %d %d", m_cellA, m_cellB);
        ImGui::SeparatorText(title);
        ImGui::BeginDisabled(readOnly);
        std::string damage;
        if (!cells.damageMod(m_cellA, m_cellB, damage))
        {
            ImGui::TextDisabled("no cell: the engine falls back to %d 0, then to the default damage", m_cellA);
            if (ImGui::Button("Add the cell"))
            {
                char line[64];
                std::snprintf(line, sizeof(line), "EditorMaterial.addCell %d %d", m_cellA, m_cellB);
                run(line);
            }
        }
        else
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%s", damage.c_str());
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputTextWithHint("damage modifier", "unset", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                char line[128];
                std::snprintf(line, sizeof(line), "EditorMaterial.setDamageMod %d %d %s", m_cellA, m_cellB, buffer[0] ? buffer : "-1");
                run(line);
            }
            ImGui::SetItemTooltip("Enter to set; empty or negative unsets it.");
            const char* const kinds[] = { "effect", "decal", "sound" };
            for (int k = 0; k < 3; ++k)
            {
                const std::vector<int> indexes = cells.indexes(m_cellA, m_cellB, static_cast<MaterialCellFile::Kind>(k));
                for (size_t i = 0; i < indexes.size(); ++i)
                {
                    std::string name;
                    cells.templateAt(m_cellA, m_cellB, static_cast<MaterialCellFile::Kind>(k), indexes[i], name);
                    ImGui::PushID(k * 100 + indexes[i]);
                    ImGui::BulletText("%s %d: %s", kinds[k], indexes[i], name.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("x"))
                    {
                        char line[128];
                        std::snprintf(line, sizeof(line), "EditorMaterial.setTemplate %d %d %s %d \"\"", m_cellA, m_cellB, kinds[k],
                                      indexes[i]);
                        run(line);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::SetNextItemWidth(90.0f);
            ImGui::Combo("##kind", &m_templateKind, kinds, 3);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70.0f);
            ImGui::InputInt("##index", &m_templateIndex);
            if (m_templateIndex < 0)
                m_templateIndex = 0;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::InputTextWithHint("##template", "template name", m_templateName, sizeof(m_templateName));
            ImGui::SameLine();
            if (ImGui::Button("Set template") && m_templateName[0])
            {
                char line[256];
                std::snprintf(line, sizeof(line), "EditorMaterial.setTemplate %d %d %s %d %s", m_cellA, m_cellB, kinds[m_templateKind],
                              m_templateIndex, m_templateName);
                run(line);
            }
            if (ImGui::Button("Remove the cell"))
            {
                char line[64];
                std::snprintf(line, sizeof(line), "EditorMaterial.removeCell %d %d", m_cellA, m_cellB);
                run(line);
            }
        }
        ImGui::EndDisabled();
    }
    ImGui::End();
}

} // namespace bf2editor
