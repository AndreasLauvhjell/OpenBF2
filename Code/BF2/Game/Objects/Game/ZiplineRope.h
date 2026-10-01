// Path:     [INFERRED] beside Zipline.{h,cpp}; no class-local __FILE__ literal was found.
// Contents: recovered from bf2.i64 and bf2_w32ded.exe.
// ZiplineRope is the visual/physical span owned by a Zipline.  It is a SimpleObject leaf with no
// new virtual slots.  Both ports show exactly two own fields: a refcount and a float returned by
// getBoundingRadiusModifier().  The ELF factory allocates 0x140 bytes (SimpleObject is 0x138),
// while the PE factory allocates 0x158 bytes (SimpleObject is 0x150); the same eight-byte tail is
// therefore independently attested on both ABIs.

#ifndef GAME_OBJECTS_GAME_ZIPLINEROPE_H
#define GAME_OBJECTS_GAME_ZIPLINEROPE_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
namespace world {

class ZiplineRopeTemplate;
class Zipline;

class ZiplineRope : public SimpleObject
{
public:
    explicit ZiplineRope(const ZiplineRopeTemplate* objectTemplate);
    virtual ~ZiplineRope();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void  init();
    virtual void  handleVisualUpdate(float deltaTime, float interpolation);
    virtual void  handleUpdate(float deltaTime, unsigned int flags);
    virtual float getBoundingRadiusModifier() const;

protected:
    friend class Zipline;

    // ELF +0x138/+0x13C; PE +0x150/+0x154.  Both constructors write 1 / 0.0f.
    int   m_refCount;
    float m_boundingRadiusModifier;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ZIPLINEROPE_H
