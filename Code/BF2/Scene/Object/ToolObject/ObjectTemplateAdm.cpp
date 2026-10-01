// Object/template console administration. Source path attested by __FILE__.
// Unimplemented editor save/list/print/layer operations remain [FRONTIER] at their declarations.

#include "ObjectTemplateAdm.h"

#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Math/Mat4.h"
#include "../ObjectManager.h"
#include "../ObjectTypes/Base/ObjectTemplateManager.h"

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

} // namespace tool
} // namespace hfe
} // namespace dice
