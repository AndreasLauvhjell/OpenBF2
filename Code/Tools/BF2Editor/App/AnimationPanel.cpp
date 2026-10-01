// Code/Tools/BF2Editor/App/AnimationPanel.cpp - see AnimationPanel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/AnimationPanel.h"

#include "App/EditSession.h"
#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"

#include "IO/File/FileManager.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <set>

namespace bf2editor
{

namespace
{

const ImVec4 kWarning(1.0f, 0.45f, 0.4f, 1.0f);
const float kFps = 24.0f;       // the timeline's rate: BF2 animations are 24 frames a second (census §5.1)

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

std::string shortName(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string folderOf(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

float snapFrame(float seconds)
{
    return std::floor(seconds * kFps + 0.5f) / kFps;
}

std::string seconds(float value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%g", value);
    return text;
}

// The file's value of a property of an object, "" when it sets none.
std::string fileValue(const AnimationSystemFile& file, AnimationKind kind, const std::string& name, const std::string& property)
{
    const std::vector<AnimationProperty> lines = file.properties(kind, name);
    for (size_t i = 0; i < lines.size(); ++i)
        if (_stricmp(lines[i].property.c_str(), property.c_str()) == 0)
            return lines[i].value.empty() ? std::string(" ") : lines[i].value;
    return std::string();
}

} // namespace

AnimationPanel::AnimationPanel()
    : m_newType(0)
    , m_systemsRead(false)
    , m_pixelsPerSecond(240.0f)
    , m_cursor(0.0f)
    , m_playing(false)
    , m_dragKind(0)
    , m_dragIndex(-1)
    , m_dragFrom(0.0f)
    , m_dragMouse(0.0f)
    , m_dragValue(0.0f)
    , m_failed(false)
{
    m_path[0] = m_value[0] = m_newName[0] = m_bafPath[0] = m_eventName[0] = m_linkName[0] = 0;
    std::snprintf(m_newValues, sizeof(m_newValues), "0.25 1 10");
}

bool AnimationPanel::run(const std::string& line)
{
    const ConsoleLineResult r = runConsoleLine(line);
    logLine(r.failed ? LogLevel::Warning : LogLevel::Info, "animations: %s", r.output.c_str());
    m_message = r.output;
    m_failed = r.failed;
    return !r.failed;
}

void AnimationPanel::select(const char* kind, const std::string& name)
{
    run(std::string("EditorAnimation.select ") + kind + " " + quoted(name));
    m_editing.clear();
}

void AnimationPanel::draw(EditSession& session, bool* open)
{
    ImGuiWindow* neighbour = ImGui::FindWindowByName("Materials");
    if (!neighbour)
        neighbour = ImGui::FindWindowByName("Object templates");
    if (neighbour && neighbour->DockId != 0)
        ImGui::SetNextWindowDockID(neighbour->DockId, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Animations", open))
    {
        ImGui::End();
        return;
    }
    drawPicker(session);
    if (session.animationCurrent() >= 0)
    {
        ImGui::Separator();
        const float timelineHeight = 150.0f;
        float listHeight = ImGui::GetContentRegionAvail().y - timelineHeight - ImGui::GetFrameHeightWithSpacing() * 2.0f;
        if (listHeight < 160.0f)
            listHeight = 160.0f;
        // Side by side when there is room; the tree above the settings in a narrow dock.
        const bool narrow = ImGui::GetContentRegionAvail().x < 640.0f;
        if (ImGui::BeginChild("##animationTree", narrow ? ImVec2(0.0f, listHeight * 0.4f)
                                                        : ImVec2(ImGui::GetContentRegionAvail().x * 0.42f, listHeight),
                              ImGuiChildFlags_Borders))
            drawTree(session);
        ImGui::EndChild();
        if (!narrow)
            ImGui::SameLine();
        if (ImGui::BeginChild("##animationSelected", ImVec2(0.0f, narrow ? listHeight * 0.6f : listHeight), ImGuiChildFlags_Borders))
        {
            drawSelected(session);
            ImGui::Separator();
            drawSystem(session);
        }
        ImGui::EndChild();
        drawTimeline(session);
    }
    if (!m_message.empty())
    {
        if (m_failed)
            ImGui::TextColored(kWarning, "%s", m_message.c_str());
        else
            ImGui::TextDisabled("%s", m_message.c_str());
    }
    ImGui::End();
}

// The system

void AnimationPanel::drawPicker(EditSession& session)
{
    if (!m_systemsRead)
    {
        m_systems = session.animationSystemList();
        m_systemsRead = true;
    }
    AnimationDocuments& documents = session.animations();
    const int current = session.animationCurrent();
    const std::string preview = current >= 0 ? documents.document(static_cast<size_t>(current)).path : std::string("(none)");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    if (ImGui::BeginCombo("System", preview.c_str(), ImGuiComboFlags_HeightLarge))
    {
        if (ImGui::Selectable("(refresh the level's systems)"))
            m_systems = session.animationSystemList();
        for (size_t i = 0; i < m_systems.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(m_systems[i].c_str(), current >= 0 && _stricmp(m_systems[i].c_str(), preview.c_str()) == 0))
                run("EditorAnimation.open " + quoted(m_systems[i]));
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::SetItemTooltip("The systems the level's templates registered with the engine (EditorAnimation.systems).");
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    const bool enter = ImGui::InputTextWithHint("##path", "or a system's path (an .inc)", m_path, sizeof(m_path),
                                                ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((ImGui::Button("Open") || enter) && m_path[0])
        run(std::string("EditorAnimation.open ") + quoted(m_path));
    if (current < 0)
        return;
    const AnimationDocument& document = documents.document(static_cast<size_t>(current));
    ImGui::TextDisabled("%s; %s", document.archived ? "read from a mounted archive" : ("loose, saved to " + document.writePath).c_str(),
                        document.modified() ? "modified" : "unchanged");
    if (document.archived)
        ImGui::TextColored(kWarning, "Read-only: extract the Objects archives (EditorObjects.extract), then start with +looseObjects 1.");
    else
    {
        ImGui::BeginDisabled(!session.animationsModified());
        if (ImGui::Button("Save the animation systems"))
            run("EditorAnimation.save");
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("They also save with the level (File > Save).");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    ImGui::SetItemTooltip("Edits change the files; the engine reads them when the level loads again (a playing system\n"
                          "cannot take them safely, and a live preview needs the local player).");
}

void AnimationPanel::drawTree(EditSession& session)
{
    const AnimationSystemFile& file = session.animations().document(static_cast<size_t>(session.animationCurrent())).file;
    const std::string& kind = session.animationSelectedKind();
    const std::string& selected = session.animationSelectedName();
    struct Walker
    {
        AnimationPanel* panel;
        const AnimationSystemFile* file;
        const std::string* kind;
        const std::string* selected;
        void trigger(const std::string& name, std::set<std::string>& path)
        {
            const AnimationObject* o = file->find(AnimationKind::Trigger, name);
            const std::vector<std::string> children = o ? file->triggerChildren(o->name) : std::vector<std::string>();
            const std::vector<std::string> bundles = o ? file->triggerBundles(o->name) : std::vector<std::string>();
            const bool loop = !path.insert(lower(name)).second;
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if ((children.empty() && bundles.empty()) || loop)
                flags |= ImGuiTreeNodeFlags_Leaf;
            if (*kind == "trigger" && _stricmp(selected->c_str(), name.c_str()) == 0)
                flags |= ImGuiTreeNodeFlags_Selected;
            const std::string label = name + "  (" + (o ? o->type : std::string("missing")) + ")" + (loop ? " (loop)" : "");
            const bool open = ImGui::TreeNodeEx((name + "##t" + std::to_string(path.size())).c_str(), flags, "%s", label.c_str());
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && o)
                panel->select("trigger", o->name);
            if (open)
            {
                if (!loop)
                {
                    for (size_t i = 0; i < bundles.size(); ++i)
                    {
                        ImGuiTreeNodeFlags leaf = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
                        if (*kind == "bundle" && _stricmp(selected->c_str(), bundles[i].c_str()) == 0)
                            leaf |= ImGuiTreeNodeFlags_Selected;
                        ImGui::TreeNodeEx((bundles[i] + "##b" + name).c_str(), leaf, "[bundle] %s", bundles[i].c_str());
                        if (ImGui::IsItemClicked())
                            panel->select("bundle", bundles[i]);
                    }
                    for (size_t i = 0; i < children.size(); ++i)
                        trigger(children[i], path);
                }
                ImGui::TreePop();
            }
            if (!loop)
                path.erase(lower(name));
        }
    };
    Walker walker = { this, &file, &kind, &selected };
    if (ImGui::CollapsingHeader("Triggers", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const std::vector<std::string> roots = file.triggerRoots();
        for (size_t i = 0; i < roots.size(); ++i)
        {
            std::set<std::string> path;
            walker.trigger(roots[i], path);
        }
        if (roots.empty())
            ImGui::TextDisabled("(none)");
    }
    const AnimationKind lists[3] = { AnimationKind::Bundle, AnimationKind::Animation, AnimationKind::ValueHolder };
    const char* const headers[3] = { "Bundles", "Animations (with settings)", "Value holders" };
    for (int l = 0; l < 3; ++l)
    {
        if (!ImGui::CollapsingHeader(headers[l]))
            continue;
        const AnimationSystemFile* in = &file;
        if (lists[l] == AnimationKind::ValueHolder)
        {
            // The process-wide file (ValueHolders.inc), once the session has opened it.
            AnimationDocuments& documents = session.animations();
            bool any = false;
            for (size_t d = 0; d < documents.count(); ++d)
                for (size_t i = 0; i < documents.document(d).file.objects().size(); ++i)
                {
                    const AnimationObject& o = documents.document(d).file.objects()[i];
                    if (o.kind != AnimationKind::ValueHolder)
                        continue;
                    any = true;
                    if (ImGui::Selectable((o.name + "##v").c_str(), kind == "valueholder" && selected == o.name))
                        select("valueholder", o.name);
                }
            if (!any && ImGui::SmallButton("Read the value holders"))
                run("EditorAnimation.list valueholder");
            continue;
        }
        for (size_t i = 0; i < in->objects().size(); ++i)
        {
            const AnimationObject& o = in->objects()[i];
            if (o.kind != lists[l])
                continue;
            const char* k = AnimationSystemFile::kindName(o.kind);
            const std::string label = o.kind == AnimationKind::Animation ? shortName(o.name) : o.name;
            if (ImGui::Selectable((label + "##" + k + std::to_string(i)).c_str(),
                                  kind == k && _stricmp(selected.c_str(), o.name.c_str()) == 0))
                select(k, o.name);
            if (o.kind == AnimationKind::Animation)
                ImGui::SetItemTooltip("%s", o.name.c_str());
        }
    }
}

void AnimationPanel::drawSelected(EditSession& session)
{
    AnimationDocuments& documents = session.animations();
    const AnimationSystemFile& file = documents.document(static_cast<size_t>(session.animationCurrent())).file;
    const std::string kindName = session.animationSelectedKind();
    const std::string name = session.animationSelectedName();
    AnimationKind kind = AnimationKind::Bundle;
    if (name.empty() || !AnimationSystemFile::kindFromName(kindName, kind))
    {
        ImGui::TextDisabled("Select a trigger, bundle, animation or value holder.");
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputTextWithHint("##newRoot", "name", m_newName, sizeof(m_newName));
        const std::vector<std::string> types = EditSession::animationTriggerTypes();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        if (m_newType >= static_cast<int>(types.size()))
            m_newType = 0;
        if (ImGui::BeginCombo("##newRootType", types[static_cast<size_t>(m_newType)].c_str()))
        {
            for (size_t i = 0; i < types.size(); ++i)
                if (ImGui::Selectable(types[i].c_str(), static_cast<int>(i) == m_newType))
                    m_newType = static_cast<int>(i);
            ImGui::EndCombo();
        }
        ImGui::BeginDisabled(!m_newName[0]);
        if (ImGui::Button("Add a root trigger"))
            run("EditorAnimation.addTrigger " + types[static_cast<size_t>(m_newType)] + " " + quoted(m_newName));
        ImGui::SameLine();
        if (ImGui::Button("Add a bundle"))
            run(std::string("EditorAnimation.addBundle ") + quoted(m_newName));
        ImGui::EndDisabled();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputTextWithHint("##newHolderValues", "min normal max", m_newValues, sizeof(m_newValues));
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_newName[0]);
        if (ImGui::Button("Add a value holder"))
            run(std::string("EditorAnimation.addValueHolder ") + quoted(m_newName) + " " + quoted(m_newValues));
        ImGui::EndDisabled();
        return;
    }
    // The value holders live in their own file.
    const AnimationSystemFile* in = &file;
    if (kind == AnimationKind::ValueHolder)
        for (size_t d = 0; d < documents.count(); ++d)
            if (documents.document(d).file.find(AnimationKind::ValueHolder, name))
                in = &documents.document(d).file;
    const AnimationObject* o = in->find(kind, name);
    const std::string type = o ? o->type : std::string();
    ImGui::Text("%s %s%s", AnimationSystemFile::kindName(kind), kind == AnimationKind::Animation ? shortName(name).c_str() : name.c_str(),
                type.empty() ? "" : ("  (" + type + ")").c_str());
    if (kind == AnimationKind::Animation)
    {
        float own = -1.0f;
        int joints = -1;
        if (session.bafInfo(name, own, joints))
            ImGui::TextDisabled("%s: %g s at 24 frames a second, %d joints", name.c_str(), own, joints);
        else
            ImGui::TextColored(kWarning, "%s: the engine cannot read it", name.c_str());
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Deselect"))
        run("EditorAnimation.select none none");

    // Its settings
    const std::vector<EditSession::AnimationPropertyInfo> rows = EditSession::animationProperties(kind, type);
    if (ImGui::BeginTable("##animationProperties", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("property", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("value");
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0f);
        for (size_t i = 0; i < rows.size(); ++i)
        {
            ImGui::TableNextRow();
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(rows[i].name.c_str());
            ImGui::TableNextColumn();
            const std::string value = fileValue(*in, kind, name, rows[i].name);
            ImGui::SetNextItemWidth(-1.0f);
            if (m_editing != rows[i].name)
            {
                if (!value.empty())
                    std::snprintf(m_value, sizeof(m_value), "%s", value.c_str());
                char shown[128];
                std::snprintf(shown, sizeof(shown), "%s", value.c_str());
                if (ImGui::InputTextWithHint("##v", ("engine: " + rows[i].fallback).c_str(), shown, sizeof(shown),
                                             ImGuiInputTextFlags_EnterReturnsTrue))
                    run("EditorAnimation.set " + rows[i].name + " " + quoted(shown));
                if (ImGui::IsItemActivated())
                {
                    m_editing = rows[i].name;
                    std::snprintf(m_value, sizeof(m_value), "%s", value.c_str());
                }
            }
            else
            {
                if (ImGui::InputText("##v", m_value, sizeof(m_value), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    run("EditorAnimation.set " + rows[i].name + " " + quoted(m_value));
                    m_editing.clear();
                }
                else if (ImGui::IsItemDeactivated())
                    m_editing.clear();
            }
            ImGui::TableNextColumn();
            if (!value.empty() && ImGui::SmallButton("x"))
                run("EditorAnimation.unset " + rows[i].name);
            if (!value.empty())
                ImGui::SetItemTooltip("Take the line out (the engine's %s then)", rows[i].fallback.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    // The object's other lines (its links and items), read-only here; the buttons below edit them.
    const std::vector<AnimationProperty> lines = in->properties(kind, name);
    std::string other;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        bool row = false;
        for (size_t k = 0; k < rows.size() && !row; ++k)
            row = _stricmp(rows[k].name.c_str(), lines[i].property.c_str()) == 0;
        if (!row)
            other += lines[i].property + " " + lines[i].value + "\n";
    }
    if (!other.empty())
        ImGui::TextDisabled("%s", other.c_str());

    // The edits
    if (kind == AnimationKind::Trigger)
    {
        const std::vector<std::string> types = EditSession::animationTriggerTypes();
        if (m_newType >= static_cast<int>(types.size()))
            m_newType = 0;
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputTextWithHint("##childName", "name", m_newName, sizeof(m_newName));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::BeginCombo("##childType", types[static_cast<size_t>(m_newType)].c_str()))
        {
            for (size_t i = 0; i < types.size(); ++i)
                if (ImGui::Selectable(types[i].c_str(), static_cast<int>(i) == m_newType))
                    m_newType = static_cast<int>(i);
            ImGui::EndCombo();
        }
        ImGui::BeginDisabled(!m_newName[0]);
        ImGui::SameLine();
        if (ImGui::Button("Add a child trigger"))
            run("EditorAnimation.addTrigger " + types[static_cast<size_t>(m_newType)] + " " + quoted(m_newName));
        ImGui::SameLine();
        if (ImGui::Button("Add a bundle it plays"))
            run(std::string("EditorAnimation.addBundle ") + quoted(m_newName));
        ImGui::EndDisabled();
        // Links to objects that exist.
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::BeginCombo("##link", m_linkName[0] ? m_linkName : "an existing trigger or bundle"))
        {
            for (size_t i = 0; i < file.objects().size(); ++i)
            {
                const AnimationObject& x = file.objects()[i];
                if (x.kind != AnimationKind::Trigger && x.kind != AnimationKind::Bundle)
                    continue;
                const std::string label = std::string(x.kind == AnimationKind::Trigger ? "trigger " : "bundle ") + x.name;
                if (ImGui::Selectable(label.c_str()))
                    std::snprintf(m_linkName, sizeof(m_linkName), "%s", label.c_str());
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_linkName[0]);
        const std::string link = m_linkName;
        const bool child = link.compare(0, 8, "trigger ") == 0;
        const std::string linked = link.empty() ? std::string() : link.substr(link.find(' ') + 1);
        if (ImGui::Button("Link"))
            run(std::string("EditorAnimation.link ") + (child ? "child " : "bundle ") + quoted(linked));
        ImGui::SameLine();
        if (ImGui::Button("Unlink"))
            run(std::string("EditorAnimation.unlink ") + (child ? "child " : "bundle ") + quoted(linked));
        ImGui::EndDisabled();
    }
    else if (kind == AnimationKind::Bundle)
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
        ImGui::InputTextWithHint("##baf", "an animation (.baf path)", m_bafPath, sizeof(m_bafPath));
        ImGui::SameLine();
        // The .baf files beside the bundle's animations, the usual place for another.
        const std::vector<BundleItem> items = file.bundleAnimations(name);
        const std::string folder = !items.empty() ? folderOf(items[0].name) : folderOf(m_bafPath);
        if (ImGui::BeginCombo("##pickBaf", "pick", ImGuiComboFlags_NoPreview | ImGuiComboFlags_HeightLarge))
        {
            std::list<dice::hfe::io::FileInfo> found;
            if (dice::hfe::io::fileManager && !folder.empty())
                dice::hfe::io::fileManager->findFiles(folder + "/", false, found);
            for (std::list<dice::hfe::io::FileInfo>::const_iterator it = found.begin(); it != found.end(); ++it)
            {
                if (it->isDirectory || it->name.size() < 5 || _stricmp(it->name.c_str() + it->name.size() - 4, ".baf") != 0)
                    continue;
                const std::string path = folder + "/" + shortName(it->name);
                if (ImGui::Selectable(shortName(it->name).c_str()))
                    std::snprintf(m_bafPath, sizeof(m_bafPath), "%s", path.c_str());
            }
            if (found.empty())
                ImGui::TextDisabled("no folder to look in: type a path");
            ImGui::EndCombo();
        }
        ImGui::BeginDisabled(!m_bafPath[0]);
        if (ImGui::Button("Add the animation"))
            run(std::string("EditorAnimation.addAnimation ") + quoted(m_bafPath));
        ImGui::EndDisabled();
        ImGui::SetNextItemWidth(150.0f);
        ImGui::InputTextWithHint("##event", "an event", m_eventName, sizeof(m_eventName));
        ImGui::SameLine();
        if (ImGui::BeginCombo("##eventPreset", "preset", ImGuiComboFlags_NoPreview))
        {
            // The plugin offered effect:0 .. effect:9; the engine also reads sound:<n> (Bundle::BundleEvent).
            for (int k = 0; k < 2; ++k)
                for (int n = 0; n < 10; ++n)
                {
                    char text[32];
                    std::snprintf(text, sizeof(text), "%s:%d", k == 0 ? "effect" : "sound", n);
                    if (ImGui::Selectable(text))
                        std::snprintf(m_eventName, sizeof(m_eventName), "%s", text);
                }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_eventName[0]);
        if (ImGui::Button("Add the event"))
            run(std::string("EditorAnimation.addEvent ") + quoted(m_eventName));
        ImGui::EndDisabled();
        ImGui::TextDisabled("Its animations and events: the timeline below (drag to time them; right-click to take one out).");
    }
    ImGui::Spacing();
    if (ImGui::Button(kind == AnimationKind::Trigger ? "Delete the trigger (and the triggers under it)"
                      : kind == AnimationKind::Animation ? "Delete the animation's settings" : "Delete"))
        run("EditorAnimation.remove");
}

void AnimationPanel::drawSystem(EditSession& session)
{
    if (!ImGui::CollapsingHeader("Camera spring (first person)"))
        return;
    const AnimationSystemFile& file = session.animations().document(static_cast<size_t>(session.animationCurrent())).file;
    const std::vector<AnimationProperty> lines = file.systemLines();
    const std::vector<std::string> rows = EditSession::animationSystemRows();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        std::string value;
        for (size_t k = 0; k < lines.size(); ++k)
            if (_stricmp(lines[k].property.c_str(), rows[i].c_str()) == 0)
                value = lines[k].value;
        ImGui::PushID(static_cast<int>(i) + 1000);
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::InputTextWithHint(rows[i].c_str(), "not set", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
            run(buffer[0] ? "EditorAnimation.setSystem " + rows[i] + " " + quoted(buffer) : "EditorAnimation.unsetSystem " + rows[i]);
        ImGui::PopID();
    }
    ImGui::TextDisabled("Vectors as x/y/z (look x/y); the spring's rows count only after cameraSpring.use 1.");
}

// The timeline

void AnimationPanel::drawTimeline(EditSession& session)
{
    std::string bundle;
    if (session.animationSelectedKind() == "bundle")
        bundle = session.animationSelectedName();
    EditSession::AnimationTimeline timeline;
    std::string error;
    if (bundle.empty() || !session.animationTimeline(bundle, timeline, error))
    {
        ImGui::TextDisabled("Timeline: select a bundle.");
        return;
    }
    const float span = timeline.span > 0.0f ? timeline.span : 1.0f;
    // The transport: a cursor over the bundle's time (no pose preview - that needs the local player, E9).
    if (m_playing)
    {
        m_cursor += ImGui::GetIO().DeltaTime;
        if (m_cursor > span)
            m_cursor = std::fmod(m_cursor, span);
    }
    if (ImGui::SmallButton(m_playing ? "Pause" : "Play"))
        m_playing = !m_playing;
    ImGui::SameLine();
    if (ImGui::SmallButton("Stop"))
    {
        m_playing = false;
        m_cursor = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("<"))
    {
        m_playing = false;
        m_cursor = snapFrame(m_cursor) - 1.0f / kFps;
        if (m_cursor < 0.0f)
            m_cursor = snapFrame(span) - 1.0f / kFps;      // before the first wraps to last - 1, as retail
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">"))
    {
        m_playing = false;
        m_cursor = snapFrame(m_cursor) + 1.0f / kFps;
        if (m_cursor > span)
            m_cursor = 0.0f;                                // past the last wraps to the first, as retail
    }
    ImGui::SameLine();
    ImGui::Text("%s: frame %d of %d (%.3f s)%s", timeline.bundle.c_str(), static_cast<int>(std::floor(m_cursor * kFps + 0.5f)),
                static_cast<int>(std::floor(span * kFps + 0.5f)), m_cursor, timeline.lengthSet ? ", length set" : "");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("##zoom", &m_pixelsPerSecond, 40.0f, 1200.0f, "%.0f px/s", ImGuiSliderFlags_Logarithmic);
    ImGui::SetItemTooltip("The cursor scrubs the bundle's timing; a pose preview needs the local player (E9).");

    const float rowHeight = 18.0f;
    const int rows = static_cast<int>(timeline.bars.size()) + (timeline.points.empty() ? 0 : 1);
    const float height = 22.0f + rowHeight * static_cast<float>(rows > 0 ? rows : 1);
    const float width = span * m_pixelsPerSecond + 60.0f;
    if (!ImGui::BeginChild("##timeline", ImVec2(0.0f, height + 18.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::EndChild();
        return;
    }
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##timelineCanvas", ImVec2(width, height));
    const bool canvasHovered = ImGui::IsItemHovered();
    const float left = origin.x + 8.0f;
    const ImU32 ruler = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    // The ruler: a tick a frame when there is room, a label each second.
    const float frameWidth = m_pixelsPerSecond / kFps;
    const int frames = static_cast<int>(std::ceil(span * kFps));
    for (int f = 0; f <= frames; ++f)
    {
        const float x = left + static_cast<float>(f) * frameWidth;
        const bool second = f % 24 == 0;
        if (!second && frameWidth < 4.0f)
            continue;
        draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + (second ? 10.0f : 5.0f)), ruler);
        if (second)
        {
            char text[16];
            std::snprintf(text, sizeof(text), "%ds", f / 24);
            draw->AddText(ImVec2(x + 2.0f, origin.y), ruler, text);
        }
    }
    const float top = origin.y + 18.0f;
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    // Dragging: started on a bar, its right edge or a marker; the edit runs when the button comes up.
    if (m_dragKind != 0)
    {
        const float delta = (mouse.x - m_dragMouse) / m_pixelsPerSecond;
        m_dragValue = snapFrame(m_dragFrom + delta);
        if (m_dragKind == 2 && m_dragValue < 1.0f / kFps)
            m_dragValue = 1.0f / kFps;                     // never 0: the engine divides by a length
        if (m_dragValue < 0.0f)
            m_dragValue = 0.0f;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const size_t index = static_cast<size_t>(m_dragIndex);
            if (m_dragKind == 1 && index < timeline.bars.size() && m_dragValue != m_dragFrom)
                run("EditorAnimation.setStart animation " + quoted(timeline.bars[index].name) + " " + seconds(m_dragValue));
            else if (m_dragKind == 2 && index < timeline.bars.size() && m_dragValue != m_dragFrom)
                run("EditorAnimation.setLength " + quoted(timeline.bars[index].name) + " " + seconds(m_dragValue));
            else if (m_dragKind == 3 && index < timeline.points.size() && m_dragValue != m_dragFrom)
                run("EditorAnimation.setStart event " + quoted(timeline.points[index].name) + " " + seconds(m_dragValue));
            m_dragKind = 0;
            m_dragIndex = -1;
        }
    }
    for (size_t i = 0; i < timeline.bars.size(); ++i)
    {
        const EditSession::AnimationBar& bar = timeline.bars[i];
        float start = bar.start;
        float length = bar.length > 0.0f ? bar.length : 1.0f / kFps;
        if (m_dragKind == 1 && m_dragIndex == static_cast<int>(i))
            start = m_dragValue;
        if (m_dragKind == 2 && m_dragIndex == static_cast<int>(i))
            length = m_dragValue;
        const float y = top + rowHeight * static_cast<float>(i);
        const ImVec2 a(left + start * m_pixelsPerSecond, y + 2.0f);
        const ImVec2 b(left + (start + length) * m_pixelsPerSecond, y + rowHeight - 2.0f);
        const bool active = m_cursor >= start && m_cursor <= start + length;
        draw->AddRectFilled(a, b, active ? IM_COL32(90, 150, 230, 255) : IM_COL32(70, 110, 170, 220), 3.0f);
        if (bar.original > 0.0f && std::fabs(bar.original - length) > 0.5f / kFps)
        {
            // Its own length, when the bundle or its settings play it faster or slower.
            const float ox = left + (start + bar.original) * m_pixelsPerSecond;
            draw->AddLine(ImVec2(ox, a.y), ImVec2(ox, b.y), IM_COL32(230, 200, 80, 255), 1.5f);
        }
        draw->PushClipRect(a, b, true);
        draw->AddText(ImVec2(a.x + 4.0f, a.y), IM_COL32_WHITE, shortName(bar.name).c_str());
        draw->PopClipRect();
        const bool overBar = mouse.x >= a.x && mouse.x <= b.x && mouse.y >= a.y && mouse.y <= b.y;
        const bool overEdge = overBar && mouse.x >= b.x - 5.0f;
        if (overBar && canvasHovered)
        {
            ImGui::SetTooltip("%s\nstart %.3f s, length %.3f s%s%s", bar.name.c_str(), bar.start, length,
                              bar.original > 0.0f ? (", its own " + seconds(bar.original) + " s").c_str() : ", its .baf unread",
                              bar.lengthSet ? " (the bundle sets the length; it sets it for every bundle)" : "");
            if (overEdge)
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_dragKind == 0)
            {
                m_dragKind = overEdge ? 2 : 1;
                m_dragIndex = static_cast<int>(i);
                m_dragFrom = overEdge ? length : bar.start;
                m_dragMouse = mouse.x;
                m_dragValue = m_dragFrom;
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                ImGui::OpenPopup(("##barMenu" + std::to_string(i)).c_str());
        }
        if (ImGui::BeginPopup(("##barMenu" + std::to_string(i)).c_str()))
        {
            if (ImGui::MenuItem(("Take " + shortName(bar.name) + " out of the bundle").c_str()))
                run("EditorAnimation.removeAnimation " + quoted(bar.name));
            if (ImGui::MenuItem("Start it at the cursor"))
                run("EditorAnimation.setStart animation " + quoted(bar.name) + " " + seconds(snapFrame(m_cursor)));
            if (ImGui::MenuItem("Select its settings"))
                select("animation", bar.name);
            ImGui::EndPopup();
        }
    }
    if (!timeline.points.empty())
    {
        const float y = top + rowHeight * static_cast<float>(timeline.bars.size());
        for (size_t i = 0; i < timeline.points.size(); ++i)
        {
            float t = timeline.points[i].time;
            if (m_dragKind == 3 && m_dragIndex == static_cast<int>(i))
                t = m_dragValue;
            const float x = left + t * m_pixelsPerSecond;
            draw->AddTriangleFilled(ImVec2(x, y + 2.0f), ImVec2(x - 6.0f, y + rowHeight - 2.0f), ImVec2(x + 6.0f, y + rowHeight - 2.0f),
                                    IM_COL32(230, 140, 60, 255));
            draw->AddText(ImVec2(x + 7.0f, y), ImGui::GetColorU32(ImGuiCol_Text), timeline.points[i].name.c_str());
            const bool over = std::fabs(mouse.x - x) <= 6.0f && mouse.y >= y && mouse.y <= y + rowHeight;
            if (over && canvasHovered)
            {
                ImGui::SetTooltip("event %s at %.3f s", timeline.points[i].name.c_str(), timeline.points[i].time);
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_dragKind == 0)
                {
                    m_dragKind = 3;
                    m_dragIndex = static_cast<int>(i);
                    m_dragFrom = timeline.points[i].time;
                    m_dragMouse = mouse.x;
                    m_dragValue = m_dragFrom;
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    ImGui::OpenPopup(("##eventMenu" + std::to_string(i)).c_str());
            }
            if (ImGui::BeginPopup(("##eventMenu" + std::to_string(i)).c_str()))
            {
                if (ImGui::MenuItem(("Take the event " + timeline.points[i].name + " out").c_str()))
                    run("EditorAnimation.removeEvent " + quoted(timeline.points[i].name));
                if (ImGui::MenuItem("Move it to the cursor"))
                    run("EditorAnimation.setStart event " + quoted(timeline.points[i].name) + " " + seconds(snapFrame(m_cursor)));
                ImGui::EndPopup();
            }
        }
    }
    // The bundle's end, and the cursor (a click on the ruler moves it).
    const float endX = left + span * m_pixelsPerSecond;
    draw->AddLine(ImVec2(endX, origin.y), ImVec2(endX, origin.y + height), IM_COL32(200, 200, 200, 120), 1.0f);
    const float cursorX = left + m_cursor * m_pixelsPerSecond;
    draw->AddLine(ImVec2(cursorX, origin.y), ImVec2(cursorX, origin.y + height), IM_COL32(240, 80, 80, 255), 1.5f);
    if (canvasHovered && m_dragKind == 0 && mouse.y < top && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        m_playing = false;
        m_cursor = snapFrame((mouse.x - left) / m_pixelsPerSecond);
        if (m_cursor < 0.0f)
            m_cursor = 0.0f;
        if (m_cursor > span)
            m_cursor = span;
    }
    ImGui::EndChild();
}

} // namespace bf2editor
