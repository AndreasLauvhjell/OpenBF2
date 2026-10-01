// [INFERRED] path beside Trigger.cpp. ELF81C68A0..81C80FF / PE544270..544E9F.
// This class's moving-target update is a NON-virtual overload; ordinary object ticks
// still inherit SimpleObject::handleUpdate.
#include "Triggerable.h"
#include "TriggerableManager.h"
#include "../TemplateLeaves.h"
#include "../../../Core/Ids.h"
#include "../../../Scene/Object/ObjectUtil.h"
#include "../../../Physics/PhysicsUtil.h"
#include "../../../Sound/NullSound.h"
#include <cmath>
#include <cstdlib>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace dice { namespace hfe { namespace world {
namespace {
Vec3 subtract(const Vec3& a, const Vec3& b) { const Vec3 v = {a.x-b.x,a.y-b.y,a.z-b.z}; return v; }
float length(const Vec3& v) { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
void normalize(Vec3& v)
{
    const float squared = v.x*v.x+v.y*v.y+v.z*v.z;
    if (std::fabs(squared-1.0f) >= 1.1920929e-7f)
    {
        if (std::fabs(squared) < 1.1920929e-7f) { v.x=0; v.y=0; v.z=0; }
        else { const float scale=1.0f/std::sqrt(squared); v.x*=scale; v.y*=scale; v.z*=scale; }
    }
}
Vec3 targetRotation(IObject* object)
{
    const Mat4& transform = object->getParent() ? object->getRelativeTransformation()
                                              : object->getAbsoluteTransformation();
    float z, x, y;
    getRotationZXY(transform, z, x, y);
    const Vec3 result = {-y, -x, -z};
    return result;
}
}
TriggerableTarget::TriggerableTarget(const TriggerableTargetTemplate* objectTemplate)
    : Bundle(objectTemplate), m_refCount(1), m_template(objectTemplate), m_name(""),
      m_state(0), m_reverse(0), m_cyclesRemaining(1), m_delay(0),
      m_vector15C{0,0,0}, m_float168(0),
      m_current(Vec3::getIdentity()), m_finish(Vec3::getIdentity()),
      m_startPoint(Vec3::getIdentity()), m_endPoint(Vec3::getIdentity()),
      m_direction(Vec3::getIdentity()), m_savedPosition(Vec3::getIdentity()),
      m_savedSecondaryPosition(Vec3::getIdentity()), m_savedRotation(Vec3::getIdentity()),
      m_savedSecondaryRotation(Vec3::getIdentity()), m_startupSound(0),
      m_movingSound(0), m_finishedSound(0), m_initialized(false)
{
    updateFlags(0x6808, 0);
    m_startupSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate("Startup", this));
    if (m_startupSound) m_startupSound->setParent(this);
    m_movingSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate("Moving", this));
    if (m_movingSound) m_movingSound->setParent(this);
    m_finishedSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate("Finished", this));
    if (m_finishedSound) m_finishedSound->setParent(this);
    TriggerableTarget::setName(objectTemplate->getName());
    m_layer = 1;
}
TriggerableTarget::~TriggerableTarget()
{
    // Both ports omit removeTriggerableTarget here, unlike Triggerable's destructor.
    if (m_startupSound) m_startupSound->release();
    if (m_movingSound) m_movingSound->release();
    if (m_finishedSound) m_finishedSound->release();
}
void TriggerableTarget::addRef() { ++m_refCount; }
int TriggerableTarget::getRef() { return m_refCount; }
int TriggerableTarget::release()
{
    const int result=--m_refCount;
    if (!result) delete this;
    return result;
}
void* TriggerableTarget::queryInterface(unsigned int iid) const
{
    TriggerableTarget* self=const_cast<TriggerableTarget*>(this);
    if (iid==IID_IBase || iid==IID_IObject) return static_cast<IObject*>(self);
    if (iid==io::IID_IConsoleSaveable) return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}
void TriggerableTarget::init()
{
    Bundle::init();
    m_createdOnLoad=true;
    if (triggerableManager) triggerableManager->addTriggerableTarget(this);
}
void TriggerableTarget::destroy(bool value) { SimpleObject::destroy(value); }
void TriggerableTarget::setName(const std::string& name) { m_name=name; }
void TriggerableTarget::handleMessage(TemplateMessage message,IPlayer* player,bool recursive)
{ Bundle::handleMessage(message,player,recursive); }
bool TriggerableTarget::handleCollision(IObject* object,const Vec3& pos,const Vec3& normal,
                                       const Vec3& velocity,int a,int b,
                                       CollisionFaceInfo* first,CollisionFaceInfo* second)
{ return Bundle::handleCollision(object,pos,normal,velocity,a,b,first,second); }
const Mat4& TriggerableTarget::getTransform() { return getAbsoluteTransformation(); }
const Vec3* TriggerableTarget::getVelocity() { return 0; }
void TriggerableTarget::initNetworkableComp()
{
    if (m_template)
    {
        if (m_template->getNetworkableInfo().empty())
        {
#if defined(_MSC_VER)
            __debugbreak();
#else
            std::abort();
#endif
        }
        // The second virtual lookup and empty test are also present in retail.
        if (!m_template->getNetworkableInfo().empty()) setNetworkableComp(this,CID_TriggerableTargetNetworkable);
    }
}
void TriggerableTarget::setPos(Vec3 position) { setAbsolutePosition(position); }
void TriggerableTarget::setRot(Vec3 rotation)
{
    Mat4 transform = getParent() ? getRelativeTransformation() : getAbsoluteTransformation();
    setRotationZXY(transform,-rotation.z,-rotation.y,-rotation.x);
    if (getParent()) setRelativeTransformation(transform);
    else setAbsoluteTransformation(transform);
}
Vec3 TriggerableTarget::getFinishPoint() { return m_reverse ? m_startPoint : m_endPoint; }
void TriggerableTarget::refresh()
{
    if (m_initialized)
    {
        setAbsolutePosition(m_savedPosition);
        setAbsolutePositionSecondary(m_savedSecondaryPosition);
        setRot(m_savedRotation);
        setRotationSecondary(m_savedSecondaryRotation);
        m_initialized=false;
        m_reverse=0;
        m_delay=0;
        m_state=0;
    }
}
void TriggerableTarget::activate()
{
    if (m_template->m_movementType==0 || m_template->m_movementType==1)
    {
        if (m_reverse) { m_current=m_endPoint; m_finish=m_startPoint; }
        else { m_current=m_startPoint; m_finish=m_endPoint; }
        m_direction=subtract(m_finish,m_current);
        normalize(m_direction);
    }
    if (m_startupSound) m_startupSound->play();
}
void TriggerableTarget::deactivate()
{
    --m_cyclesRemaining;
    m_reverse=(m_reverse==0);
    m_current=m_finish;
    if (m_template->m_movementType==0) setAbsolutePosition(m_current);
    else if (m_template->m_movementType==1) setRot(m_current);
    if (m_finishedSound) m_finishedSound->play();
}
void TriggerableTarget::handleUpdate(IObject*,Triggerable* trigger,float deltaTime)
{
    switch (m_state)
    {
    case 0:
        if (!m_initialized)
        {
            if (m_template->m_movementType==0)
            {
                if (!m_reverse) m_startPoint=getAbsolutePosition();
                m_endPoint=getAbsolutePositionSecondary();
            }
            else if (m_template->m_movementType==1)
            {
                if (!m_reverse) m_startPoint=targetRotation(this);
                m_endPoint=getRotationSecondary();
            }
            m_savedPosition=getAbsolutePosition();
            m_savedSecondaryPosition=getAbsolutePositionSecondary();
            m_savedRotation=targetRotation(this);
            m_savedSecondaryRotation=getRotationSecondary();
            m_initialized=true;
        }
        m_cyclesRemaining=m_template->m_cycles;
        m_delay=m_template->m_delayBeforeStart;
        m_state=2;
        return;
    case 2:
        m_delay-=deltaTime;
        if (!(m_delay<=0)) return;
        if (trigger->m_template->m_bool19D && m_cyclesRemaining!=m_template->m_cycles
            && trigger->isOccupied()) return;
        m_delay=m_template->m_delayBeforeCycle;
        activate();
        m_state=3;
        {
            const float radius=getPhysicBoundingRadius();
            wakeUpObjects(getAbsolutePosition(),radius,this,false);
        }
        return;
    case 3:
        {
            bool finished=false;
            if (m_template->m_movementType==0)
            {
                const float speed=deltaTime*m_template->m_movementSpeed;
                const Vec3 remaining=subtract(m_finish,m_current);
                const Vec3 step={speed*m_direction.x,speed*m_direction.y,speed*m_direction.z};
                if (length(step)<length(remaining))
                { m_current.x+=step.x; m_current.y+=step.y; m_current.z+=step.z; }
                else { m_current=m_finish; finished=true; }
                setAbsolutePosition(m_current);
            }
            else if (m_template->m_movementType==1)
            {
                for (int i=0;i<3;++i)
                {
                    float& value=(&m_current.x)[i];
                    value+=(&m_direction.x)[i]*m_template->m_movementSpeed*deltaTime;
                    if (value>360) value-=360;
                    if (value< -360) value+=360;
                }
                setRot(m_current);
                if (deltaTime*m_template->m_movementSpeed>=std::fabs(length(subtract(m_finish,m_current))))
                    finished=true;
            }
            if (finished) { deactivate(); m_state=4; }
            if (m_movingSound) m_movingSound->play();
        }
        return;
    case 4:
        m_delay-=deltaTime;
        if (!(m_delay<=0)) return;
        if (trigger->m_template->m_bool19D && trigger->isOccupied()) return;
        if (m_cyclesRemaining>0) { m_delay=m_template->m_delayBeforeCycle; m_state=2; }
        else { m_delay=m_template->m_delayBeforeEnd; m_state=5; }
        return;
    case 5: return;
    case 6: m_state=0; return;
    default: m_state=2; return;
    }
}
} } }
