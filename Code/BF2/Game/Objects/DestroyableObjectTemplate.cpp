// Destroyable-object creation and script serialization.

#include "DestroyableObjectTemplate.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>

#include "../../Core/Ids.h"
#include "Game/DestroyableObject.h"               // createObject's product

namespace dice {
namespace hfe {
namespace world {

// C1/C2, 164 B: BundleTemplate base, then every field - the walkable-by-default, one-respawn
// destroyable. Tail: updateFlags(1, 0) - flag 1, unlike the templates that set flag 2.
DestroyableObjectTemplate::DestroyableObjectTemplate()
{
    m_refCount = 1;                               // +0x17C
    respawnDelay = 0.0f;                          // +0x180
    nrOfRespawns = 1;                             // +0x184
    usedByAi = false;                             // +0x188
    initiallyWalkable = true;                     // +0x189
    minCorner[0] = 0.0f; minCorner[1] = 0.0f; minCorner[2] = 0.0f;
    maxCorner[0] = 0.0f; maxCorner[1] = 0.0f; maxCorner[2] = 0.0f;

    updateFlags(1, 0);
}

// (D1): vptr resets + ~BundleTemplate - nothing manual survives the translation.
DestroyableObjectTemplate::~DestroyableObjectTemplate()
{
}

// The per-class quartet - refcount +0x17C, delete on the 1->0 edge through the D0 slot.
void DestroyableObjectTemplate::addRef()  { ++m_refCount; }
int  DestroyableObjectTemplate::getRef()  { return m_refCount; }

int DestroyableObjectTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

// the Bundle-chain hop: IBase/IObjectTemplate -> this, ISimpleObjectTemplate -> +0x30,
// IBundleTemplate -> +0x164, IConsoleSaveable -> +8, else 0. Same shape as
// GenericProjectileTemplate's minus the projectile interface.
void* DestroyableObjectTemplate::queryInterface(unsigned int iid) const
{
    DestroyableObjectTemplate* self = const_cast<DestroyableObjectTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return static_cast<IObjectTemplate*>(self);
    if (iid == IID_ISimpleObjectTemplate)
        return static_cast<ISimpleObjectTemplate*>(self);
    if (iid == IID_IBundleTemplate)
        return static_cast<IBundleTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DestroyableObjectTemplate::getClassID() const
{
    return CID_DestroyableObjectTemplate;
}

// the standard one-CID hop into Bundle's chain (a literal tail-jump in the ELF).
void* DestroyableObjectTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_DestroyableObjectTemplate)
        return this;
    return BundleTemplate::queryImplementation(classId);
}

// new(0x140) DestroyableObject(this). the tenth per-class object
// (Game/Objects/Game/DestroyableObject.{h,cpp}), reached from a level's StaticObjects.con.
IObject* DestroyableObjectTemplate::createObject() const
{
    return new DestroyableObject(this);
}

// . Retail explicitly skips BundleTemplate::makeScript.
bool DestroyableObjectTemplate::makeScript(io::IStream* stream)
{
    if (!SimpleObjectTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    if (respawnDelay != 0.0f) out << "ObjectTemplate.respawnDelay " << respawnDelay << std::endl;
    if (nrOfRespawns != 1) out << "ObjectTemplate.nrOfRespawns " << nrOfRespawns << std::endl;
    if (usedByAi)
    {
        out << "ObjectTemplate.usedByAi " << usedByAi << std::endl;
        out << "ObjectTemplate.initiallyWalkable " << initiallyWalkable << std::endl;
        out << "ObjectTemplate.minCorner " << minCorner[0] << '/' << minCorner[1] << '/' << minCorner[2] << std::endl;
        out << "ObjectTemplate.maxCorner " << maxCorner[0] << '/' << maxCorner[1] << '/' << maxCorner[2] << std::endl;
    }
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
