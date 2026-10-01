// Owns editor camera position and yaw/pitch in free, top-down, orbit and terrain-following modes.
// Writes a camera-to-world matrix to IRenderView: right/up/forward/position rows, Y up.
// Consumes look and movement input only while the 3D view owns input; orbit uses a centre and
// terrain following preserves ground clearance.
#ifndef BF2EDITOR_APP_FREECAMERA_H
#define BF2EDITOR_APP_FREECAMERA_H

#include "Framework/EditorConfig.h"

namespace dice { namespace hfe { namespace dx9rend { class IRenderView; } } }

namespace bf2editor
{

enum class CameraMode
{
    Free = 0,
    TopDown = 1,
    Orbit = 2,
    FollowTerrain = 3
};

class FreeCamera
{
public:
    FreeCamera();

    // The terrain height under (x, z); false where there is none (FollowTerrain keeps its height then).
    typedef bool (*GroundQuery)(void* context, float x, float z, float& height);
    void setGroundQuery(GroundQuery query, void* context);

    // `centre` is the orbit's centre (Orbit only).
    void setMode(CameraMode mode, const float centre[3]);
    CameraMode mode() const { return m_mode; }
    static const char* modeName(CameraMode mode);

    void place(float x, float y, float z, float yawDegrees, float pitchDegrees);
    // Options > Mouse look sensitivity (the retail host's Environment.MouseSensitivity): a factor on the look
    // speed, 0.1 to 5.
    void setLookSensitivity(float factor) { m_lookFactor = factor < 0.1f ? 0.1f : (factor > 5.0f ? 5.0f : factor); }
    float lookSensitivity() const { return m_lookFactor; }

    // Reads this frame's ImGui input (between ImGuiHost::beginFrame and endFrame) and moves.
    void update(float seconds);

    // Writes the view: camera matrix, field of view, aspect (height / width), far plane, viewport.
    void apply(dice::hfe::dx9rend::IRenderView* view, int width, int height) const;

    // Row-vector matrices in the engine's layout (translation in elements 12..14). cameraToWorld is
    // what apply() hands setViewMatrix; worldToView is its inverse (the camera basis is orthonormal).
    void cameraToWorld(float out[16]) const;
    void worldToView(float out[16]) const;

    // The ray through window pixel (x, y) of a width x height view, from the camera's position; the
    // direction is unit length. Matches the renderer's projection: RenderView::rebuildProjection
    // scales view y by cot(fov / 2) and view x by cot(fov / 2) * height / width.
    void rayThroughPixel(float x, float y, int width, int height, float origin[3], float direction[3]) const;
    // The inverse: the window pixel a world point projects to; false when it lies behind the camera.
    bool worldToPixel(const float point[3], int width, int height, float& x, float& y) const;

    // Backs the camera away from `centre`, keeping its heading, until a sphere of `radius` fills
    // most of the view.
    void frame(const float centre[3], float radius);

    // True while the right button looks around (the keyboard then belongs to the camera).
    bool isLooking() const { return m_looking; }
    // The wheel turned with Shift or Ctrl held is left to a tool (the terrain brush's strength and
    // size) while this is set.
    void setModifiedWheelReserved(bool reserved) { m_wheelReserved = reserved; }

    float x() const { return m_position[0]; }
    float y() const { return m_position[1]; }
    float z() const { return m_position[2]; }
    float yawDegrees() const;
    float pitchDegrees() const;
    float speed() const { return m_speed; }
    void setSpeed(float metresPerSecond);
    static float defaultSpeed() { return 40.0f; }
    float farPlane() const { return m_farPlane; }
    // The view distance (Tools > Options), kept within 100..20000 m.
    void setFarPlane(float metres) { m_farPlane = metres < 100.0f ? 100.0f : (metres > 20000.0f ? 20000.0f : metres); }

private:
    void basis(float right[3], float up[3], float forward[3]) const;
    void lookAt(const float target[3]);
    void keepAboveTerrain();

    float m_position[3];
    float m_yaw;            // radians about +Y; 0 looks along +Z
    float m_pitch;          // radians; positive looks up
    float m_speed;          // metres per second
    float m_fov;            // radians
    float m_farPlane;       // metres
    bool  m_looking;        // the right button went down over the 3D view
    bool  m_wheelReserved;  // setModifiedWheelReserved
    CameraMode  m_mode;
    float       m_centre[3];    // Orbit
    float       m_distance;     // Orbit
    float       m_followHeight; // FollowTerrain: metres above the terrain
    GroundQuery m_ground;
    void*       m_groundContext;
    float       m_lookFactor = 1.0f;    // Options > Mouse look sensitivity
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_FREECAMERA_H
