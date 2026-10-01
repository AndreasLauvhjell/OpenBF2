// Path:     [INFERRED] beside the attested Game/Objects/Base/Bundle.cpp.
// Contents: runtime Bundle recovered from both dedicated-server ports.

#ifndef GAME_OBJECTS_BASE_BUNDLE_H
#define GAME_OBJECTS_BASE_BUNDLE_H

#include "SimpleObject.h"

namespace dice {
namespace hfe {
namespace world {

class BundleTemplate;

class Bundle : public SimpleObject
{
public:
    explicit Bundle(const BundleTemplate* objectTemplate);
    virtual ~Bundle();

    virtual void init();
    virtual void enableCachedObject();
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    virtual void setInputId(char);

    const Mat4* getBundleTransformation() const;

private:
    const Mat4* m_bundleTransformation; // ELF +0x138 / PE +0x150, non-owning
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_BASE_BUNDLE_H
