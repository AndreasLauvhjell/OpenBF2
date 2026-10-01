// Camera post-processing settings for screen distortion.
// [INFERRED] source placement; string layout depends on the compiler.

#ifndef GAME_OBJECTS_POSTPROCESSCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_POSTPROCESSCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class PostProcessComp;

// Post-processing settings and script serialization; preserve virtual declaration order.
class PostProcessCompTemplate : public IPostProcessCompTemplate, public io::IConsoleSaveable
{
public:
    PostProcessCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual void  setTvInterference(float);      virtual float getTvInterference();
    virtual void  setTvDistortionRoll(float);    virtual float getTvDistortionRoll();
    virtual void  setTvDistortionFreq(float);    virtual float getTvDistortionFreq();
    virtual void  setTvDistortionScale(float);   virtual float getTvDistortionScale();
    virtual ~PostProcessCompTemplate();
    virtual bool  makeScript(io::IStream*);                          // +0x54 last

protected:
    friend class PostProcessComp;

    /*  8 */ int   m_refCount;

    /* 12 */ bool  m_needsBuild;                   // cleared by createComponent
    /* 13 */ char  _padD[3];                       // not initialized by either shipped ctor

    // These are owned std::strings, not dwords. ELF ctor/dtor 0x8227E20/0x8227F00 and PE
    // ctor/dtor 0x65A290/0x65A250 construct and destroy both. No non-lifecycle use names them.
    /* 16 */ std::string m_unknownString10;        // [UNRESOLVED] name/purpose
    /* 20 */ std::string m_unknownString14;        // [UNRESOLVED] name/purpose

    /* 24 */ float m_tvInterference;
    /* 28 */ float m_tvDistortionRoll;
    /* 32 */ float m_tvDistortionFreq;
    /* 36 */ float m_tvDistortionScale;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_POSTPROCESSCOMPTEMPLATE_H
