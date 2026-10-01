// Vehicle wheel sound state.


#ifndef SOUND_WHEELSOUND_H     // [INFERRED] guard style
#define SOUND_WHEELSOUND_H

namespace dice {
namespace hfe {

class ISound;
class ObjectSoundNodeFixed;
namespace world { class SpringTemplate; class Spring; }

class WheelSound
{
public:
    WheelSound(const world::SpringTemplate* springTemplate, world::Spring* spring);
    ~WheelSound();

    void update(float);
    static void initTemplates(world::SpringTemplate* springTemplate, unsigned int);

private:
    /* 0x00 */ ISound*              m_sound;
    /* 0x04 */ world::Spring*       m_spring;
    /* 0x08 */ ObjectSoundNodeFixed* m_node;
    /* 0x0C */ float                m_value;
};

} // namespace hfe
} // namespace dice

#endif // SOUND_WHEELSOUND_H
