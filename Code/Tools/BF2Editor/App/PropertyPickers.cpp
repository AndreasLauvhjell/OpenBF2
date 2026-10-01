// Code/Tools/BF2Editor/App/PropertyPickers.cpp - see PropertyPickers.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/PropertyPickers.h"

#include "App/EditSession.h"
#include "App/MaterialDocuments.h"
#include "Framework/EditorLog.h"

#include "Game/Objects/IObjectTemplate.h"
#include "IO/File/FileManager.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <list>
#include <map>
#include <sstream>
#include <vector>

namespace bf2editor
{

namespace PropertyPickers
{

namespace
{

enum class Kind
{
    File,           // `filter`: the extensions, blank-separated
    Template,       // `filter`: the class names, blank-separated
    Material
};

struct Spec
{
    const char* property;
    Kind        kind;
    const char* filter;
};

// The reference properties of the census's list that a list can serve; their stock values
// are mod-rooted Objects paths (animation systems, sounds), template names and material ids.
const Spec kSpecs[] = {
    { "animationsystem1p", Kind::File, ".inc" },
    { "animationsystem3p", Kind::File, ".inc" },
    { "seatanimationsystem", Kind::File, ".inc" },
    { "soundfilename", Kind::File, ".wav .ogg" },
    { "projectiletemplate", Kind::Template, "GenericProjectile" },
    { "detonation.endeffecttemplate", Kind::Template, "EffectBundle Bundle" },
    { "material", Kind::Material, "" },
    { "armor.explosionmaterial", Kind::Material, "" },
};

struct Choice
{
    std::string label;
    std::string value;
};

// The open popup's list, and the files found under Objects by extension filter (read once a session).
std::vector<Choice> g_choices;
char g_filter[96] = "";
std::map<std::string, std::vector<std::string> > g_files;

std::string lowered(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    return text;
}

std::string bareProperty(const std::string& property)
{
    std::string bare = lowered(property);
    if (bare.size() > 3 && (bare.compare(0, 3, "set") == 0 || bare.compare(0, 3, "get") == 0))
        bare = bare.substr(3);
    return bare;
}

std::vector<std::string> words(const char* text)
{
    std::vector<std::string> out;
    std::istringstream in(text ? text : "");
    std::string word;
    while (in >> word)
        out.push_back(word);
    return out;
}

const Spec* specFor(const std::string& property)
{
    const std::string bare = bareProperty(property);
    for (size_t i = 0; i < sizeof(kSpecs) / sizeof(kSpecs[0]); ++i)
        if (bare == kSpecs[i].property)
            return &kSpecs[i];
    return 0;
}

// Every file under Objects with one of the extensions, as the engine's file manager finds them (archives, or the loose
// folder when none is mounted there); paths relative to Objects, '/'.
const std::vector<std::string>& objectFiles(const char* extensions)
{
    std::map<std::string, std::vector<std::string> >::iterator cached = g_files.find(extensions);
    if (cached != g_files.end())
        return cached->second;
    std::vector<std::string>& files = g_files[extensions];
    if (!dice::hfe::io::fileManager)
        return files;
    const std::vector<std::string> wanted = words(extensions);
    std::list<dice::hfe::io::FileInfo> found;
    dice::hfe::io::fileManager->findFiles("objects/", true, found);
    for (std::list<dice::hfe::io::FileInfo>::const_iterator it = found.begin(); it != found.end(); ++it)
    {
        if (it->isDirectory)
            continue;
        std::string name = it->name;
        std::replace(name.begin(), name.end(), '\\', '/');
        const std::string low = lowered(name);
        for (size_t e = 0; e < wanted.size(); ++e)
            if (low.size() > wanted[e].size() && low.compare(low.size() - wanted[e].size(), wanted[e].size(), wanted[e]) == 0)
            {
                files.push_back(name);
                break;
            }
    }
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) { return lowered(a) < lowered(b); });
    logLine(LogLevel::Info, "pickers: %u %s files under Objects", static_cast<unsigned>(files.size()), extensions);
    return files;
}

void fill(const Spec& spec, EditSession& session)
{
    g_choices.clear();
    if (spec.kind == Kind::File)
    {
        const std::vector<std::string>& files = objectFiles(spec.filter);
        for (size_t i = 0; i < files.size(); ++i)
        {
            Choice choice;
            choice.value = "Objects/" + files[i];
            choice.label = choice.value;
            g_choices.push_back(choice);
        }
    }
    else if (spec.kind == Kind::Template)
    {
        using namespace dice::hfe::world;
        std::list<IObjectTemplate*>* all = objectTemplateManager ? objectTemplateManager->getGlobalTemplateList() : 0;
        const std::vector<std::string> classes = words(spec.filter);
        if (!all)
            return;
        for (std::list<IObjectTemplate*>::const_iterator it = all->begin(); it != all->end(); ++it)
        {
            if (!*it)
                continue;
            const std::string& className = objectTemplateManager->getClassNameFromTemplateClassID((*it)->getClassID());
            if (std::find(classes.begin(), classes.end(), className) == classes.end())
                continue;
            Choice choice;
            choice.value = (*it)->getName();
            choice.label = choice.value + "  (" + className + ")";
            g_choices.push_back(choice);
        }
        std::sort(g_choices.begin(), g_choices.end(),
                  [](const Choice& a, const Choice& b) { return lowered(a.value) < lowered(b.value); });
    }
    else
    {
        MaterialDocuments& materials = session.materials();
        std::string error;
        if (!materials.loaded() && !materials.load(error))
            logLine(LogLevel::Warning, "pickers: the material database could not be read: %s", error.c_str());
        const std::vector<int> ids = materials.loaded() ? materials.defines().ids() : std::vector<int>();
        for (size_t i = 0; i < ids.size(); ++i)
        {
            std::string name;
            materials.defines().value(ids[i], "name", name);
            Choice choice;
            choice.value = std::to_string(ids[i]);
            choice.label = choice.value + "  " + name;
            g_choices.push_back(choice);
        }
    }
}

} // namespace

bool serves(const std::string& property)
{
    return specFor(property) != 0;
}

bool button(const char* id, const std::string& property, const std::string& value, EditSession& session, std::string& out)
{
    const Spec* spec = specFor(property);
    if (!spec)
        return false;
    bool picked = false;
    ImGui::PushID(id);
    if (ImGui::SmallButton("..."))
    {
        fill(*spec, session);
        g_filter[0] = 0;
        ImGui::OpenPopup("picker");
    }
    ImGui::SetItemTooltip(spec->kind == Kind::File ? "Pick a %s file of the mod's Objects"
                          : spec->kind == Kind::Template ? "Pick a template (%s)" : "Pick a material%s",
                          spec->filter);
    // A popup sizes itself to what it holds, so the list has a size of its own.
    if (ImGui::BeginPopup("picker"))
    {
        ImGui::Text("%s (%u)", property.c_str(), static_cast<unsigned>(g_choices.size()));
        ImGui::SetNextItemWidth(460.0f);
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##filter", "filter", g_filter, sizeof(g_filter));
        const std::string filter = lowered(g_filter);
        ImGui::BeginChild("choices", ImVec2(460.0f, 320.0f), ImGuiChildFlags_None);
        for (size_t i = 0; i < g_choices.size(); ++i)
        {
            if (!filter.empty() && lowered(g_choices[i].label).find(filter) == std::string::npos)
                continue;
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(g_choices[i].label.c_str(), _stricmp(g_choices[i].value.c_str(), value.c_str()) == 0))
            {
                out = g_choices[i].value;
                picked = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return picked;
}

} // namespace PropertyPickers

} // namespace bf2editor
