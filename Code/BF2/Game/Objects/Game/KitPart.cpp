// Path: [INFERRED], see KitPart.h.
// ..0x818843D; dedicated PE 0x59A2B0..0x59A569.
#include "KitPart.h"
#include "../../../Animation/BoneAnimation/BoneManager.h"
#include "../../../Core/Ids.h"

namespace dice { namespace hfe { namespace world {

KitPartTemplate::KitPartTemplate() : m_kitPartIndex(-1), m_rank(1) {}
KitPartTemplate::~KitPartTemplate() {}
unsigned int KitPartTemplate::getClassID() const { return CID_KitPartTemplate; }
void* KitPartTemplate::queryImplementation(unsigned int cid)
{
    if (cid == CID_KitPartTemplate) return this;
    return BundleTemplate::queryImplementation(cid);
}
IObject* KitPartTemplate::createObject() const { return new KitPart(this); }
void KitPartTemplate::setBoneName(std::string name)
{
    m_boneNameIndex = static_cast<short>(anim::BoneManager::getInstance().addBoneName(name));
}
const std::string& KitPartTemplate::getBoneName()
{
    return anim::BoneManager::getInstance().getBoneName(m_boneNameIndex);
}
short KitPartTemplate::getBoneNameIndex() { return m_boneNameIndex; }
void KitPartTemplate::setCopyLinksCount(int count) { m_copyLinksCount = count; }
int KitPartTemplate::getCopyLinksCount() { return m_copyLinksCount; }

KitPart::KitPart(const KitPartTemplate* objectTemplate) : Bundle(objectTemplate), m_refCount(1) {}
KitPart::~KitPart() {}
void KitPart::addRef() { ++m_refCount; }
int KitPart::getRef() { return m_refCount; }
int KitPart::release()
{
    const int count = --m_refCount;
    if (!count) delete this;
    return count;
}
void* KitPart::queryInterface(unsigned int iid) const
{
    KitPart* self = const_cast<KitPart*>(this);
    if (iid == IID_IBase || iid == IID_IObject) return static_cast<IObject*>(self);
    if (iid == IID_IKitPart) return static_cast<IKitPart*>(self);
    if (iid == io::IID_IConsoleSaveable) return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}
int KitPart::getRank() const { return m_kitPartTemplate->m_rank; }
int KitPart::getKitPartIndex() { return m_kitPartTemplate->m_kitPartIndex; }
void KitPart::setTeam(int team)
{
    m_team = team;
    const KitPartTemplate* objectTemplate = static_cast<const KitPartTemplate*>(m_template);
    const auto geometry = objectTemplate->m_teamGeometry.find(team);
    // the fallback lookup's result is discarded (EBX keeps the first iterator).
    if (geometry == objectTemplate->m_teamGeometry.end())
        (void)objectTemplate->m_teamGeometry.find(0);
    if (geometry != objectTemplate->m_teamGeometry.end())
        setGeometry(this, geometry->second);
}

} } }
