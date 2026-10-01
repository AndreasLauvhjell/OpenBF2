// Radio template message storage and console serialization. Source placement is [INFERRED].

#include "../RadioGLCompTemplate.h"

#include "RadioGLComp.h"
#include "../../../Core/Ids.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// both vptrs, refCount 1, needsBuild true, all seven strings default-construct
// (empty-rep attach) - and then the shipped constructor redundantly assigns "" onto the five
// message strings, five separate `assign(<empty literal>, strlen(<empty literal>))` calls
// (0x8223EE6..; the source literal is a shared empty-string byte the IDB mislabels "filename").
// A no-op after default construction, preserved because it is what the binary does.
// _byteD/_byteE/_byteF are NOT written - as shipped.
RadioGLCompTemplate::RadioGLCompTemplate()
    : m_refCount(1)
    , m_needsBuild(true)
{
    m_failMessage.assign("");
    m_useMessage.assign("");
    m_destroyedMessage.assign("");
    m_repairedMessage.assign("");
    m_spottedMessage.assign("");
}

// D1/D0 at vtable +0x54/+0x58 - the compiler's member teardown only (the seven strings drop their
// reps, in reverse declaration order). No hand-written body in the ELF beyond the vptr restores.
RadioGLCompTemplate::~RadioGLCompTemplate()
{
}

// IBase - release() dispatches the deleting destructor through the vtable (+0x58, the Itanium D0)
// and tests `this` after decrementing through it, as shipped; `delete this` is the typed spelling.
void RadioGLCompTemplate::addRef()  { ++m_refCount; }
int  RadioGLCompTemplate::getRef()  { return m_refCount; }

int RadioGLCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// Three ids answer with `this`; IConsoleSaveable answers with the second base at +4, which is what
// the RTTI says (IRadioGLCompTemplate @ +0, io::IConsoleSaveable @ +4).
void* RadioGLCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IRadioGLCompTemplate)
        return const_cast<RadioGLCompTemplate*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<RadioGLCompTemplate*>(this));
    return 0;
}

unsigned int RadioGLCompTemplate::getClassID() const  { return CID_RadioGLCompTemplate; }   // 50358
unsigned int RadioGLCompTemplate::getInterfaceID()    { return IID_IRadioGLCompTemplate; }  // 50359

// "Give me the implementation if you ARE this class" - asks its own getClassID through the vtable
// rather than returning the constant, so a subclass answers for itself (Armor's exact shape).
void* RadioGLCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == getClassID())
        return this;
    return 0;
}

// createComponent - 0x8224450, 63 B. THE RW-6b slot: clears the needs-build flag (the same
// test-then-clear the constructor armed, Armor's m_needsBuild semantics) and news the runtime
// component bound to this template. dalian's mobileradar_ch_dest reaches this at map load.
IObjectComponent* RadioGLCompTemplate::createComponent()
{
    if (m_needsBuild)
        m_needsBuild = false;
    return new RadioGLComp(this);
}

// Three five-byte bodies - every template component gets these hooks; this one declines all three.
void RadioGLCompTemplate::objectCreated(IObject*)          {}
void RadioGLCompTemplate::objectDestroyed(IObject*)        {}
void RadioGLCompTemplate::setTemplate(IObjectTemplate*)    {}

// the five console-backed message pairs
// Setters take std::string BY value (mangling ...Ss); getters return std::string BY value - every
// ELF caller receives a hidden-sret copy it destroys itself (RadioGLComp::playFailMessage and
// twins, 41 B of copy-and-drop each). 17 B setters / sret-copy getters.
void        RadioGLCompTemplate::setFailMessage(std::string v)       { m_failMessage = v; }
std::string RadioGLCompTemplate::getFailMessage()                    { return m_failMessage; }
void        RadioGLCompTemplate::setUseMessage(std::string v)        { m_useMessage = v; }
std::string RadioGLCompTemplate::getUseMessage()                     { return m_useMessage; }
void        RadioGLCompTemplate::setDestroyedMessage(std::string v)  { m_destroyedMessage = v; }
std::string RadioGLCompTemplate::getDestroyedMessage()               { return m_destroyedMessage; }
void        RadioGLCompTemplate::setRepairedMessage(std::string v)   { m_repairedMessage = v; }
std::string RadioGLCompTemplate::getRepairedMessage()                { return m_repairedMessage; }
void        RadioGLCompTemplate::setSpottedMessage(std::string v)    { m_spottedMessage = v; }
std::string RadioGLCompTemplate::getSpottedMessage()                 { return m_spottedMessage; }

// no quotes or escaping; empty messages are omitted.
bool RadioGLCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!m_failMessage.empty()) out << "ObjectTemplate.Radio.failMessage " << m_failMessage << std::endl;
    if (!m_useMessage.empty()) out << "ObjectTemplate.Radio.useMessage " << m_useMessage << std::endl;
    if (!m_destroyedMessage.empty()) out << "ObjectTemplate.Radio.destroyedMessage " << m_destroyedMessage << std::endl;
    if (!m_repairedMessage.empty()) out << "ObjectTemplate.Radio.repairedMessage " << m_repairedMessage << std::endl;
    if (!m_spottedMessage.empty()) out << "ObjectTemplate.Radio.spottedMessage " << m_spottedMessage << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
