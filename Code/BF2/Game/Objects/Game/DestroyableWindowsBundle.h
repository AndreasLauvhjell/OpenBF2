// Path: [INFERRED], beside the recovered gameplay runtime objects.
#ifndef GAME_OBJECTS_GAME_DESTROYABLEWINDOWSBUNDLE_H
#define GAME_OBJECTS_GAME_DESTROYABLEWINDOWSBUNDLE_H

#include "../Base/Bundle.h"

namespace dice { namespace hfe {
class DestroyableWindowsBundleNetworkable;
namespace world {
class DestroyableWindowTemplate;
class DestroyableWindowsBundleTemplate;

// ELF 0x140 / PE 0x158. No additional virtual slots.
class DestroyableWindow : public SimpleObject
{
public:
    explicit DestroyableWindow(const DestroyableWindowTemplate*);
    virtual ~DestroyableWindow();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual void initNetworkableComp();
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
    void setIsBroken(bool);
    bool getIsBroken() const;
private:
    int m_refCount;  // ELF +0x138 / PE +0x150
    bool m_isBroken; // ELF +0x13C / PE +0x154
};

// ELF 0x148 / PE 0x160. State bits address geometry parts, not child IDs.
class DestroyableWindowsBundle : public Bundle
{
public:
    explicit DestroyableWindowsBundle(const DestroyableWindowsBundleTemplate*);
    virtual ~DestroyableWindowsBundle();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual void init();
    virtual void initNetworkableComp();
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    void updateStates(unsigned int);
    unsigned int getNumStates() const; // [INFERRED] unsigned: consumed as BitStream's bit count
    void handleChildCollision(DestroyableWindow*);
private:
    friend class ::dice::hfe::DestroyableWindowsBundleNetworkable;
    int m_refCount;                // ELF +0x13C / PE +0x154
    unsigned int m_states;         // ELF +0x140 / PE +0x158, ctor ~0u
    unsigned int m_previousStates; // ELF +0x144 / PE +0x15C, ctor 0
};
} } }
#endif
