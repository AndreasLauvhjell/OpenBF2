// Editable-road settings and writer for compiled road .con/.dat files.
// [INFERRED] header placement. Indexed setters do not check bounds: use texture layers 0/1
// and LOD 0. The integer at +0x5C is [UNRESOLVED]; the float at +0x60 is [UNRESOLVED].

#ifdef BF2_IMAGE_RENDDX9

#ifndef BF2_GEOM_ROADTEMPLATE_H
#define BF2_GEOM_ROADTEMPLATE_H

#include "UndergrowthSystem.h"        // Vc71Vec<T>
#include "Vc71BoolVector.h"
#include "../Math/Vec3.h"             // Vec2
#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/PluginAbi.h"   // renddx9abi::HostString

#include <stddef.h>

namespace dice {
namespace hfe {
namespace geom {


// IRoadTemplate (IID 820008) - the 25 slots after the IBase quartet, table 0x1021D610 in order.
// Every string crosses the host boundary (the retail LevelEditor, and this tree's editor road
//   module, call these slots from the other image), so every string is renddx9abi::HostString.
// The names follow the plugin's rows where one exists (census §2.2); the rest are [INFERRED].

class IRoadTemplate : public IBase
{
public:
    virtual void setTextureFile(int index, const ::renddx9abi::HostString& file) = 0;   // +0x10
    virtual const ::renddx9abi::HostString& getTextureFile(int index) = 0;            // +0x14
    virtual void setTextureScale(int index, const Vec2& scale) = 0;                     // +0x18
    virtual const Vec2& getTextureScale(int index) = 0;                                 // +0x1C
    virtual void setMirror(unsigned int index, bool mirror) = 0;                        // +0x20
    virtual bool getMirror(unsigned int index) = 0;                                     // +0x24
    virtual void setWidth(float width) = 0;                                             // +0x28
    virtual float getWidth() = 0;                                                       // +0x2C
    virtual void setMaxTesselatedError(int lod, float error) = 0;                       // +0x30
    virtual float getMaxTesselatedError(int lod) = 0;                                   // +0x34
    virtual void setBlendFactor(float factor) = 0;                                      // +0x38
    virtual float getBlendFactor() = 0;                                                 // +0x3C
    virtual void setFadeOffDistance(const Vec2& distance) = 0;                          // +0x40
    virtual const Vec2& getFadeOffDistance() = 0;                                       // +0x44
    virtual void setPriority(int priority) = 0;                                         // +0x48
    virtual int getPriority() = 0;                                                      // +0x4C
    virtual void setMaterial(int material) = 0;                                         // +0x50
    virtual int getMaterial() = 0;                                                      // +0x54
    virtual void setName(const ::renddx9abi::HostString& name) = 0;                     // +0x58
    virtual ::renddx9abi::HostString getName() = 0;                                     // +0x5C
    virtual bool compile(const ::renddx9abi::HostString& path) = 0;                     // +0x60
    virtual void setUnknown5C(int value) = 0;                                           // +0x64
    virtual int getUnknown5C() = 0;                                                     // +0x68
    virtual void setUnknown60(float value) = 0;                                         // +0x6C
    virtual float getUnknown60() = 0;                                                   // +0x70
};

class RoadTemplate : public IRoadTemplate
{
public:
    RoadTemplate();
    ~RoadTemplate();

    // IBase
    virtual void  addRef();                                                     // +0x00
    virtual int   getRef();                                                     // +0x04
    virtual int   release();                                                    // +0x08
    virtual void* queryInterface(unsigned int iid) const;                       // +0x0C

    // IRoadTemplate
    virtual void setTextureFile(int index, const ::renddx9abi::HostString& file);
    virtual const ::renddx9abi::HostString& getTextureFile(int index);
    virtual void setTextureScale(int index, const Vec2& scale);
    virtual const Vec2& getTextureScale(int index);
    virtual void setMirror(unsigned int index, bool mirror);
    virtual bool getMirror(unsigned int index);
    virtual void setWidth(float width);
    virtual float getWidth();
    virtual void setMaxTesselatedError(int lod, float error);
    virtual float getMaxTesselatedError(int lod);
    virtual void setBlendFactor(float factor);
    virtual float getBlendFactor();
    virtual void setFadeOffDistance(const Vec2& distance);
    virtual const Vec2& getFadeOffDistance();
    virtual void setPriority(int priority);                                         // (ICF)
    virtual int getPriority();                                                      // (ICF)
    virtual void setMaterial(int material);                                         // (ICF)
    virtual int getMaterial();                                                      // (ICF)
    virtual void setName(const ::renddx9abi::HostString& name);
    virtual ::renddx9abi::HostString getName();
    virtual bool compile(const ::renddx9abi::HostString& path);
    virtual void setUnknown5C(int value);
    virtual int getUnknown5C();
    virtual void setUnknown60(float value);
    virtual float getUnknown60();

    // data, in shipped order
    int                                   m_refCount;              // +0x04
    Vc71Vec< ::renddx9abi::HostString>    m_textureFiles;          // +0x08
    Vc71Vec<Vec2>                         m_textureScales;         // +0x18
    Vc71BoolVector                        m_mirrors;               // +0x28
    Vc71Vec<float>                        m_maxTesselatedErrors;   // +0x3C
    float                                 m_blendFactor;           // +0x4C
    Vec2                                  m_fadeOffDistance;       // +0x50
    float                                 m_width;                 // +0x58
    int                                   m_unknown5C;             // +0x5C
    float                                 m_unknown60;             // +0x60
    int                                   m_material;              // +0x64
    ::renddx9abi::HostString              m_name;                  // +0x68
    int                                   m_priority;              // +0x84

private:
    RoadTemplate(const RoadTemplate&);
    RoadTemplate& operator=(const RoadTemplate&);
};

// The retail flavour's layout is the shipped one. In the native flavour HostString is the host's
// std::string, so the name (and the texture-file elements) take that type's size instead - the
// object never crosses the boundary by layout, only through its slots.
#if defined(_M_IX86) && !BF2_PLUGIN_ABI_NATIVE
static_assert(offsetof(RoadTemplate, m_refCount) == 0x04, "refCount at +0x04");
static_assert(offsetof(RoadTemplate, m_textureFiles) == 0x08, "texture files at +0x08");
static_assert(offsetof(RoadTemplate, m_textureScales) == 0x18, "texture scales at +0x18");
static_assert(offsetof(RoadTemplate, m_mirrors) == 0x28, "mirror flags at +0x28");
static_assert(offsetof(RoadTemplate, m_maxTesselatedErrors) == 0x3C, "max errors at +0x3C");
static_assert(offsetof(RoadTemplate, m_blendFactor) == 0x4C, "blend factor at +0x4C");
static_assert(offsetof(RoadTemplate, m_fadeOffDistance) == 0x50, "fade-off at +0x50");
static_assert(offsetof(RoadTemplate, m_width) == 0x58, "width at +0x58");
static_assert(offsetof(RoadTemplate, m_material) == 0x64, "material at +0x64");
static_assert(offsetof(RoadTemplate, m_name) == 0x68, "name at +0x68");
static_assert(offsetof(RoadTemplate, m_priority) == 0x84, "priority at +0x84");
static_assert(sizeof(RoadTemplate) == 0x88, "the factory 0x100B099A allocates 0x88");
#endif

// RefFactory published by ModuleGeom::getClassList.
IBase* __fastcall createRoadTemplate(unsigned int iid, IBase* outer);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_ROADTEMPLATE_H
#endif // BF2_IMAGE_RENDDX9
