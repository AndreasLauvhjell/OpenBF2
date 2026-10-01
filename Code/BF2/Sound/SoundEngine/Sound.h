
// Audio DLL implementations of the host sound and template interfaces.


#ifndef SOUND_SOUNDENGINE_SOUND_H
#define SOUND_SOUNDENGINE_SOUND_H

#include "../NullSound.h"
#include "../ISoundNode.h"
#include "../ISoundEvaluator.h"
#include "../ISoundObstructionEvaluator.h"
#include "../../Math/Mat4.h"

#include <vector>

namespace dice {
namespace hfe {

class Sample;
class SoundFile;
class SoundEvaluator;
struct EnvelopePoint;

// Engine sound template; x86 layout is 156 bytes.
class SoundTemplate : public ISoundTemplate
{
public:
    SoundTemplate();
    virtual ~SoundTemplate();

    virtual void  addRef();
    virtual void  release();
    virtual void  setFilename(const char* name);
    virtual const char* getFilename() const;
    virtual void  setFilenames(const char** names, int count);
    virtual const char** getFilenames(int* count) const;
    virtual void  setLoopCount(unsigned int n) { m_loopCount = (int) n; }
    virtual unsigned int getLoopCount() const { return (unsigned int) m_loopCount; }
    virtual void  setVolume(float v);
    virtual float getVolume() const { return m_volume; }
    virtual void  setVolumeEnvelope(float xMin, float xMax, float yMin, float yMax, bool randomize,
                                    unsigned int count, const SoundEnvelopePoint* points);
    virtual const SoundEnvelopePoint* getVolumeEnvelope(float* xMin, float* xMax, float* yMin, float* yMax,
                                                        bool* randomize, unsigned int* count);
    virtual void* getVolumeEvaluator() const { return m_volumeEvaluator; }
    virtual void  setPitch(float p);
    virtual float getPitch() const { return m_pitch; }
    virtual void  setPitchEnvelope(float xMin, float xMax, float yMin, float yMax, bool randomize,
                                   unsigned int count, const SoundEnvelopePoint* points);
    virtual const SoundEnvelopePoint* getPitchEnvelope(float* xMin, float* xMax, float* yMin, float* yMax,
                                                       bool* randomize, unsigned int* count);
    virtual void* getPitchEvaluator() const { return m_pitchEvaluator; }
    virtual void  setVolumeGroup(int g);
    virtual int   getVolumeGroup() const { return m_volumeGroup; }
    virtual void  setPan(float p);
    virtual float getPan() const { return m_pan; }
    virtual void  setMinDistance(float d);
    virtual float getMinDistance() const { return m_minDistance; }
    virtual void  setHalfVolumeDistance(float d);
    virtual float getHalfVolumeDistance() const { return m_halfVolumeDistance; }
    virtual void  setIs3dSound(bool b) { m_is3d = b; }
    virtual bool  getIs3dSound() const { return m_is3d; }
    virtual void  setStopType(int t);
    virtual int   getStopType() const { return m_stopType; }
    virtual void  setReverbLevel(float r);
    virtual float getReverbLevel() const { return m_reverbLevel; }
    virtual void  setPosition(const Vec3& p);
    virtual const Vec3& getPosition() const { return m_position; }
    virtual void  setDirection(const Vec3& d);
    virtual const Vec3& getDirection() const { return m_direction; }
    virtual void  setRadius(float r);
    virtual float getRadius() const { return m_radius; }
    virtual void  setConeInnerAngle(float a);
    virtual float getConeInnerAngle() const { return m_coneInnerAngle; }
    virtual void  setConeOuterAngle(float a);
    virtual float getConeOuterAngle() const { return m_coneOuterAngle; }
    virtual void  setConeOuterVolume(float v);
    virtual float getConeOuterVolume() const { return m_coneOuterVolume; }
    virtual void  setGlobalEffectLevel(float l);
    virtual float getGlobalEffectLevel() const { return m_globalEffectLevel; }
    virtual void  setIsUsed();
    virtual void  setMaxSamples(SoundQuality q, int n);
    virtual int   getMaxSamples(SoundQuality q) const { return m_maxSamples[q]; }
    virtual void  setFlags(unsigned int f) { m_flags = f; }
    virtual unsigned int getFlags() const { return m_flags; }
    virtual bool  checkSampleAvailable(int index) const;
    virtual ISound* createSound();

    // Non-virtual helpers the engine and the samples use.
    SoundFile* getSoundFile(int index) const;       // null outside [0, sampleCount)
    int        getLengthFrames(int index) const;    // loops * samples at pitch, or INT_MAX
    int        getSampleCount() const;              // min(maxSamples[quality], files)
    bool       hasCone() const;                     // inner angle below 360
    void       recomputeDistances();                // rolloff and cut-off from the distances
    void       clearFiles();

    int                       m_refCount;           // +0x04
    int                       m_loopCount;          // +0x08 = 1
    float                     m_minDistance;        // +0x0C = 1
    float                     m_halfVolumeDistance; // +0x10 = 0
    float                     m_maxDistance;        // +0x14 computed
    float                     m_rolloff;            // +0x18 computed
    bool                      m_is3d;               // +0x1C = true
    int                       m_stopType;           // +0x20 = 1
    float                     m_reverbLevel;        // +0x24 = 1
    float                     m_volume;             // +0x28 = 1
    float                     m_pitch;              // +0x2C = 1
    float                     m_pan;                // +0x30 = 0.5
    SoundEvaluator*           m_volumeEvaluator;    // +0x34
    SoundEvaluator*           m_pitchEvaluator;     // +0x38
    Vec3                      m_position;           // +0x3C
    float                     m_radius;             // +0x48
    Vec3                      m_direction;          // +0x4C
    float                     m_coneInnerAngle;     // +0x58 = 360
    float                     m_coneOuterAngle;     // +0x5C = 360
    float                     m_coneOuterVolume;    // +0x60 = 1
    float                     m_globalEffectLevel;  // +0x64 = 1
    std::vector<SoundFile*>   m_files;              // +0x68
    std::vector<const char*>  m_fileNames;          // +0x78
    unsigned int              m_flags;              // +0x88
    int                       m_volumeGroup;        // +0x8C
    int                       m_maxSamples[3];      // +0x90 = INT_MAX each
};

// 0x58 bytes: the node a released-but-sounding Sound plays from. Two tables: the obstruction
// evaluator's one slot at +0x00 (answers the frozen value) and ISoundNode's two at +0x04.
class SoundNodeSnapshot : public ISoundObstructionEvaluator, public ISoundNode
{
public:
    SoundNodeSnapshot();
    virtual float evaluate(ISound*, const Math::Vec3*) { return m_obstruction; }   // [INFERRED body]
    virtual const Mat4& getTransform() { return m_matrix; }
    virtual const Math::Vec3* getVelocity() { return &m_velocity; }

    Mat4  m_matrix;         // +0x08 identity until filled
    Vec3  m_velocity;       // +0x48
    float m_obstruction;    // +0x54
};

// Default sound parent: identity transform and zero velocity, constructed before main.
extern SoundNodeSnapshot g_nullSoundNode;

// Engine sound instance; x86 layout is 104 bytes.
class Sound : public ISound
{
public:
    explicit Sound(SoundTemplate* t);
    virtual ~Sound();

    virtual int   addRef();
    virtual int   release();
    virtual void  setSoundFlags(unsigned int f) { m_soundFlags = f; }
    virtual unsigned int getSoundFlags() const { return m_soundFlags; }
    virtual void  play();
    virtual void  play(int loops);
    virtual void  playAt(const Vec3& pos);
    virtual void  playAt(const Vec3& pos, int loops);
    virtual int   playSample(int index);
    virtual int   playSample(int index, int loops);
    virtual int   playSampleAt(const Vec3& pos, int index);
    virtual int   playSampleAt(const Vec3& pos, int index, int loops);
    virtual void  stop();
    virtual void  forceStop();
    virtual bool  isPlaying() { return m_playingCount > 0; }
    virtual void  setParent(ISoundNode* n);
    virtual ISoundNode* getParent() const { return m_parent; }
    virtual void  setVolumeEvaluator(ISoundEvaluator* e, const float* param);
    virtual ISoundEvaluator* getVolumeEvaluator() const;
    virtual void  setPitchEvaluator(ISoundEvaluator* e, const float* param);
    virtual ISoundEvaluator* getPitchEvaluator() const;
    virtual void  setObstructionEvaluator(ISoundObstructionEvaluator* e) { m_obstructionEvaluator = e; }
    virtual ISoundObstructionEvaluator* getObstructionEvaluator() const { return m_obstructionEvaluator; }
    virtual void  setIs1PSound(bool b) { m_is1P = b; }
    virtual bool  getIs1PSound() const { return m_is1P; }
    virtual void  setUserData(int i, void* p) { m_userData[i] = p; }
    virtual void* getUserData(int i) const { return m_userData[i]; }
    virtual void* getSampleInfo(int index) const;
    virtual ISoundTemplate* getTemplate() const { return m_template; }

    // Non-virtual helpers.
    SoundTemplate* getTemplateImpl() const { return m_template; }           // (21 callers)
    const Mat4*    getParentMatrix() const;
    const Vec3*    getParentVelocity() const;
    void           addPlaying() { ++m_playingCount; }
    int            getLoopOverride() const { return m_loopOverride; }
    float          computeVolume(float x, int sample);
    float          computePitch(float x, int sample);
    void           detachSample(Sample* s);
    Vec3*          computePosition(Vec3* out, const Vec3* offset);
    int            chooseSlot(const Vec3* position);
    int            randomIndex();
    bool           attachSample(Sample* s);
    bool           detachToSnapshot();

    int                          m_refCount;            // +0x04
    int                          m_playingCount;        // +0x08 live samples
    SoundTemplate*               m_template;            // +0x0C
    ISoundNode*                  m_parent;              // +0x10 never null (the null node)
    ISoundEvaluator*             m_volumeEvaluator;     // +0x14
    const float*                 m_volumeParam;         // +0x18
    ISoundEvaluator*             m_pitchEvaluator;      // +0x1C
    const float*                 m_pitchParam;          // +0x20
    ISoundObstructionEvaluator*  m_obstructionEvaluator;// +0x24
    int                          m_loopOverride;        // +0x28 the loop count the last play* gave
    Sample*                      m_samples[2];          // +0x2C the retrigger pair
    float                        m_slotDistance[2];     // +0x34
    int                          m_slotFlags[2];        // +0x3C bit 0 right, bit 1 front
    int                          m_lastAttachFrame;     // +0x44 = -1
    int                          m_slotIndex;           // +0x48
    void*                        m_userData[3];         // +0x4C
    SoundNodeSnapshot*           m_snapshot;            // +0x58
    unsigned int                 m_soundFlags;          // +0x5C
    int                          m_lastRandomIndex;     // +0x60 = -1
    bool                         m_is1P;                // +0x64
};

// ds:live Sound objects. The stats block reports it.
extern int g_soundCount;

} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUND_H
