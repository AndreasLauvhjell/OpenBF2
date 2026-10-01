// Path: [INFERRED], beside PostProcessCompTemplate.h.
// Contents: complete template and runtime component recovered from the symbolized Linux ELF
// (template 0x8227E20..0x82287C0, runtime 0x8227C20..0x8227E10) and cross-checked against
// BF2.exe (template vtables 0x9056D8/0x9056C0, runtime vtable 0x90B830).

#include "PostProcessCompTemplate.h"

#include "IPostProcessComp.h"
#include "../../Core/Ids.h"
#include "../../IO/Stream/IStream.h"

#include <sstream>
#include <string>

namespace dice {
namespace hfe {
namespace world {

// ELF C2/C1 ctors 0x8227C20/0x8227C40 / PE ctor 0x689EB0. The template is borrowed and the object pointer is
// deliberately untouched until init() or setObject(), exactly as in both shipped constructors.
class PostProcessComp : public IPostProcessComp
{
public:
    explicit PostProcessComp(PostProcessCompTemplate* componentTemplate);

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getInterfaceID() const;
    virtual void init(IObject*);
    virtual void setObject(IObject*);
    virtual IObject* getObject() const;
    virtual bool hasUpdate() const;
    virtual void handleUpdate(float);
    virtual PostProcessInfo* getInfo();
    virtual ~PostProcessComp();

private:
    int m_refCount;                          // +0x04
    PostProcessCompTemplate* m_template;     // +0x08 borrowed
    IObject* m_object;                       // +0x0C ctor deliberately leaves indeterminate
};

PostProcessComp::PostProcessComp(PostProcessCompTemplate* componentTemplate)
    : m_refCount(1)
    , m_template(componentTemplate)
{
}

PostProcessComp::~PostProcessComp()
{
}

void PostProcessComp::addRef()
{
    ++m_refCount;
}

int PostProcessComp::getRef()
{
    return m_refCount;
}

int PostProcessComp::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* PostProcessComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IPostProcessComp)
        return const_cast<PostProcessComp*>(this);
    return 0;
}

unsigned int PostProcessComp::getInterfaceID() const
{
    return IID_IPostProcessComp;
}

void PostProcessComp::init(IObject* object)
{
    m_object = object;
}

void PostProcessComp::setObject(IObject* object)
{
    m_object = object;
}

IObject* PostProcessComp::getObject() const
{
    return m_object;
}

// Named overrides in both ports rather than inherited IObjectComponent defaults.
bool PostProcessComp::hasUpdate() const
{
    return false;
}

void PostProcessComp::handleUpdate(float)
{
}

PostProcessInfo* PostProcessComp::getInfo()
{
    // Borrowed four-float block: ELF returns template+0x18; VC7.1's larger strings put the same
    // source member at +0x48 in BF2.exe.
    return reinterpret_cast<PostProcessInfo*>(&m_template->m_tvInterference);
}

PostProcessCompTemplate::PostProcessCompTemplate()
    : m_refCount(1)
    , m_needsBuild(true)
    , m_tvInterference(0.05f)
    , m_tvDistortionRoll(0.1f)
    , m_tvDistortionFreq(0.5f)
    , m_tvDistortionScale(0.5f)
{
    // Both strings are default-constructed and otherwise untouched by the shipped constructor.
    // _padD is likewise deliberately not initialized.
}

PostProcessCompTemplate::~PostProcessCompTemplate()
{
}

void PostProcessCompTemplate::addRef()
{
    ++m_refCount;
}

int PostProcessCompTemplate::getRef()
{
    return m_refCount;
}

int PostProcessCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* PostProcessCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IPostProcessCompTemplate)
    {
        return const_cast<PostProcessCompTemplate*>(this);
    }
    if (iid == io::IID_IConsoleSaveable)
    {
        return static_cast<io::IConsoleSaveable*>(
            const_cast<PostProcessCompTemplate*>(this));
    }
    return 0;
}

unsigned int PostProcessCompTemplate::getClassID() const
{
    return CID_PostProcessCompTemplate;
}

unsigned int PostProcessCompTemplate::getInterfaceID()
{
    return IID_IPostProcessCompTemplate;
}

IObjectComponent* PostProcessCompTemplate::createComponent()
{
    if (m_needsBuild)
        m_needsBuild = false;
    return new PostProcessComp(this);
}

void PostProcessCompTemplate::objectCreated(IObject*)
{
}

void PostProcessCompTemplate::objectDestroyed(IObject*)
{
}

void PostProcessCompTemplate::setTemplate(IObjectTemplate*)
{
}

void* PostProcessCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void PostProcessCompTemplate::setTvInterference(float value)
{
    m_tvInterference = value;
}

float PostProcessCompTemplate::getTvInterference()
{
    return m_tvInterference;
}

void PostProcessCompTemplate::setTvDistortionRoll(float value)
{
    m_tvDistortionRoll = value;
}

float PostProcessCompTemplate::getTvDistortionRoll()
{
    return m_tvDistortionRoll;
}

void PostProcessCompTemplate::setTvDistortionFreq(float value)
{
    m_tvDistortionFreq = value;
}

float PostProcessCompTemplate::getTvDistortionFreq()
{
    return m_tvDistortionFreq;
}

void PostProcessCompTemplate::setTvDistortionScale(float value)
{
    m_tvDistortionScale = value;
}

float PostProcessCompTemplate::getTvDistortionScale()
{
    return m_tvDistortionScale;
}

// emit only authored deviations, in this order, then make one write.
bool PostProcessCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_tvInterference != 0.05f)
        script << "ObjectTemplate.postProcess.tvInterference " << m_tvInterference << std::endl;
    if (m_tvDistortionRoll != 0.1f)
        script << "ObjectTemplate.postProcess.tvDistortionRoll " << m_tvDistortionRoll << std::endl;
    if (m_tvDistortionFreq != 0.5f)
        script << "ObjectTemplate.postProcess.tvDistortionFreq " << m_tvDistortionFreq << std::endl;
    if (m_tvDistortionScale != 0.5f)
        script << "ObjectTemplate.postProcess.tvDistortionScale " << m_tvDistortionScale << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(PostProcessComp) == 0x10, "BF2.exe PostProcessComp size");
#elif defined(__i386__)
static_assert(sizeof(PostProcessComp) == 0x10, "32-bit ELF PostProcessComp size");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
