// World-side decal settings backed by a renderer IDecalTemplate.
// Initialization resolves DecalManager by name; createObject only marks precached and returns null.
// Animation getters/serialization require the renderer handle; other getters have defaults.
// Preserve getSize's non-const signature. Original TU is attested as Game/Objects/Fx/Decal.cpp;
// this header's placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_DECALTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_DECALTEMPLATE_H

#include "../ObjectTemplate.h"
#include "../../../Geom/DecalSystem/IDecalTemplate.h"   // m_decal - the renderer template contract

namespace dice {
namespace hfe {
namespace world {

class DecalTemplate : public ObjectTemplate
{
public:
    DecalTemplate();                                          // read in full

    // The per-class IBase quartet (refcount +0x30).
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                  // delete on 0
    virtual void* queryInterface(unsigned int iid) const;     // IBase/IObjectTemplate/
                                                              // IConsoleSaveable only
    virtual unsigned int getClassID() const;                  // reads the CID global
    virtual ~DecalTemplate();                                 // vptr resets + base only
    virtual void templateInitialize();                        // the DecalManager bind
    virtual bool writeBinary(io::IStream*) const;             // returns false (empty stub)
    virtual bool readBinary(io::IStream*);                    // returns false (empty stub)
    virtual void* queryImplementation(unsigned int classId);  // flat two-CID check
    virtual IObject* createObject() const;                    // flags 0x20 + return NULL
    virtual bool makeScript(io::IStream*);                    // ELF/PE recovered

    // the renderer-template accessors, ELF-named and non-virtual (the Decal console rows of
    // Fx/ModuleWorldObjTemplFx.cpp and makeScript call them). ELF / client PE; [IDecalTemplate slot].
    // unguarded = no m_decal null test in either image (a fault without a renderer template).
    void  setSize(float size);                                // 81F86C0 / 646F10 [+08]
    float getSize();                                          // 81F8700 / 646F40 [+0C] NOT const
    void  setRotation(float rotation);                        // 81F8720 / 646F60 [+48]
    float getRotation() const;                                // 81F8760 / 646F90 [+4C]
    void  setRandomRotation(float randomRotation);            // 81F8780 / 646FB0 [+40]
    float getRandomRotation() const;                          // 81F87C0 / 646FE0 [+44]
    void  setColor(const Vec3& color);                        // 81F87E0 / 647000 [+20]
    Vec3  getColor() const;                                   // 81F8820 / 647460 [+24] zero default
    void  setRandomSize(float randomSize);                    // 81F8870 / 647030 [+10]
    float getRandomSize() const;                              // 81F88B0 / 647060 [+14]
    void  setUsedOnTransparentMaterials(bool used);           // 81F88D0 / 647080 [+30]
    bool  getUsedOnTransparentMaterials() const;              // 81F8910 / 6470B0 [+34] default true
    void  setIsStatic(bool isStatic);                         // 81F8940 / 6470D0 [+28]
    bool  getIsStatic() const;                                // 81F8980 / 647100 [+2C] default true
    void  setDecalTextureName(const std::string& name);       // 81F89B0 / 647120 [+50]
    const std::string& getDecalTextureName() const;           // 81F89F0 / 647150 [+54] g_emptyString
    void  setRandomColorIntensity(float intensity);           // 81F8A20 / 647170 [+18] no refresh
    float getRandomColorIntensity() const;                    // 81F8A50 / 647190 [+1C]
    void  setAnimationEnable(bool enable);                    // 81F8A70 / 6471B0 [+58]
    bool  getAnimationEnable() const;                         // 81F8AB0 / 6471E0 [+5C] unguarded
    void  setAnimationRandomizedStartFrame(bool randomized);  // 81F8AD0 / 6471F0 [+60]
    bool  getAnimationRandomizedStartFrame() const;           // 81F8B10 / 647220 [+64] unguarded
    void  setAnimationSpeed(float speed);                     // 81F8B30 / 647230 [+68]
    float getAnimationSpeed() const;                          // 81F8B70 / 647260 [+6C] unguarded
    void  setAnimationFrameWidth(unsigned int width);         // 81F8B90 / 647270 [+70]
    unsigned int getAnimationFrameWidth() const;              // 81F8BD0 / 6472A0 [+74] unguarded
    void  setAnimationFrameHeight(unsigned int height);       // 81F8BF0 / 6472B0 [+78]
    unsigned int getAnimationFrameHeight() const;             // 81F8CD0 / 647340 [+7C] unguarded
    void  setAnimationFrameWidthRelative(float width);        // 81F8C30 / 6472E0 [+80]
    void  setAnimationFrameHeightRelative(float height);      // 81F8C80 / 647310 [+88]
    void  setAnimationFrameCount(unsigned int count);         // 81F8CF0 / 647350 [+90]
    unsigned int getAnimationFrameCount() const;              // 81F8D40 / 647380 [+94] unguarded
    void  setAnimationFrameCountX(unsigned int count);        // 81F8D60 / 647390 [+98]
    unsigned int getAnimationFrameCountX() const;             // 81F8DB0 / 6473C0 [+9C] unguarded

    int    m_refCount;                                        // +0x30 ctor 1 (client +0x60)
    // +0x34 ctor 0 (client +0x64) - RendDX9's renderer-side template, created (or found) by name by
    // IDecalManager::addTemplate in templateInitialize; the renderer owns it (the ELF destructor
    // does not release it). from IBase* [UNRESOLVED] - IDecalTemplate.h.
    geom::IDecalTemplate* m_decal;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif
