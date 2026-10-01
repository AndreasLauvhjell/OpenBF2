// Light configuration template. Range setters report a fatal error above their
// caps (100 for attenuation, 180 for cone angles/FOV). Direction is normalized.
// getConeAngle1 deliberately returns the second cone angle. Projector geometry is
// owned by the template and released on retyping or destruction.

#ifndef GAME_OBJECTS_VISUAL_LIGHTSOURCETEMPLATE_H   // [INFERRED] guard style
#define GAME_OBJECTS_VISUAL_LIGHTSOURCETEMPLATE_H

#include <string>
#include "../ObjectTemplate.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

namespace geom {
// [UNRESOLVED surface] - only two dispatched slots are attested (from setLightType /
// setProjectorTexture / the dtor): release at +0x08 - the quartet position, which is why IBase is
// the [CHOSEN] base - and a +0x10 method handed the projector-texture string [CHOSEN name
// setTexture]. The concrete LightGeometry (geom::CID_LightGeometry) is unregistered in this tree,
// so createInstance returns null and every use is behind the ELF's own null guard.
class ILightGeometry : public IBase
{
public:
    virtual void  setTexture(const std::string&) = 0;        // +0x10 dispatched [CHOSEN name]
};
} // namespace geom

namespace world {

// the attested name->enum table setLightType walks (compareIgnoreCase, indices 0..3).
extern const char* const LightSourceTypeNames[4];   // Directional, Point, Spot, SpotProjector

class LightSourceTemplate : public ObjectTemplate
{
public:
    LightSourceTemplate();                                    // disasm-verified

    // The per-class IBase quartet (refcount +0x30).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                  // delete on 0
    virtual void* queryInterface(unsigned int iid) const;     // IBase/IObjectTemplate/
                                                              // IConsoleSaveable only
    virtual unsigned int getClassID() const;                  // reads the CID global
    virtual ~LightSourceTemplate();                           // releases the geometry,
                                                              // then the three strings
    virtual bool writeBinary(io::IStream*) const;             // returns false (empty stub)
    virtual bool readBinary(io::IStream*);                    // returns false (empty stub)
    virtual void* queryImplementation(unsigned int classId);  // CID_LightSourceTemplate or
                                                              // CID_ObjectTemplate, flat, no
                                                              // chain
    virtual IObject* createObject() const;                    // new(0x388) LightSource
    virtual bool makeScript(io::IStream*);                    // ELF/PE recovered

    // the 72 own slots (+0xE8..+0x204 in the ELF primary table), all read
    virtual void setLightType(const std::string&);            // the name->enum map; see cpp
    virtual const std::string& getLightType() const;
    virtual int  getLightTypeEnum() const;

    virtual void setIsDynamic(bool);                virtual bool getIsDynamic() const;
    virtual void setCastsStaticShadow(bool);        virtual bool getCastsStaticShadow() const;
    virtual void setCastsDynamicShadow(bool);       virtual bool getCastsDynamicShadow() const;
    virtual void setSelfLights(bool);               virtual bool getSelfLights() const;
    virtual void setSelfShadows(bool);              virtual bool getSelfShadows() const;
    virtual void setSupportsPerVertex(bool);        virtual bool getSupportsPerVertex() const;
    virtual void setSupportsPerPixel(bool);         virtual bool getSupportsPerPixel() const;
    virtual void setSupportsGlow(bool);             virtual bool getSupportsGlow();
    virtual void setSupportsEmitter(bool);          virtual bool getSupportsEmitter();
    virtual void setAffectLightmappedObjects(bool); virtual bool getAffectLightmappedObjects() const;
    virtual void setAffectingType(int);             virtual int  getAffectingType() const;

    virtual void setDirection(const Vec3&);                   // store + normalize in place
    virtual const Vec3& getDirection() const;
    virtual void setAttenuationRange1(float);                 // cap 100 + type-6 warning
    virtual float getAttenuationRange1() const;
    virtual void setAttenuationRange2(float);                 // cap 100
    virtual float getAttenuationRange2() const;
    virtual void setColor(const Vec3&);                       // plain store
    virtual const Vec3& getColor() const;
    virtual void setSpecColor(const Vec3&);
    virtual const Vec3& getSpecColor() const;
    virtual void setHDRIntensity(float);
    virtual float getHDRIntensity() const;
    virtual void setConeAngle1(float);                        // cap 180; stores +0x88
    virtual float getConeAngle1() const;                      // [DICE-BUG] reads +0x8C
    virtual void setConeAngle2(float);                        // cap 180; stores +0x8C
    virtual float getConeAngle2() const;                      // reads +0x8C
    virtual void setProjectorTexture(const std::string&);     // guarded assign + forward
    virtual const std::string& getProjectorTexture() const;
    virtual void setProjectorFov(float);
    virtual float getProjectorFov() const;
    virtual void setProjectorAspect(float);
    virtual float getProjectorAspect() const;
    virtual void setProjectorNear(float);
    virtual float getProjectorNear() const;
    virtual void setProjectorFar(float);
    virtual float getProjectorFar() const;
    virtual void setProjectorApplyMask(bool);
    virtual bool getProjectorApplyMask() const;
    virtual void setProjectorExplicitMask(const Vec3&);       // plain store
    virtual const Vec3& getProjectorExplicitMask() const;
    virtual void setFov(float);                               // cap 180
    virtual float getFov() const;
    virtual void setObjectShadows(bool);
    virtual bool getObjectShadows() const;
    virtual void setShadowIntensity(float);
    virtual float getShadowIntensity() const;
    virtual void setSoftness(float);
    virtual float getSoftness() const;
    virtual void setIntensity(float);
    virtual float getIntensity() const;
    virtual const Vec3& getScale() const;                     // GET before SET in the
    virtual void setScale(const Vec3&);                       // slot order, as attested
    virtual void setFlicker(float);                           // clamped to [0, 1], see cpp
    virtual float getFlicker() const;
    virtual void setEnabled(bool);
    virtual bool getEnabled() const;
    virtual geom::ILightGeometry* getProjectorGeometry() const;   // the +0xC0 cache, raw

    // fields (+0x30..+0xD7; defaults in the header block)
    int             m_refCount;                               // +0x30
    unsigned char   m_isDynamic;                              // +0x34
    unsigned char   m_castsStaticShadow;                      // +0x35
    unsigned char   m_castsDynamicShadow;                     // +0x36
    unsigned char   m_selfShadows;                            // +0x37 (before selfLights!)
    unsigned char   m_selfLights;                             // +0x38
    unsigned char   m_supportsPerVertex;                      // +0x39
    unsigned char   m_supportsPerPixel;                       // +0x3A
    unsigned char   m_supportsGlow;                           // +0x3B
    unsigned char   m_supportsEmitter;                        // +0x3C
    unsigned char   m_affectLightmappedObjects;               // +0x3D
    int             m_lightTypeEnum;                          // +0x40
    std::string     m_lightType;                              // +0x44
    int             m_affectingType;                          // +0x48
    Vec3            m_direction;                              // +0x4C
    float           m_attenuationRange1;                      // +0x58
    float           m_attenuationRange2;                      // +0x5C
    Vec3            m_color;                                  // +0x60
    Vec3            m_specColor;                              // +0x6C
    Vec3            m_scale;                                  // +0x78
    float           m_HDRIntensity;                           // +0x84
    float           m_coneAngle1;                             // +0x88
    float           m_coneAngle2;                             // +0x8C
    float           m_fov;                                    // +0x90
    float           m_flicker;                                // +0x94
    unsigned char   m_enabled;                                // +0x98
    std::string     m_projectorTexture;                       // +0x9C
    float           m_projectorFov;                           // +0xA0
    float           m_projectorAspect;                        // +0xA4
    float           m_projectorNear;                          // +0xA8
    float           m_projectorFar;                           // +0xAC
    unsigned char   m_projectorApplyMask;                     // +0xB0
    Vec3            m_projectorExplicitMask;                  // +0xB4
    geom::ILightGeometry* m_projectorGeometry;                // +0xC0
    unsigned char   m_objectShadows;                          // +0xC4
    float           m_shadowIntensity;                        // +0xC8
    float           m_softness;                               // +0xCC
    float           m_intensity;                              // +0xD0
    std::string     m_geometry;                               // +0xD4 console `geometry` (raw)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
