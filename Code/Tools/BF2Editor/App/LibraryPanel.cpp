// Code/Tools/BF2Editor/App/LibraryPanel.cpp - see LibraryPanel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/LibraryPanel.h"

#include "App/EditSession.h"
#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"
#include "Framework/ObjectLibrary.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cstdio>
#include <cstring>

namespace bf2editor
{

namespace
{

std::string vectorArgument(const float v[3])
{
    char text[128];
    std::snprintf(text, sizeof(text), "%.3f/%.3f/%.3f", v[0], v[1], v[2]);
    return text;
}

const ImVec4 kWarning(1.0f, 0.45f, 0.4f, 1.0f);
const ImVec4 kNote(1.0f, 0.8f, 0.4f, 1.0f);

} // namespace

LibraryPanel::LibraryPanel()
    : m_bundlesListed(false)
    , m_bundleYaw(0.0f)
    , m_seed(1)
    , m_messageFailed(false)
{
    m_bundleName[0] = m_brushName[0] = m_itemTemplate[0] = 0;
}

bool LibraryPanel::run(const std::string& line, const char* what)
{
    const ConsoleLineResult r = runConsoleLine(line);
    logLine(r.failed ? LogLevel::Warning : LogLevel::Info, "%s: %s", what, r.output.c_str());
    m_message = r.output;
    m_messageFailed = r.failed;
    return !r.failed;
}

void LibraryPanel::draw(EditSession& session, bool* open)
{
    // A layout saved before the panel existed (and the first-run layout, which does not name it): open it
    // beside the Terrain panel, never floating over the view.
    ImGuiWindow* neighbour = ImGui::FindWindowByName("Terrain");
    if (!neighbour || neighbour->DockId == 0)
        neighbour = ImGui::FindWindowByName("Level settings");
    if (neighbour && neighbour->DockId != 0)
        ImGui::SetNextWindowDockID(neighbour->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Bundles and brushes", open))
    {
        ImGui::End();
        return;
    }
    ImGui::TextDisabled("Kept in %s", session.library().folder().c_str());
    ImGui::SetItemTooltip("ObjectBundle/ and LevelBrushes/ in this folder, in the retail editor's formats: by default "
                          "its own content folder under the game, so both editors see the same bundles and brushes.");
    if (!m_message.empty())
        ImGui::TextColored(m_messageFailed ? kWarning : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s",
                           m_message.c_str());
    if (ImGui::CollapsingHeader("Object bundles", ImGuiTreeNodeFlags_DefaultOpen))
        drawBundles(session);
    if (ImGui::CollapsingHeader("Level brushes", ImGuiTreeNodeFlags_DefaultOpen))
        drawBrushes(session);
    ImGui::End();
}

void LibraryPanel::drawBundles(EditSession& session)
{
    if (!m_bundlesListed)
    {
        m_bundles = session.library().bundleNames();
        m_bundlesListed = true;
    }
    if (ImGui::SmallButton("Refresh##bundles"))
        m_bundlesListed = false;
    ImGui::SameLine();
    ImGui::TextDisabled("%u bundles", static_cast<unsigned>(m_bundles.size()));
    if (ImGui::BeginListBox("##bundles", ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * 6.0f)))
    {
        for (size_t i = 0; i < m_bundles.size(); ++i)
        {
            if (ImGui::Selectable(m_bundles[i].c_str(), m_bundles[i] == m_selectedBundle))
            {
                m_selectedBundle = m_bundles[i];
                session.bundleInfo(m_selectedBundle, m_bundleInfo);     // the answer or why it cannot be read
            }
        }
        ImGui::EndListBox();
    }
    if (!m_selectedBundle.empty())
    {
        ImGui::TextWrapped("%s", m_bundleInfo.c_str());
        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("Turn", &m_bundleYaw, 1.0f, -360.0f, 360.0f, "%.0f deg");
        ImGui::SetItemTooltip("Turns the whole bundle about the vertical through its root.");
        bool group = session.bundleGrouping();
        if (ImGui::Checkbox("Group the placed objects", &group))
            run(group ? "EditorBundle.group 1" : "EditorBundle.group 0", "bundle");
        ImGui::SetItemTooltip("They get a group of their own (the lowest Object.group no object uses), so Select the "
                              "group finds them again and Ungroup takes them apart.");
        char yaw[32];
        std::snprintf(yaw, sizeof(yaw), "%g", m_bundleYaw);
        if (ImGui::Button("Place at the view centre"))
            run("EditorBundle.placeHere " + m_selectedBundle + " " + yaw, "bundle");
        ImGui::SameLine();
        if (ImGui::Button("Place with a click"))
        {
            session.armBundle(m_selectedBundle, m_bundleYaw);
            m_message = "click the terrain in the view to place " + m_selectedBundle + " (Esc cancels)";
            m_messageFailed = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete##bundle"))
            m_confirmDelete = "bundle:" + m_selectedBundle;
        if (m_confirmDelete == "bundle:" + m_selectedBundle)
        {
            ImGui::TextColored(kNote, "Delete the file of %s?", m_selectedBundle.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Yes, delete it##bundle"))
            {
                if (run("EditorBundle.delete " + m_selectedBundle, "bundle"))
                    m_selectedBundle.clear();
                m_confirmDelete.clear();
                m_bundlesListed = false;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("No##bundle"))
                m_confirmDelete.clear();
        }
    }
    if (!session.armedBundle().empty())
        ImGui::TextColored(kNote, "Click the terrain in the view to place %s (Esc cancels).", session.armedBundle().c_str());

    ImGui::SeparatorText("Save the selection as a bundle");
    ImGui::TextDisabled("Two or more objects; the primary one is the root.");
    ImGui::SetNextItemWidth(-90.0f);
    const bool enter = ImGui::InputTextWithHint("##bundleName", "name (letters, digits, _ and -)", m_bundleName,
                                                sizeof(m_bundleName), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((ImGui::Button("Save##bundle") || enter) && m_bundleName[0])
    {
        run(std::string("EditorBundle.save ") + m_bundleName, "bundle");
        m_bundlesListed = false;
    }
    if (m_messageFailed && m_bundleName[0] && m_message.find("exists already") != std::string::npos
        && m_message.find(m_bundleName) != std::string::npos)
    {
        if (ImGui::SmallButton("Replace it"))
        {
            run(std::string("EditorBundle.saveOver ") + m_bundleName, "bundle");
            m_bundlesListed = false;
        }
    }

    ImGui::SeparatorText("Groups");
    if (ImGui::Button("New group"))
        run("EditorSelection.groupNew", "group");
    ImGui::SetItemTooltip("The selected objects get the lowest Object.group no object uses.");
    ImGui::SameLine();
    if (ImGui::Button("Ungroup"))
        run("EditorSelection.ungroup", "group");
    ImGui::SetItemTooltip("Group 0 for the selected objects: a placed bundle taken apart.");
    ImGui::SameLine();
    if (ImGui::Button("Select the group"))
        run("EditorSelection.selectGroup", "group");
    ImGui::SetItemTooltip("Every object of the primary object's group.");
}

void LibraryPanel::drawBrushes(EditSession& session)
{
    const std::vector<LevelBrushFile>& brushes = session.library().brushes();
    if (ImGui::SmallButton("Read again##brushes"))
    {
        session.library().reloadBrushes();
        run("EditorLibrary.info", "library");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%u brushes", static_cast<unsigned>(brushes.size()));
    const std::vector<std::string>& problems = session.library().brushProblems();
    for (size_t i = 0; i < problems.size(); ++i)
        ImGui::TextColored(kWarning, "%s", problems[i].c_str());
    if (ImGui::BeginListBox("##brushes", ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * 5.0f)))
    {
        for (size_t i = 0; i < brushes.size(); ++i)
        {
            char label[160];
            std::snprintf(label, sizeof(label), "%s (%u items)", brushes[i].name.c_str(),
                          static_cast<unsigned>(brushes[i].items.size()));
            if (ImGui::Selectable(label, _stricmp(brushes[i].name.c_str(), session.activeBrush().c_str()) == 0))
                run("EditorBrush.select " + brushes[i].name, "brush");
        }
        ImGui::EndListBox();
    }

    LevelBrushFile* brush = session.activeBrushFile();
    if (brush)
    {
        const std::string name = brush->name;
        bool tool = session.brushToolActive();
        if (ImGui::Checkbox("Paint tool (B)", &tool))
            run(tool ? "EditorBrush.tool 1" : "EditorBrush.tool 0", "brush");
        ImGui::SetItemTooltip("In the view: the left button paints (a stamp where it goes down, and again each time the "
                              "cursor moves farther than the spacing), Shift+left or the middle button erases this "
                              "brush's objects within the erase radius. Each stroke is one undoable edit.");
        float radius = session.brushEraseRadius();
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::SliderFloat("Erase radius", &radius, 0.5f, 100.0f, "%.1f m", ImGuiSliderFlags_Logarithmic))
        {
            std::string ignored;
            session.setBrushEraseRadius(radius, ignored);
        }
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            char line[64];
            std::snprintf(line, sizeof(line), "EditorBrush.eraseRadius %g", radius);
            run(line, "brush");
        }

        // The brush's settings: edited in place while a field is dragged, written to its file when it lets go.
        ImGui::SetNextItemWidth(160.0f);
        ImGui::DragFloat("Spacing", &brush->spacing, 0.1f, 0.1f, 1000.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            char line[96];
            std::snprintf(line, sizeof(line), "EditorBrush.set spacing %g", brush->spacing);
            run(line, "brush");
        }
        bool snap = brush->snapToGround;
        if (ImGui::Checkbox("Snap to the ground", &snap))
            run(snap ? "EditorBrush.set snapToGround 1" : "EditorBrush.set snapToGround 0", "brush");
        ImGui::SetItemTooltip("Each object stands on the terrain under it (its pivot, until the anchor points are in); "
                              "otherwise it keeps the stamp's height plus its offsets.");
        struct Range { const char* label; const char* key; float* v; };
        const Range ranges[] = {
            { "Random position min", "randomPositionMin", brush->randomPositionMin },
            { "Random position max", "randomPositionMax", brush->randomPositionMax },
            { "Random rotation min", "randomRotationMin", brush->randomRotationMin },
            { "Random rotation max", "randomRotationMax", brush->randomRotationMax },
        };
        for (size_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
        {
            ImGui::SetNextItemWidth(-150.0f);
            ImGui::DragFloat3(ranges[i].label, ranges[i].v, 0.1f);
            if (ImGui::IsItemDeactivatedAfterEdit())
                run("EditorBrush.set " + std::string(ranges[i].key) + " " + vectorArgument(ranges[i].v), "brush");
        }
        if (brush->rotationRangesCopied)
            ImGui::TextColored(kNote, "The rotation ranges repeat the position ranges: the retail editor saved them so. "
                                      "Set the rotation ranges you want; they are saved as given.");

        ImGui::SeparatorText("Items");
        if (ImGui::BeginTable("##items", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("template", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("offset", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("rotation", ImGuiTableColumnFlags_WidthStretch, 1.2f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
            ImGui::TableHeadersRow();
            int remove = -1;
            for (size_t i = 0; i < brush->items.size(); ++i)
            {
                LevelBrushItem& item = brush->items[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.templateName.c_str());
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::DragFloat3("##position", item.position, 0.05f);
                char line[200];
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    std::snprintf(line, sizeof(line), "EditorBrush.item %u position %s", static_cast<unsigned>(i),
                                  vectorArgument(item.position).c_str());
                    run(line, "brush");
                }
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::DragFloat3("##rotation", item.rotation, 0.5f);
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    std::snprintf(line, sizeof(line), "EditorBrush.item %u rotation %s", static_cast<unsigned>(i),
                                  vectorArgument(item.rotation).c_str());
                    run(line, "brush");
                }
                ImGui::TableNextColumn();
                if (ImGui::SmallButton("x"))
                    remove = static_cast<int>(i);
                ImGui::SetItemTooltip("Take this item out of the brush.");
                ImGui::PopID();
            }
            ImGui::EndTable();
            if (remove >= 0)
            {
                char line[64];
                std::snprintf(line, sizeof(line), "EditorBrush.removeItem %d", remove);
                run(line, "brush");
            }
        }
        if (ImGui::Button("Add the selection"))
            run("EditorBrush.addSelection", "brush");
        ImGui::SetItemTooltip("Each selected object's template, with its offset from the primary object and its rotation.");
        ImGui::SetNextItemWidth(-90.0f);
        ImGui::InputTextWithHint("##itemTemplate", "a template name", m_itemTemplate, sizeof(m_itemTemplate));
        ImGui::SameLine();
        if (ImGui::Button("Add item") && m_itemTemplate[0])
            run(std::string("EditorBrush.addItem ") + m_itemTemplate, "brush");
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("##seed", &m_seed);
        ImGui::SameLine();
        if (ImGui::Button("Set the seed"))
        {
            char line[64];
            std::snprintf(line, sizeof(line), "EditorBrush.seed %d", m_seed);
            run(line, "brush");
        }
        ImGui::SetItemTooltip("Where the brushes' random numbers start: the same seed paints the same strokes again.");
        if (ImGui::Button("Delete the brush"))
            m_confirmDelete = "brush:" + name;
        if (m_confirmDelete == "brush:" + name)
        {
            ImGui::TextColored(kNote, "Delete %s and its file?", name.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Yes, delete it##brush"))
            {
                run("EditorBrush.delete " + name, "brush");
                m_confirmDelete.clear();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("No##brush"))
                m_confirmDelete.clear();
        }
    }
    else
        ImGui::TextDisabled("No active brush: pick one above, or define one from the selection.");

    ImGui::SeparatorText("New brush from the selection");
    ImGui::TextDisabled("Each selected object becomes an item: its offset from the primary object and its rotation.");
    ImGui::SetNextItemWidth(-90.0f);
    const bool enter = ImGui::InputTextWithHint("##brushName", "name (letters, digits, _ and -)", m_brushName,
                                                sizeof(m_brushName), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((ImGui::Button("Define") || enter) && m_brushName[0])
        run(std::string("EditorBrush.define ") + m_brushName, "brush");
}

} // namespace bf2editor
