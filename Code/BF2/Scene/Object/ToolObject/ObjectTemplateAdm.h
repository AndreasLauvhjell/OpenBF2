// Administrator shared by editor commands and dedicated-server level scripts.
// The file-scope instance creates static objects and spawners through Object.create.
// The layer array starts with visible main; the two flag bytes and trailing field are [UNRESOLVED].
// Header name [INFERRED]; source path is attested. Unimplemented operations remain [FRONTIER].

#ifndef SCENE_OBJECT_TOOLOBJECT_OBJECTTEMPLATEADM_H     // [INFERRED] guard style
#define SCENE_OBJECT_TOOLOBJECT_OBJECTTEMPLATEADM_H

#include <string>

#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

namespace world { class IObject; class IObjectTemplate; }

namespace tool {

extern const char* const DefaultLayer;               // _ZN4dice3hfe4tool12DefaultLayerE -> "main"

class ObjectTemplateAdm
{
public:
    struct Layer
    {
        std::string name;
        bool        hidden;
    };

    ObjectTemplateAdm();

    // Creation - the `Object.create` family (all three read end to end)
    // if template && (setActiveObjectTemplate(template), getActiveObjectTemplate())
    //   obj = getActiveObjectTemplate()->createObject(); if obj: setAbsolutePosition(pos);
    //   m = parent ? relative : absolute transformation; setRotationZXY(m, -rot.z, -rot.y, -rot.x)
    //   (yaw/pitch/roll, negated - BF2's .con rotation convention); write it back the same way;
    //   updateFlags(0x20000, 0); init(). Then activateObject(obj, false) and return obj - or, for
    //   a null / unregistered template, activateObject(0, false) and 0.
    world::IObject* createObject(const world::IObjectTemplate* objectTemplate, const Vec3& position,
                                 const Vec3& rotation);
    world::IObject* createObject(const world::IObjectTemplate* objectTemplate, const Vec3& position);   // rotation 0
    world::IObject* createObject(const world::IObjectTemplate* objectTemplate);                         // both 0

    // an object without a template is refused (false); `toggle` && it is already
    // the active object -> deactivate (setActiveObject(0)); else make it the active object and make
    // its template the active template. Returns true.
    bool activateObject(world::IObject* object, bool toggle);
    bool setActiveObject(world::IObject* object);    // activateObject(object, false)

    // Selects and returns the active template. The class-name overload performs class
    // lookups but does not compare their results before selecting it.
    world::IObjectTemplate* setActiveObjectTemplate(const world::IObjectTemplate* objectTemplate);
    void setActiveObjectTemplate(const std::string& className, const world::IObjectTemplate* objectTemplate);

    // "" -> the active object, else the name hash.
    world::IObject* getObjectFromName(const std::string& name);
    // "" -> the active template, else the name lookup.
    const world::IObjectTemplate* getTemplateFromName(const std::string& name);

    // Sends start/stop/forced start/forced stop messages to a non-null object.
    // Ordinary messages respect effect masks; forced messages bypass them.
    // TemplateMessage enumerator names remain [UNRESOLVED].
    void start(world::IObject* object);
    void stop(world::IObject* object);
    void forceStart(world::IObject* object);
    void forceStop(world::IObject* object);

    // [FRONTIER] Remaining editor object/template persistence, listing, and layer operations
    // require their corresponding implementations.

private:
    unsigned char m_flag0;                           // +0x000
    unsigned char m_flag1;                           // +0x001
    Layer         m_layers[128];                     // +0x004
    int           m_unresolved404;                   // +0x404
};

// The file-scope instance ModuleToolObject.cpp's console executors address directly (0x8AB0B40 -
// no symbol survives; the name is [CHOSEN]).
extern ObjectTemplateAdm objectTemplateAdm;

} // namespace tool
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_TOOLOBJECT_OBJECTTEMPLATEADM_H
