// Free object/replication helpers grouped by their original translation unit; filename [INFERRED].

#ifndef SCENE_OBJECT_OBJECTUTIL_H     // [INFERRED] guard style
#define SCENE_OBJECT_OBJECTUTIL_H

#include <string>
#include <vector>

#include "../../Core/IBase.h"    // geom::IPortalMesh derives from it; getPortalMesh returns one
#include "../../Math/Vec3.h"     // geom::IPortalMesh::getPortalVertex

namespace dice {
namespace hfe {
namespace world {

class IObjectTemplate;

class IObject;

// Build and attach the object's replication component. Returns whether the object ended up in a
// consistent state, which is not the same as "a component was created" - see the body.
bool setNetworkableComp(IObject* object, unsigned int networkableClassId);

// 0x831B730 / PE 0x6D8D50. Climb through the inline parent link and return the nearest object whose
// getNetworkable() answer is non-null. Both ports perform the networkable dispatch once per level.
IObject* findClosestParentHavingNetworkable(IObject* object);

// ELF 0x831CBD0 / PE 0x6D9520. Walk from `nearObject` toward `root` until the first object whose
// template has the requested class id. The root is a boundary, not a candidate.
IObject* findClosestChildOfType(IObject* root, IObject* nearObject, unsigned int classId);

// Depth-first child/sibling walk used to encode a rotational-bundle child in sticky-
// projectile traffic. `index` counts matching templates encountered before `wanted`; true means
// the wanted object was found. The reference type is fixed by the mangled `Ri` suffix.
bool getChildIndexOfType(IObject* root, IObject* wanted, unsigned int classId, int& index);

// ELF 0x831CCA0 / PE 0x6D9020. Resolve the receiver's child index. Unlike the encoder above,
// recursion visits only children whose own template matches classId. Same inferred TU placement.
IObject* findChildTypeIndex(IObject* root, int wantedIndex, unsigned int classId, int& index);

// Finds an EngineTemplate object with the exact name, checking self, sibling subtrees,
// then children. Returns null when absent.
IObject* findEngine(IObject* object, const std::string& name);

// Clears flag 0x40 recursively through named bundle children; null templates are ignored.
// [INFERRED] The flag controls precaching.
void turnOffPreCacheOnTemplateAndAllChildren(IObjectTemplate* objectTemplate);

// Applies flags to the object itself, then all descendants; placement [INFERRED placement].
// Qualify calls inside Object subclasses: the same-named virtual only updates descendants.
void updateFlagsRecursive(IObject* object, unsigned int set, unsigned int clear);

// Returns the portal mesh or null; on success the optional output receives its owning object.
// No parent traversal occurs. The pointer is borrowed.
IBase* getPortalMesh(IObject* object, IObject** portalObject);

} // namespace world

// Portal polygon interface returned by getPortalMesh; header placement is [CHOSEN PLACEMENT].
// The four observed methods follow IBase directly, without a virtual destructor. getPortal precedes
// getPortalVertex despite AIObjectCover's different order. Any later virtual slots remain unknown.
namespace geom {

class IPortalMesh : public IBase
{
public:
    virtual unsigned int getPortalCount() const = 0;                        // +0x10
    virtual unsigned int getPortalVertexCount(unsigned int portal) const = 0;  // +0x14
    virtual void getPortal(unsigned int portal,
                           std::vector<Math::Vec3>& out) const = 0;         // +0x18
    virtual const Math::Vec3& getPortalVertex(unsigned int portal,
                                              unsigned int vertex) const = 0;  // +0x1C
};

} // namespace geom

namespace world {

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_OBJECTUTIL_H
