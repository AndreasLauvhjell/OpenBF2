// Vehicle engine sound state and pitch/volume updates.


#ifndef SOUND_ENGINESOUND_H     // [INFERRED] guard style
#define SOUND_ENGINESOUND_H

#include "ISoundNode.h"
#include "ISoundEvaluator.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

class ISound;
class EngineSoundTemplate;
namespace world { class EngineTemplate; class EngineBase; class IObject; }

class EngineSound : public ISoundNode
{
public:
    EngineSound(const world::EngineTemplate* engineTemplate, world::EngineBase* engine,
                world::IObject* root, const EngineSoundTemplate* soundTemplate,
                const float* rpmSource);
    virtual const Mat4& getTransform();
    virtual const Math::Vec3* getVelocity();
    virtual ~EngineSound();

    void  setIsRunning(bool running, bool forceStop, bool notLocal);
    void  setVelocity(const Vec3& velocity);
    float update(bool engineOn, float input, int material);
    float updateAudioRpm(bool engineOn, float input, int material);
    float updateSwayEffect(bool engineOn);
    void  resetAudioRpm();
    void  playGearShift();
    static void initTemplates(world::EngineTemplate* engineTemplate);

private:
    /* 0x04 */ const EngineSoundTemplate* m_template;
    /* 0x08 */ ISound*        m_sounds[4];
    /* 0x18 */ const float*   m_rpmSource;
    /* 0x1C */ float          m_rpm;
    /* 0x20 */ float          m_values[4];
    /* 0x30 */ ScaleEvaluator m_evaluators[4];
    /* 0x60 */ Vec3           m_velocity;
    /* 0x6C */ world::EngineBase* m_engine;
    /* 0x70 */ world::IObject* m_root;
    /* 0x74 */ bool           m_newStyleAudio;
    /* 0x78 */ float          m_load;
    /* 0x7C */ int            m_noiseCountdown;  // signed tick countdown; decremented even when stopped
    /* 0x80 */ float          m_noiseRpm;
    /* 0x84 */ float          m_audioRpm;
    /* 0x88 */ float          m_float88;         // [UNRESOLVED] reset to zero, no other use in this TU
    /* 0x8C */ float          m_swayTime;
    /* 0x90 */ int            m_lastGear;
    /* 0x94 */ float          m_audioVelocity;
};

} // namespace hfe
} // namespace dice

#endif // SOUND_ENGINESOUND_H
