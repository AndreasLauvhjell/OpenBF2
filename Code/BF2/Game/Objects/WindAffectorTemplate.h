// Per-object wind settings with authored type name and parsed enum. [INFERRED] Source placement.
// windBlowTime and the legacy widnBlowTime spelling address the same field.
// [PLATFORM] String storage changes the layout between ELF and PE.

#ifndef GAME_OBJECTS_WINDAFFECTORTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_WINDAFFECTORTEMPLATE_H

#include <string>

#include "../../Math/Vec3.h"
#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class WindAffector;

// Inherits getInterfaceID from IWindAffectorTemplate without adding an override.
// setWindType takes its string by reference; preserve virtual declaration order.
class WindAffectorTemplate : public IWindAffectorTemplate, public io::IConsoleSaveable
{
public:
    WindAffectorTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual void  setWindType(const std::string&);                   // string->enum lookup
    virtual const std::string& getWindType() const;
    virtual int   getWindTypeEnum() const;
    virtual void  setWindIsDynamic(bool);        virtual bool  getWindIsDynamic() const;
    virtual void  setWindDirection(const Math::Vec3&);
    virtual const Math::Vec3& getWindDirection() const;
    virtual void  setWindSpeed(float);           virtual float getWindSpeed() const;
    virtual void  setWindFalloff(float);         virtual float getWindFalloff() const;
    virtual void  setWindRadialFalloff(float);   virtual float getWindRadialFalloff() const;
    virtual void  setWindBlowTime(float);        virtual float getWindBlowTime() const;
    virtual ~WindAffectorTemplate();
    virtual bool  makeScript(io::IStream*);                          // +0x70 last

protected:
    friend class WindAffector;

    /*  8 */ int   m_refCount;

    /* 12 */ bool  m_windIsDynamic;
    /* 13 */ bool  _byteD;                        // [UNRESOLVED]
    /* 14 */ bool  _byteE;
    /* 15 */ bool  _byteF;

    /* 16 */ int         m_windTypeEnum;          // parsed
    /* 20 */ std::string m_windType;              // authored

    /* 24 */ Math::Vec3  m_windDirection;         // 24,28,32
    /* 36 */ float       m_windSpeed;
    /* 40 */ float       m_windFalloff;
    /* 44 */ float       m_windRadialFalloff;
    /* 48 */ float       m_windBlowTime;          // both windBlowTime and widnBlowTime write here
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WINDAFFECTORTEMPLATE_H
