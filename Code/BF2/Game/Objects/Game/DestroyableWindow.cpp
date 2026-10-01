// Path: [INFERRED]. Runtime/template pairing follows the ELF TU's global constructor.
// Evidence: ELF 0x8174430..0x81754FF; PE 0x590200/210/220/2A0/380.
#include "DestroyableWindowsBundle.h"
#include "../TemplateLeaves.h"
#include "../../GameLogic/GameLogicCollision.h"
#include "../../../Core/Ids.h"
#include "../../../IO/Stream/IStream.h"
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Physics/ICollisionMesh.h"

#include <cmath>
#include <sstream>

namespace dice { namespace hfe { namespace world {

DestroyableWindow::DestroyableWindow(const DestroyableWindowTemplate* objectTemplate)
    : SimpleObject(objectTemplate), m_refCount(1), m_isBroken(false) {}
DestroyableWindow::~DestroyableWindow() {}
void DestroyableWindow::addRef() { ++m_refCount; }
int DestroyableWindow::getRef() { return m_refCount; }
int DestroyableWindow::release()
{
    const int result = --m_refCount;
    if (!result) delete this;
    return result;
}
void* DestroyableWindow::queryInterface(unsigned int iid) const
{
    DestroyableWindow* self = const_cast<DestroyableWindow*>(this);
    if (iid == IID_IBase || iid == IID_IObject) return static_cast<IObject*>(self);
    if (iid == io::IID_IConsoleSaveable) return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

// Retail is empty (ELF 0x8174570, PE 0x590200): the parent replicates the panes.
void DestroyableWindow::initNetworkableComp() {}
bool DestroyableWindow::getIsBroken() const { return m_isBroken; }

void DestroyableWindow::setIsBroken(bool broken)
{
    if (broken != m_isBroken)
    {
        if (broken)
        {
            updateFlags(0, 0x100);
            if (hasCollisionLod(2))
            {
                ICollisionMesh* mesh = static_cast<ICollisionMesh*>(m_collisionMesh);
                const LocalBoundingBox* box = mesh->getBoundingBox(2);
                const unsigned short* faces = static_cast<const unsigned short*>(mesh->getFaceVector(2));
                const Vec3* vertices = static_cast<const Vec3*>(mesh->getVertexVector(2));
                const Vec3& a = vertices[faces[0]];
                const Vec3& b = vertices[faces[1]];
                const Vec3& c = vertices[faces[2]];
                const Vec3 ab = {b.x - a.x, b.y - a.y, b.z - a.z};
                const Vec3 ac = {c.x - a.x, c.y - a.y, c.z - a.z};
                Vec3 normal = {ab.y * ac.z - ab.z * ac.y,
                               ab.z * ac.x - ab.x * ac.z,
                               ab.x * ac.y - ab.y * ac.x};
                const float lengthSq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
                if (!(std::fabs(lengthSq - 1.0f) < 1.1920929e-7f))
                {
                    if (!(std::fabs(lengthSq) < 1.1920929e-7f))
                    {
                        const float inverse = 1.0f / std::sqrt(lengthSq);
                        normal.x *= inverse; normal.y *= inverse; normal.z *= inverse;
                    }
                    else normal.x = normal.y = normal.z = 0.0f;
                }
                if (box->m_derivedDataDirty) const_cast<LocalBoundingBox*>(box)->calcRadius();
                const Vec3 centre = box->m_centre;
                const Mat4& absolute = getAbsoluteTransformation();
                const Vec3 position = {
                    absolute.m[0] * centre.x + absolute.m[4] * centre.y + absolute.m[8] * centre.z + absolute.m[12],
                    absolute.m[1] * centre.x + absolute.m[5] * centre.y + absolute.m[9] * centre.z + absolute.m[13],
                    absolute.m[2] * centre.x + absolute.m[6] * centre.y + absolute.m[10] * centre.z + absolute.m[14]};
                const Vec3 up = {0, 1, 0};
                // Retail transforms the centre, but leaves the triangle normal in local space.
                const Mat4 effectTransform = GameLogicCollision::getCollisionEffectTransformation(position, up, normal);
                IObject* root = getRootParent(this);
                if (root && root->m_template->getClassID() == CID_DestroyableWindowsBundleTemplate)
                {
                    DestroyableWindowsBundleTemplate* parentTemplate = const_cast<DestroyableWindowsBundleTemplate*>(
                        static_cast<const DestroyableWindowsBundleTemplate*>(root->m_template));
                    IObjectTemplate* effectTemplate = parentTemplate->getEffectTemplate(*box);
                    if (effectTemplate)
                    {
                        IObject* effect = effectTemplate->createObject();
                        effect->setAbsoluteTransformation(effectTransform);
                        effect->init();
                        effect->handleMessage(static_cast<TemplateMessage>(0), 0, false);
                    }
                }
            }
        }
        else updateFlags(0x100, 0);
    }
    m_isBroken = broken;
}

bool DestroyableWindow::handleCollision(IObject* other, const Vec3& a, const Vec3& b,
    const Vec3& c, int first, int second, CollisionFaceInfo* firstFace, CollisionFaceInfo* secondFace)
{
    if (m_isBroken) return false;
    IObject* root = getRootParent(this);
    if (root && root->m_template->getClassID() == CID_DestroyableWindowsBundleTemplate && getGeometryPart() != -1)
        static_cast<DestroyableWindowsBundle*>(root)->handleChildCollision(this);
    return SimpleObject::handleCollision(other, a, b, c, first, second, firstFace, secondFace);
}

DestroyableWindowTemplate::DestroyableWindowTemplate() : _dword168(0), _dword16C(1)
{
    m_refCount = 1;
}
IObject* DestroyableWindowTemplate::createObject() const { return new DestroyableWindow(this); }
bool DestroyableWindowTemplate::makeScript(io::IStream* stream)
{
    if (!SimpleObjectTemplate::makeScript(stream)) return false;
    // Both ports construct and write an empty suffix after the base script.
    std::ostringstream out;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}
} } }
