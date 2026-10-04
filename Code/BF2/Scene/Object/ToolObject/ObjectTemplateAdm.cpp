// Object/template console administration. Source path attested by __FILE__.
// Unimplemented editor save/list/print/layer operations remain [FRONTIER] at their declarations.

#include "ObjectTemplateAdm.h"

#include "../../../Core/Ids.h"                          // io::IID_IConsoleSaveable
#include "../../../Game/IGame.h"                        // game / IGame::destroyObject
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../IO/Console/IConsoleSaveable.h"
#include "../../../IO/Stream/MemStream.h"
#include "../../../Math/Mat4.h"
#include "../ObjectManager.h"
#include "../ObjectTypes/Base/ObjectTemplateManager.h"

#include <map>
#include <sstream>

namespace dice {
namespace hfe {
namespace tool {

const char* const DefaultLayer = "main";             // 0x898B9C4 -> 0x88CB145 "main"

ObjectTemplateAdm objectTemplateAdm;                 // the TU's static init constructs it

// the two flag bytes, 128 empty layers, the first named DefaultLayer, +0x404 = 0.
ObjectTemplateAdm::ObjectTemplateAdm()
{
    m_flag0 = 0;
    m_flag1 = 0;
    for (int i = 0; i < 128; ++i)
        m_layers[i].hidden = false;
    m_unresolved404 = 0;
    std::string defaultLayer(DefaultLayer);
    Layer first;
    first.name = defaultLayer;
    first.hidden = false;
    m_layers[0].name.assign(first.name);
    m_layers[0].hidden = first.hidden;
}

// createObject - 0x832A4D0, 778 B, read at the instruction level (the transform choice and the
// setRotationZXY argument order are what matter). Object.create's target.
world::IObject* ObjectTemplateAdm::createObject(const world::IObjectTemplate* objectTemplate,
                                                const Vec3& position, const Vec3& rotation)
{
    std::string unused;                              // the ELF constructs and destroys one
    world::IObject* object = 0;
    if (objectTemplate)
    {
        world::objectTemplateManager->setActiveObjectTemplate(objectTemplate);           // +0x54
        if (world::objectTemplateManager->getActiveObjectTemplate())                     // +0x50
        {
            object = world::objectTemplateManager->getActiveObjectTemplate()->createObject();   // template +0x54
            if (object)
            {
                object->setAbsolutePosition(position);                                    // +0x70
                Mat4 m = object->getParent() ? object->getRelativeTransformation()        // +0xE8
                                             : object->getAbsoluteTransformation();       // +0x84
                // (Mat4&, zDegrees, xDegrees, yDegrees) <- (-rot.z, -rot.y, -rot.x): the .con
                // triple is yaw/pitch/roll = about Y / about X / about Z, each negated.
                setRotationZXY(m, -rotation.z, -rotation.y, -rotation.x);
                if (object->getParent())
                    object->setRelativeTransformation(m);                                 // +0xEC
                else
                    object->setAbsoluteTransformation(m);                                 // +0x8C
                object->updateFlags(0x20000u, 0u);                                        // +0x60
                object->init();                                                           // +0x20
            }
            activateObject(object, false);
            return object;
        }
    }
    activateObject(0, false);
    return 0;
}


world::IObject* ObjectTemplateAdm::createObject(const world::IObjectTemplate* objectTemplate,
                                                const Vec3& position)
{
    Vec3 rotation; rotation.x = rotation.y = rotation.z = 0.0f;
    return createObject(objectTemplate, position, rotation);
}


world::IObject* ObjectTemplateAdm::createObject(const world::IObjectTemplate* objectTemplate)
{
    Vec3 position; position.x = position.y = position.z = 0.0f;
    Vec3 rotation; rotation.x = rotation.y = rotation.z = 0.0f;
    return createObject(objectTemplate, position, rotation);
}


bool ObjectTemplateAdm::activateObject(world::IObject* object, bool toggle)
{
    if (object && !object->getTemplate())
        return false;
    if (toggle && world::objectManager->getActiveObject()
        && world::objectManager->getActiveObject() == object)
    {
        world::objectManager->setActiveObject(0);
    }
    else
    {
        world::objectManager->setActiveObject(object);
        if (object)
            world::objectTemplateManager->setActiveObjectTemplate(object->getTemplate());
    }
    return true;
}


bool ObjectTemplateAdm::setActiveObject(world::IObject* object)
{
    return activateObject(object, false);
}

// Preview commands forward their message to a nonnull object with zero data and false
// propagation. Used by start, stop, forceStart, and forceStop console commands.

void ObjectTemplateAdm::start(world::IObject* object)
{
    if (object)
        object->handleMessage(static_cast<world::TemplateMessage>(0), 0, false);
}


void ObjectTemplateAdm::stop(world::IObject* object)
{
    if (object)
        object->handleMessage(static_cast<world::TemplateMessage>(1), 0, false);
}


void ObjectTemplateAdm::forceStart(world::IObject* object)
{
    if (object)
        object->handleMessage(static_cast<world::TemplateMessage>(0x40), 0, false);
}


void ObjectTemplateAdm::forceStop(world::IObject* object)
{
    if (object)
        object->handleMessage(static_cast<world::TemplateMessage>(0x41), 0, false);
}

// Selects the active template after the required class lookups, even when a lookup misses.
world::IObjectTemplate* ObjectTemplateAdm::setActiveObjectTemplate(const world::IObjectTemplate* objectTemplate)
{
    world::objectTemplateManager->setActiveObjectTemplate(objectTemplate);       // +0x54
    return world::objectTemplateManager->getActiveObjectTemplate();              // +0x50
}

void ObjectTemplateAdm::setActiveObjectTemplate(const std::string& className,
                                                const world::IObjectTemplate* objectTemplate)
{
    world::objectTemplateManager->getClassIDFromTemplateClassName(className);    // +0x20 - result dropped
    if (objectTemplate)
    {
        std::string resolvedName =                                               // +0x1C - built, dropped
            world::objectTemplateManager->getClassNameFromTemplateClassID(objectTemplate->getClassID());
        (void)resolvedName;
    }
    world::objectTemplateManager->setActiveObjectTemplate(objectTemplate);       // +0x54
}


world::IObject* ObjectTemplateAdm::getObjectFromName(const std::string& name)
{
    if (!name.empty())
        return world::objectManager->getObjectFromNameHash(name);
    return world::objectManager->getActiveObject();
}

// "" -> the active template, else the name lookup.
const world::IObjectTemplate* ObjectTemplateAdm::getTemplateFromName(const std::string& name)
{
    if (name.empty())
        return world::objectTemplateManager->getActiveObjectTemplate();
    return world::objectTemplateManager->getTemplate(name);
}

namespace {

// tool::makeConsoleScript (ELF 0x6a6dc0): resolve the object's IConsoleSaveable face and let it
// write the script into the stream. Returns whether a writer ran.
bool makeConsoleScript(const IBase* object, io::IStream* stream)
{
    if (!object || !stream)
        return false;
    io::IConsoleSaveable* saveable = static_cast<io::IConsoleSaveable*>(
        object->queryInterface(io::IID_IConsoleSaveable));
    if (!saveable)
        return false;
    return saveable->makeScript(stream);
}

// tool::getAsString (ELF 0x6a82d0): return the bytes the writer left in the stream.
// [CHOSEN, not recovered] This body is deliberately NOT the shipped one. The shipped body sizes the
// result with the strlen-based std::string(const char*) constructor, then writes a NUL at
// _M_p + getSize() (the valgrind "Invalid write of size 1" overrun): whenever getSize() exceeds
// strlen(getPtr()) that write runs past the string's allocation. This version copies getSize() bytes
// instead, which is byte-for-byte identical for the script text these callers produce (no embedded
// NUL, so size == strlen) and avoids the out-of-bounds write. Restore the shipped construction only
// with evidence that the overrun is wanted.
std::string getAsString(io::IMemStream* stream)
{
    const unsigned int size = static_cast<unsigned int>(stream->getSize());
    if (!size)
        return std::string();
    return std::string(static_cast<const char*>(stream->getPtr()), size);
}

} // namespace

// ObjectTemplateAdm::getTemplateType (ELF 0x6a8060): the registered class name, or the shipped
// miss text when no template is selected.
std::string ObjectTemplateAdm::getTemplateType(const world::IObjectTemplate* objectTemplate)
{
    std::ostringstream out;
    if (objectTemplate)
        out << world::objectTemplateManager->getClassNameFromTemplateClassID(objectTemplate->getClassID());
    else
        out << "No object template";
    return out.str();
}

// ObjectTemplateAdm::getObjectTemplateInfo (ELF 0x6a7db0): `"<name>" of primitive <class>`.
std::string ObjectTemplateAdm::getObjectTemplateInfo(const world::IObjectTemplate* objectTemplate)
{
    std::ostringstream out;
    if (objectTemplate)
        out << "\"" << objectTemplate->getName() << "\" of primitive "
            << world::objectTemplateManager->getClassNameFromTemplateClassID(objectTemplate->getClassID());
    else
        out << "No object template";
    return out.str();
}

// ObjectTemplateAdm::getObjectInfo (ELF 0x6a9390): one object row. The template part is the full
// getObjectTemplateInfo text, not the bare template name. Unnamed objects print Untitled.
std::string ObjectTemplateAdm::getObjectInfo(const world::IObject* object)
{
    std::ostringstream out;
    if (object)
    {
        const std::string& name = object->getName();
        if (!name.empty())
            out << "\"" << name << "\"";
        else
            out << "\"Untitled\"";
        out << " with ID " << object->getObjectId()
            << " of template " << getObjectTemplateInfo(object->getTemplate());
    }
    else
        out << "No object";
    return out.str();
}

// ObjectTemplateAdm::listObjectsOfTemplate (ELF 0x6ab520): every grid object whose template pointer
// matches, one getObjectInfo row plus a newline each.
std::string ObjectTemplateAdm::listObjectsOfTemplate(const world::IObjectTemplate* objectTemplate)
{
    std::string result;
    const std::map<unsigned int, world::IObject*>& objects =
        world::objectManager->getAllObjectsFromGrid();
    for (std::map<unsigned int, world::IObject*>::const_iterator it = objects.begin();
         it != objects.end(); ++it)
    {
        world::IObject* object = it->second;
        if (object->getTemplate() == objectTemplate)
            result += getObjectInfo(object) + "\n";
    }
    return result;
}

// ObjectTemplateAdm::initMemStream (ELF 0x6a7120): create the admin's owned memory stream once
// through the CID_MemStream factory, or clear the existing one so the next writer starts at zero.
void ObjectTemplateAdm::initMemStream()
{
    if (m_memStream)
        m_memStream->clear();
    else
        m_memStream.create(io::CID_MemStream);
}

// ObjectTemplateAdm::printObjectCreationScript (ELF 0x6a83b0): write the object's script into the
// admin's stream, then read it back as text.
std::string ObjectTemplateAdm::printObjectCreationScript(world::IObject* object)
{
    initMemStream();
    makeConsoleScript(object, m_memStream);
    return getAsString(m_memStream);
}

// ObjectTemplateAdm::printTemplateCreationScript (ELF 0x6a8410): the template variant.
std::string ObjectTemplateAdm::printTemplateCreationScript(const world::IObjectTemplate* objectTemplate)
{
    initMemStream();
    makeConsoleScript(objectTemplate, m_memStream);
    return getAsString(m_memStream);
}

// ObjectTemplateAdm::deleteObject (ELF 0x6a8980): the shipped body reads the active-object cursor
// (and drops it), routes the removal through the game singleton, then clears the cursor when it
// named the object.
void ObjectTemplateAdm::deleteObject(world::IObject* object)
{
    if (!object)
        return;
    world::objectManager->getActiveObject();
    if (game)
        game->destroyObject(object);                                        // IGame +0xA0
    if (object == world::objectManager->getActiveObject())
        world::objectManager->setActiveObject(0);                           // +0x94
}

// ObjectTemplateAdm::deleteTemplate (ELF 0x6a7570): destroy every object of the template, remove
// the template, then clear the active-template cursor when it named the removed template.
void ObjectTemplateAdm::deleteTemplate(const world::IObjectTemplate* objectTemplate)
{
    if (!objectTemplate)
        return;
    const std::map<unsigned int, world::IObject*>& objects =
        world::objectManager->getAllObjectsFromGrid();
    for (std::map<unsigned int, world::IObject*>::const_iterator it = objects.begin();
         it != objects.end(); ++it)
    {
        if (it->second->getTemplate() == objectTemplate && game)
            game->destroyObject(it->second);                                // IGame +0xA0
    }
    world::objectTemplateManager->deleteTemplate(                            // +0x44
        const_cast<world::IObjectTemplate*>(objectTemplate));
    if (objectTemplate == world::objectTemplateManager->getActiveObjectTemplate())
        world::objectTemplateManager->setActiveObjectTemplate(0);            // +0x54
}

} // namespace tool
} // namespace hfe
} // namespace dice
