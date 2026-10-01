// Path:     [CHOSEN] - see the header. The TU is real (own static init 0x831FDA0), its name is not.
// Contents: recovered from bf2.i64 (ELF, symbolised).

#include "ObjectTemplateUtil.h"
#include "ObjectManager.h"                       // the IObject_ptrproxy lookups
#include "ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../Core/StringUtil.h"                // compareIgnoreCase
#include "../../Game/Objects/IObject.h"
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Game/Objects/IObjectTemplateComponent.h"   // getActiveTemplateComponent
#include "../../Core/Ids.h"                             // IID_IObjectTemplateComponent

#include <cstdlib>                               // atoi
#include <istream>
#include <ostream>
#include <string>

namespace dice {
namespace hfe {
namespace world {

// Returns the active template's requested class face, or null for no active template/wrong class.
// Requires the template manager to be initialized; it is not null-checked.
IObjectTemplate* getActiveTemplate(unsigned int classId)
{
    IObjectTemplate* active = objectTemplateManager->getActiveObjectTemplate();   // +0x50
    if (!active)
        return 0;
    return static_cast<IObjectTemplate*>(active->queryImplementation(classId));   // +0x50 on the template
}

// Gets a component from an active template implementing CID_ObjectTemplate.
// The two-ID overload additionally queries IObjectTemplateComponent then the requested implementation;
// any failed step returns null.
IObjectTemplateComponent* getActiveTemplateComponent(unsigned int iid)
{
    IObjectTemplate* active = getActiveTemplate(CID_ObjectTemplate);              // the is-A gate
    if (!active)
        return 0;
    return active->queryComponent(iid);                                            // ELF +0x5C / PE +0x58
}

void* getActiveTemplateComponent(unsigned int iid, unsigned int cid)
{
    IObjectTemplateComponent* comp = getActiveTemplateComponent(iid);
    if (!comp)
        return 0;
    IObjectTemplateComponent* typed = static_cast<IObjectTemplateComponent*>(
        comp->queryInterface(IID_IObjectTemplateComponent));                       // +0x0C, 50379
    if (!typed)
        return 0;
    return typed->queryImplementation(cid);                                         // +0x28
}

// The pointer proxies' stream operators - see the header block. All four read end to end.

std::istream& operator>>(std::istream& in, IObjectTemplate_ptrproxy& proxy)
{
    std::string name;
    in >> name;
    proxy.ptr = objectTemplateManager->getTemplate(name);                     // +0x24
    return in;
}


std::ostream& operator<<(std::ostream& out, const IObjectTemplate_ptrproxy& proxy)
{
    if (proxy.ptr)
        out << proxy.ptr->getName();                                          // +0x2C
    else
        out << "NoObjectTemplate";
    return out;
}


std::istream& operator>>(std::istream& in, IObject_ptrproxy& proxy)
{
    std::string token;
    in >> token;
    if (compareIgnoreCase(token.c_str(), "id", 2) != 0)
        proxy.ptr = objectManager->getObjectFromNameHash(token);              // +0x24
    else
        proxy.ptr = objectManager->getObjectFromGrid(static_cast<unsigned int>(atoi(token.c_str() + 2)));   // +0x30
    return in;
}


std::ostream& operator<<(std::ostream& out, const IObject_ptrproxy& proxy)
{
    if (!proxy.ptr)
    {
        out << "NoObject";
        return out;
    }
    if (!proxy.ptr->getName().empty())                                       // +0x64
        out << proxy.ptr->getName();
    else
        out << "id" << proxy.ptr->getObjectId();                              // +0x10
    return out;
}

} // namespace world
} // namespace hfe
} // namespace dice
