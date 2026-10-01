// Previews selected effects or an unsaved temporary object at the view's ground hit.
// Clear, replacement and level unload destroy temporary previews. Console calls restore the active object.
// RW-272: start/stop use object messages; pause requires exactly an EffectBundle template.
// Quality controls and a separate preview background are unsupported.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "Framework/EditorLog.h"

#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "IO/Console/Console.h"
#include "Scene/Object/ObjectManager.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include <cstdio>
#include <cstring>

namespace bf2editor
{

namespace
{

using dice::hfe::world::IObject;
using dice::hfe::world::IObjectTemplate;

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

// The template's class as ObjectTemplate.create names it ("EffectBundle", "WheelEffectBundle" ...).
std::string classOf(const IObjectTemplate* objectTemplate)
{
    return objectTemplate && dice::hfe::world::objectTemplateManager
        ? dice::hfe::world::objectTemplateManager->getClassNameFromTemplateClassID(objectTemplate->getClassID())
        : std::string();
}

bool effectClass(const std::string& className)
{
    const char tail[] = "EffectBundle";
    const size_t n = sizeof(tail) - 1;
    return className.size() >= n && _stricmp(className.c_str() + className.size() - n, tail) == 0;
}

} // namespace

bool EditSession::isEffectTemplate(const std::string& name)
{
    const IObjectTemplate* objectTemplate =
        dice::hfe::world::objectTemplateManager ? dice::hfe::world::objectTemplateManager->getTemplate(name) : 0;
    return objectTemplate && effectClass(classOf(objectTemplate));
}

bool EditSession::previewEffect(const std::string& templateName, std::string& result)
{
    if (!dice::hfe::io::mainConsole || !dice::hfe::world::objectManager || !dice::hfe::world::objectTemplateManager)
    {
        result = "the engine console is not up";
        return false;
    }
    IObjectTemplate* objectTemplate = dice::hfe::world::objectTemplateManager->getTemplate(templateName);
    if (!objectTemplate)
    {
        result = "no template named " + templateName;
        return false;
    }
    const std::string className = classOf(objectTemplate);
    if (!effectClass(className))
    {
        result = objectTemplate->getName() + " is a " + className + ", not an effect bundle";
        return false;
    }
    std::string ignored;
    clearEffectPreview(ignored);
    float point[3];
    pointAtViewCentre(point);
    if (point[1] == 0.0f)
        point[1] = 0.01f;           // QuadTreeCuller::createObject refuses an object at exactly y = 0
    IObject* previous = dice::hfe::world::objectManager->getActiveObject();
    const std::string answer = engineLine("Object.create " + objectTemplate->getName());
    IObject* made = dice::hfe::world::objectManager->getActiveObject();
    if (!made || made == previous || made->getTemplate() != objectTemplate)
    {
        dice::hfe::world::objectManager->setActiveObject(previous);
        result = "the engine made no " + objectTemplate->getName() + " (Object.create answered '" + answer + "')";
        return false;
    }
    char line[128];
    std::snprintf(line, sizeof(line), "Object.absolutePosition %.3f/%.3f/%.3f", point[0], point[1], point[2]);
    const std::string placed = engineLine(line);
    const std::string layered = placed.empty() ? engineLine("Object.layer 1") : std::string();
    m_effectPreview = made->getObjectId();
    dice::hfe::world::objectManager->setActiveObject(previous);
    if (!placed.empty() || !layered.empty())
    {
        std::string removed;
        clearEffectPreview(removed);
        result = "the engine refused the preview's place: " + (placed.empty() ? layered : placed);
        return false;
    }
    std::snprintf(line, sizeof(line), " at %.1f/%.1f/%.1f (object #%u, not part of the level)", point[0], point[1],
                  point[2], m_effectPreview);
    result = "previewing " + objectTemplate->getName() + line;
    logLine(LogLevel::Info, "effect: %s", result.c_str());
    return true;
}

bool EditSession::effectAction(const std::string& action, std::string& result)
{
    const char* row = action == "play" ? "Object.forceStart"
                    : action == "stop" ? "Object.forceStop"
                    : action == "pause" ? "Object.togglePause" : 0;
    if (!row)
    {
        result = "'" + action + "' is not play, stop or pause";
        return false;
    }
    if (!dice::hfe::io::mainConsole || !dice::hfe::world::objectManager)
    {
        result = "the engine console is not up";
        return false;
    }
    // The selected effect objects, else the preview.
    std::vector<unsigned> ids;
    const std::vector<unsigned>& selected = m_selection.ids();
    for (size_t i = 0; i < selected.size(); ++i)
    {
        IObject* o = object(selected[i]);
        if (o && effectClass(classOf(o->getTemplate())))
            ids.push_back(selected[i]);
    }
    if (ids.empty() && m_effectPreview != 0 && object(m_effectPreview))
        ids.push_back(m_effectPreview);
    if (ids.empty())
    {
        result = "nothing to " + action + ": select an effect object, or make a preview (EditorEffect.preview <template>)";
        return false;
    }
    IObject* previous = dice::hfe::world::objectManager->getActiveObject();
    int done = 0;
    std::string refused;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        IObject* o = object(ids[i]);
        const std::string className = classOf(o->getTemplate());
        char label[64];
        std::snprintf(label, sizeof(label), " #%u", ids[i]);
        if (action == "pause" && _stricmp(className.c_str(), "EffectBundle") != 0)
        {
            refused += (refused.empty() ? "" : "; ") + o->getTemplate()->getName() + label + " is a " + className
                     + " (pause takes an EffectBundle only)";
            continue;
        }
        dice::hfe::world::objectManager->setActiveObject(o);
        const std::string answer = engineLine(row);
        if (!answer.empty())
            refused += (refused.empty() ? "" : "; ") + o->getTemplate()->getName() + label + ": " + answer;
        else
            ++done;
    }
    dice::hfe::world::objectManager->setActiveObject(previous);
    char count[96];
    std::snprintf(count, sizeof(count), "%s: %d effect%s (%s)", action.c_str(), done, done == 1 ? "" : "s", row);
    result = count;
    if (!refused.empty())
        result += "; refused: " + refused;
    return done > 0;
}

bool EditSession::clearEffectPreview(std::string& result)
{
    if (m_effectPreview == 0)
    {
        result = "there is no preview";
        return true;
    }
    const unsigned id = m_effectPreview;
    m_effectPreview = 0;
    m_selection.remove(id);
    IObject* o = object(id);
    if (o && dice::hfe::world::objectManager)
    {
        // The engine's own removal, as removeObject does it: forceDestroy, then the destroy list drained.
        o->forceDestroy();
        dice::hfe::world::objectManager->destroyObjects();
    }
    result = "the preview is gone";
    return true;
}

bool EditSession::effectInfo(std::string& result)
{
    IObject* preview = m_effectPreview != 0 ? object(m_effectPreview) : 0;
    char text[64];
    if (preview && preview->getTemplate())
    {
        std::snprintf(text, sizeof(text), " (object #%u)", m_effectPreview);
        result = "preview: " + preview->getTemplate()->getName() + text;
    }
    else
        result = "no preview";
    int effects = 0;
    const std::vector<unsigned>& selected = m_selection.ids();
    for (size_t i = 0; i < selected.size(); ++i)
    {
        IObject* o = object(selected[i]);
        if (o && effectClass(classOf(o->getTemplate())))
            ++effects;
    }
    std::snprintf(text, sizeof(text), "; %d selected effect object%s", effects, effects == 1 ? "" : "s");
    result += text;
    return true;
}

} // namespace bf2editor
