// Commander overhead camera with shared enable state and terrain-following pan controls.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_OVERHEADCAMERA_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_OVERHEADCAMERA_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "../Base/Bundle.h"
#include "../OverheadCameraTemplate.h"
#include "../ICameraObject.h"                     // the real third base

namespace dice {
namespace hfe {
class OverheadCameraNetworkable;
namespace world {

// (CameraViewMode moved to ICameraObject.h - the interface's slots take it.)

// Overhead camera exposing the ICameraObject secondary interface.
class OverheadCamera : public Bundle, public ICameraObject
{
public:
    explicit OverheadCamera(const OverheadCameraTemplate* objectTemplate);  // C1 594 B, read in full
    virtual ~OverheadCamera();                       // container teardown + vptrs

    // own quartet, refcount at +0x140
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// IBase/IObject -> this;
                                                     // ICameraObject -> +0x13C; ConsoleSaveable
                                                     // -> +0x80. note IObject accepted here!

    virtual void init();                             // slot 8 - : Bundle::init,
                                                     // own ICameraObject into the camera map
                                                     // (key 0), then pan(0, 0, 0.001f)
    virtual void destroy(bool force);                // slot 13 - (gated on
                                                     // !isObjectDestroyed())
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
                                                     // slot 64 - (server && root
                                                     // -> handlePlayerInputDefault)
    virtual void initNetworkableComp();              // slot 94 - (no Debug arm)

    // the camera surface: primary slots 96..133, order walked from the ZTV
    virtual const Mat4& getTransformation(float);    // s96 forwards to +0x84
    virtual void setInterpolatedMode(bool);          // s97 empty
    virtual bool setViewMode(CameraViewMode, IObject*, bool);   // s98 (mode, IObject*, bool) per the
                                                                // ICameraObject mangling; was [CHOSEN] (mode, IPlayer*) -
                                                     // `return true`, args ignored
    virtual CameraViewMode getViewMode() const;      // s99 +0x180
    virtual bool isActive() const;                   // s100 +0x15C
    virtual void setIsActive(bool);                  // s101
    virtual void setCameraId(int);                   // s102 +0x160
    virtual int  getCameraId();                      // s103
    virtual void resetCameraView(bool);              // s104 empty
    virtual void updateCameraView(bool);             // s105 vcalls slot 104 (resetCameraView)
    virtual void prevViewMode();                     // s106 empty
    virtual std::map<int, ICameraObject*>* getCameras();     // s107 &m_cameras
    virtual std::map<int, ICameraObject*>* getOwnCameras();  // s108 &m_cameras (same!)
    virtual void nextCamera();                       // s109 empty
    virtual ICameraObject* getActiveCamera();        // s110 the +0x13C sub-object
    virtual void setActiveCamera(int);               // s111 empty
    virtual void setTraceObjectId(unsigned int);     // s112 +0x1DC
    virtual unsigned int getTraceObjectId();         // s113
    virtual void setExternCameraTrans(Mat4);         // s114 empty (BY value, per mangling)
    virtual Mat4* getExternCameraTrans();            // s115 returns 0 [CHOSEN ret type]
    virtual void setTeam(int);                       // s116 writes m_defaultTeam - the
                                                     // setDefaultTeam alias (spawnDelay again)
    virtual int  getTeam();                          // s117 reads m_defaultTeam
    virtual void setDefaultTeam(int);                // s118 same store
    virtual int  getDefaultTeam();                   // s119
    virtual bool getToggleMouseLook();               // s120 false
    virtual void setMenuVisibleForTeam(unsigned char);   // s121 empty
    virtual unsigned char getMenuVisibleForTeam();   // s122 0
    virtual int  getPcoId();                         // s123 0
    virtual void handleExplosion(float);             // s124 empty
    virtual float getShakeFactor() const;            // s125 0
    virtual bool getIsUnderwater() const;            // s126 false
    virtual float getNormalizedUnderwaterTime() const;   // s127 0
    virtual float getActiveFOV() const;              // s128 template FOV, 2.3f if orphaned
    virtual const Vec3& getStaticSunDirection() const;   // s129 Vec3::getIdentity
    virtual const Vec3& getStaticSunColor() const;       // s130
    virtual const Vec3& getStaticSunDamageColor() const; // s131
    virtual const Vec3& getStaticSunAmbientColor() const;// s132
    virtual bool isOverheadCamera() const;           // s133 true (the one class where it is)

    // non-virtual
    void pan(float dx, float dz, float dt);          // the terrain-follow. See
                                                     // the .cpp; needs heightmapCluster live to
                                                     // go past its gate.
    float getMaxSpeed() const;                       // m_speedScale * template maxSpeed
    bool requestNewPosition(const Vec3&);            // server force-set / client
                                                     // CommanderCamEvent send
    bool forceSetPosition(const Vec3&);              // (server gate, teleport
                                                     // root, pan(0,0,-1) reset)
    void handlePlayerInputDefault(IPlayer*, const io::PlayerInput&, float, unsigned int);
                                                     // yaw/throttle acceleration + pan

    static bool m_enabled;                           // initial true in ELF/PE .data; sole code writer:
                                                     // OverheadCameraNetworkable::setNetUpdate

private:
    // The shipped networkable snapshots the pan integrators/default team directly, and applies
    // replicated default-team updates as a direct store. Keeping that source-level relationship
    // typed is essential: these fields sit at different offsets in the ELF and PE layouts.
    friend class ::dice::hfe::OverheadCameraNetworkable;

    // layout +0x13C..+0x20F, from the 594-byte C1 ctor. [CHOSEN] names unless noted.
    // (+0x13C = the ICameraObject vptr - a real base now, the compiler places it)
    /* 0x140 */ int   m_refCount;                    // ctor 1
    /* 0x144 */ std::vector<int> _vec144;            // [UNRESOLVED element/role] ctor empty
    /* 0x150 */ std::list<std::string> _list150;     // [UNRESOLVED role] ctor empty
    /* 0x154 */ std::list<std::string> _list154;     // [UNRESOLVED role] ctor empty
    /* 0x158 */ std::string _str158;                 // [UNRESOLVED role] ctor empty
    /* 0x15C */ bool  m_isActive;                    // ctor false
    /* 0x160 */ int   m_cameraId;                    // ctor 0
    /* 0x164 */ std::map<int, ICameraObject*> m_cameras;  // ctor empty; init inserts {0, self}
    /* 0x174 */ std::vector<int> _vec174;            // [UNRESOLVED role] ctor empty
    /* 0x180 */ int   m_viewMode;                    // ctor 3; getViewMode reads this exact field
    /* 0x184 */ float _float184;                     // ctor -1.0f [UNRESOLVED role]
    /* 0x188 */ bool  _bool188;                      // ctor true [UNRESOLVED role]
    /* 0x18C */ int   _dword18C;                     // ctor 0
    /* 0x190 */ float _float190[2];                  // ctor 0,0
    /* 0x198 */ int   _dword198;                     // ctor -1
    /* 0x19C */ Mat4  m_transformation;              // ctor identity (diagonal writes at
                                                     // +0x19C/+0x1B0/+0x1C4/+0x1D8)
    /* 0x1DC */ unsigned int m_traceObjectId;        // ctor -1
    /* 0x1E0 */ bool  _bool1E0;                      // ctor true
    /* 0x1E4 */ int   _dword1E4;                     // ctor 0
    /* 0x1E8 */ int   _dword1E8;                     // ctor 0
    /* 0x1EC */ int   _dword1EC;                     // ctor 0
    /* 0x1F0 */ float _float1F0;                     // ctor 1.0f
    /* 0x1F4 */ bool  _bool1F4;                      // ctor false
    /* 0x1F8 */ float m_speedScale;                  // ctor 1.0f - getMaxSpeed's factor
    /* 0x1FC */ float m_panState[3];                 // ctor 0 (+0x1FC..+0x207) - pan's dt<0 reset
                                                     // zeroes exactly these three
    /* 0x208 */ std::string _str208;                 // [UNRESOLVED role] ctor empty
    /* 0x20C */ int   m_defaultTeam;                 // ctor 0 - setTeam AND setDefaultTeam write it
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_OVERHEADCAMERA_H
