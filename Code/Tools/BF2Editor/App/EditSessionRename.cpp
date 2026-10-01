// Renames a level-defined template and recreates its instances as one undoable edit.
// Reusing a template skips additive addTemplate commands to avoid duplicate children.
// Creates all replacement objects before deleting originals, preserving properties and history ID resolution.
// Updates the file in place; player-visible control-point names are unchanged.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"

#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "Scene/Object/ObjectManager.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "IO/Console/Console.h"

#include <cctype>
#include <cstdio>
#include <cstring>

using dice::hfe::Mat4;
using dice::hfe::world::IObject;
using dice::hfe::world::IObjectTemplate;

namespace bf2editor
{

namespace
{

std::string lowerName(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

// One engine console line with full access; its answer, trimmed (EditSession.cpp's engineLine).
std::string engineLine(const std::string& line)
{
    std::string answer;
    dice::hfe::io::mainConsole->executeLine(line + "\n", answer, false, false, false, false, true);
    const size_t begin = answer.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return answer.substr(begin, answer.find_last_not_of(" \t\r\n") - begin + 1);
}

bool templateNameOk(const std::string& name)
{
    return !name.empty() && name.find_first_of(" \t/\\\"") == std::string::npos;
}

} // namespace

bool EditSession::renameLevelTemplate(const std::string& from, const std::string& to, std::string& result)
{
    size_t file = 0;
    std::string cls;
    if (!m_document || !m_document->isOpen() || !m_document->templateFile(from, file, cls))
    {
        result = "no level file this editor writes defines a template '" + from
               + "' (the Objects tree's templates are the object editor's, WP7)";
        return false;
    }
    if (m_document->objects(file).isTemplateDeleted(from))
    {
        result = "'" + from + "' went with its last object";
        return false;
    }
    if (!templateNameOk(to))
    {
        result = "'" + to + "' is not a template name (no blanks, slashes or quotes)";
        return false;
    }
    if (_stricmp(to.c_str(), from.c_str()) == 0)
    {
        result = to == from ? "'" + from + "' has that name already"
                            : "the engine compares template names without case: a rename must change more than the case";
        return false;
    }
    size_t otherFile = 0;
    std::string otherClass;
    if (m_document->templateFile(to, otherFile, otherClass) || m_removedTemplates.count(lowerName(to)))
    {
        result = "a template named '" + to + "' is in the level files already";
        return false;
    }
    // The engine keeps every template it loaded; only one a rename of this session left behind, of the
    // same class, can take the name back.
    IObjectTemplate* existing =
        dice::hfe::world::objectTemplateManager ? dice::hfe::world::objectTemplateManager->getTemplate(to) : 0;
    if (existing && (!m_renamedAway.count(lowerName(to)) || _stricmp(className(existing->getClassID()).c_str(), cls.c_str()) != 0))
    {
        result = "the engine has a template named '" + to + "' already (" + className(existing->getClassID())
               + "); choose another name";
        return false;
    }
    HistoryEntry entry;
    entry.label = "Rename " + from + " to " + to;
    entry.doScript = "EditorGameplay.applyRename " + from + " " + to + "\n";
    entry.undoScript = "EditorGameplay.applyRename " + to + " " + from + "\n";
    if (!m_history.execute(entry, result))
        return false;
    logLine(LogLevel::Info, "edit: %s", entry.label.c_str());
    result = entry.label;
    return true;
}

bool EditSession::applyTemplateRename(const std::string& from, const std::string& to, std::string& error)
{
    size_t file = 0;
    std::string cls;
    if (!m_document || !m_document->templateFile(from, file, cls))
    {
        error = "no level file this editor writes defines a template '" + from + "'";
        return false;
    }
    if (!templateNameOk(to) || !dice::hfe::io::mainConsole || !dice::hfe::world::objectTemplateManager
        || !dice::hfe::world::objectManager)
    {
        error = "cannot rename '" + from + "' to '" + to + "'";
        return false;
    }

    // 1. The engine template of the new name, with the lines the file gives the old one.
    const bool existed = dice::hfe::world::objectTemplateManager->getTemplate(to) != 0;
    engineLine((existed ? "ObjectTemplate.activeSafe " : "ObjectTemplate.create ") + cls + " " + to);
    IObjectTemplate* made = dice::hfe::world::objectTemplateManager->getTemplate(to);
    if (!made || _stricmp(className(made->getClassID()).c_str(), cls.c_str()) != 0)
    {
        error = "the engine made no " + cls + " template '" + to + "'";
        return false;
    }
    const std::vector<LevelObjectFile::TemplateLine> lines = m_document->templateLines(file, from);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (existed && lines[i].key == "addtemplate")
            continue;
        // The retail editor's author stamp: the 1.50 engine has no such property (the level load's own
        // line answers "Unknown object or method!"); it stays in the file.
        if (lines[i].key == "modifiedbyuser")
            continue;
        const std::string line = "ObjectTemplate." + lines[i].property + " " + lines[i].value;
        const std::string answer = engineLine(line);
        if (!answer.empty())
            logLine(LogLevel::Warning, "rename: %s answered: %s", line.c_str(), answer.c_str());
    }

    // 2. Its objects, made again from it where they stand; every new one before any old one goes.
    struct Moved
    {
        LevelDocument::RecordRef record;
        unsigned                 before;
        unsigned                 after;
    };
    std::vector<Moved> moved;
    const LevelObjectFile& objects = m_document->objects(file);
    for (size_t r = 0; r < objects.recordCount(); ++r)
    {
        if (objects.isDeleted(r) || _stricmp(objects.placement(r).templateName.c_str(), from.c_str()) != 0)
            continue;
        Moved m;
        m.record.file = file;
        m.record.record = r;
        m.before = m_document->idOf(m.record);
        m.after = 0;
        if (m.before && object(m.before))
            moved.push_back(m);
    }
    for (size_t i = 0; i < moved.size(); ++i)
    {
        IObject* old = object(moved[i].before);
        const Mat4 transform = old->getAbsoluteTransformation();
        const int layer = static_cast<int>(old->getLayer());
        const ObjectProperties properties = propertiesOf(old);
        std::string why;
        if (!makeObject(to, transform, layer, moved[i].after, why)
            || !runConsoleScript(propertiesScript(moved[i].after, properties), why))
        {
            // Nothing is renamed: the objects made so far go again.
            for (size_t k = 0; k <= i; ++k)
            {
                IObject* extra = moved[k].after ? object(moved[k].after) : 0;
                if (extra)
                    extra->forceDestroy();
            }
            dice::hfe::world::objectManager->destroyObjects();
            error = "making " + describe(moved[i].before) + " again from '" + to + "': " + why;
            return false;
        }
    }
    for (size_t i = 0; i < moved.size(); ++i)
    {
        IObject* old = object(moved[i].before);
        if (old)
            old->forceDestroy();
    }
    dice::hfe::world::objectManager->destroyObjects();
    for (size_t i = 0; i < moved.size(); ++i)
    {
        // Scripts may name the object by any id it had.
        m_remap[moved[i].before] = moved[i].after;
        for (std::map<unsigned, unsigned>::iterator it = m_remap.begin(); it != m_remap.end(); ++it)
            if (it->second == moved[i].before)
                it->second = moved[i].after;
        m_document->rebind(moved[i].record, moved[i].after);
        if (m_selection.contains(moved[i].before))
        {
            m_selection.remove(moved[i].before);
            m_selection.add(moved[i].after);
        }
    }

    // 3. The file.
    if (!m_document->renameTemplate(file, from, to))
    {
        error = "renaming '" + from + "' in " + m_document->fileName(file) + " failed";
        return false;
    }
    m_renamedAway.insert(lowerName(from));
    m_renamedAway.erase(lowerName(to));
    if (_stricmp(m_activeSound.c_str(), from.c_str()) == 0)
        m_activeSound = to;
    m_summariesStale = true;
    m_countsStale = true;
    return true;
}

} // namespace bf2editor
