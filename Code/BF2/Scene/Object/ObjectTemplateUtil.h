// Active-template/component lookup and stream proxy helpers.
// [CHOSEN] Filename; the original translation-unit grouping is known but its path is not attested.

#ifndef SCENE_OBJECT_OBJECTTEMPLATEUTIL_H     // [INFERRED] guard style
#define SCENE_OBJECT_OBJECTTEMPLATEUTIL_H

#include <iosfwd>

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IObjectTemplate;
class IObjectTemplateComponent;

// The bridge every generated ObjectTemplate.* property binding stands on: ConsoleClass127's
// isObjectActive is `getActiveTemplate(CID) != 0` and its executor applies the property through
// the same call (BfSound exemplar, 0x83DD1E0/0x83DD510; the world Form-B classes 1272/1273/265
// all do the same). `ObjectTemplate.create` sets the manager's cursor; this narrows it by class.
IObjectTemplate* getActiveTemplate(unsigned int classId);

// Returns an active ObjectTemplate component, optionally narrowed by implementation CID.
// Requires objectTemplateManager; absent template/component/interfaces return null. Returned pointers are borrowed.
IObjectTemplateComponent* getActiveTemplateComponent(unsigned int iid);
void*                     getActiveTemplateComponent(unsigned int iid, unsigned int cid);

// One-pointer console adapters for name/pointer conversion.
// Templates parse one name and format NoObjectTemplate when null. Objects accept names or id<n>,
// and format the name when present, otherwise id<n> or NoObject.
struct IObjectTemplate_ptrproxy
{
    const IObjectTemplate* ptr;
    IObjectTemplate_ptrproxy() : ptr(0) {}
};

struct IObject_ptrproxy
{
    IObject* ptr;
    IObject_ptrproxy() : ptr(0) {}
    IObject_ptrproxy(IObject* p) : ptr(p) {}
};

std::istream& operator>>(std::istream& in, IObjectTemplate_ptrproxy& proxy);
std::ostream& operator<<(std::ostream& out, const IObjectTemplate_ptrproxy& proxy);
std::istream& operator>>(std::istream& in, IObject_ptrproxy& proxy);
std::ostream& operator<<(std::ostream& out, const IObject_ptrproxy& proxy);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_OBJECTTEMPLATEUTIL_H
