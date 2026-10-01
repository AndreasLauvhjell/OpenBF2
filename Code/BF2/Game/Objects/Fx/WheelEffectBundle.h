// SimpleObject wheel/track effect bundle. Only the client factory creates it.
// Field offsets describe retail x86; native STL and pointers follow their compiler layout.
// Header placement is [INFERRED].
#ifndef GAME_OBJECTS_FX_WHEELEFFECTBUNDLE_H
#define GAME_OBJECTS_FX_WHEELEFFECTBUNDLE_H
#include "../Base/SimpleObject.h"
#include "../../../Math/Plane3.h"
#include <map>

namespace dice { namespace hfe { namespace world {
class WheelEffectBundleTemplate;
class EffectBundle;

class WheelEffectBundle : public SimpleObject
{
public:
    explicit WheelEffectBundle(const WheelEffectBundleTemplate* objectTemplate);
    virtual ~WheelEffectBundle();
    virtual void init();
    virtual void disableCachedObject();
    virtual void enableCachedObject();
    virtual void destroy(bool force);
    virtual void handleVisualUpdate(float deltaTime, float interpolation);
    virtual void handleUpdate(float deltaTime, unsigned int flags);
    virtual void handleMessage(TemplateMessage message, IPlayer* player, bool recursive);
    virtual int getMaterial() const;
    // The only two added primary slots: ELF +180/+184; PE +178/+17C.
    virtual void setCollisionPlane(const Plane3& plane);
    virtual void setRotationMatrixPtr(const Mat4* matrix);

    void setCurrentMaterial(int material, const Vec3& contactOffset, bool recursive);
    void updateCurrentState();
    void updateEffectProperties();
    void stopEffect();

    // [CHOSEN] access expression for a direct field read from another TU:
    // Physics/PhysicsUtil.cpp's getWheelMaterials returns m_material itself - ELF 0x8361FF2
    // `mov eax, [ebx+144h]`, BF2.exe 0x6E93D1 `mov eax, [edi+15Ch]`, both this member.
    friend int getWheelMaterials(const WeakPtr<IObject>& wheel, int* outSurface);

protected:
    std::map<int, EffectBundle*> m_effects; // ELF +138 / PE +150; destroy owns values, dtor only frees map.
    int m_material;                       // +144 / +15C, ctor -1.
    unsigned int m_unknown;               // +148 / +160: [UNRESOLVED], never initialized or read here.
    Vec3 m_contactOffset;                 // +14C / +164, ctor zero.
    Mat4 m_matrix;                        // +158 / +170: identity, otherwise unused in dedicated methods.
    const Mat4* m_rotationMatrix;         // +198 / +1B0, borrowed (Spring parent's transformation).
    EffectBundle* m_currentEffect;        // +19C / +1B4, alias into m_effects.
    bool m_playing;                       // +1A0 / +1B8, ctor false.
    unsigned int m_expiryFrame;           // +1A4 / +1BC, ctor zero; unsigned compare at ELF8215767.
};
} } }
#endif
