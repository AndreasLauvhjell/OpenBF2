// Code/Tools/BF2Editor/App/FreeCamera.cpp - see FreeCamera.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/FreeCamera.h"

#include "App/ImGuiHost.h"
#include "App/ShortcutKeys.h"

#include "RendMisc/IRenderView.h"

#include "imgui.h"

#include <cmath>

namespace bf2editor
{

namespace
{
const float kPi = 3.14159265358979f;
const float kDegrees = 180.0f / kPi;
const float kLookSensitivity = 0.004f;      // radians per pixel
const float kMaxPitch = 89.0f / 180.0f * kPi;
}

FreeCamera::FreeCamera()
    : m_yaw(0.0f)
    , m_pitch(0.0f)
    , m_speed(defaultSpeed())
    , m_fov(1.2f)                           // Renderer::initialize's own default view (RendDX9/BF2Render.cpp)
    , m_farPlane(1500.0f)
    , m_looking(false)
    , m_wheelReserved(false)
    , m_mode(CameraMode::Free)
    , m_distance(30.0f)
    , m_followHeight(30.0f)
    , m_ground(0)
    , m_groundContext(0)
{
    m_position[0] = m_position[1] = m_position[2] = 0.0f;
    m_centre[0] = m_centre[1] = m_centre[2] = 0.0f;
}

void FreeCamera::setGroundQuery(GroundQuery query, void* context)
{
    m_ground = query;
    m_groundContext = context;
}

const char* FreeCamera::modeName(CameraMode mode)
{
    switch (mode)
    {
    case CameraMode::TopDown:       return "top-down";
    case CameraMode::Orbit:         return "orbit";
    case CameraMode::FollowTerrain: return "follow the terrain";
    default:                        return "free";
    }
}

void FreeCamera::lookAt(const float target[3])
{
    const float d[3] = { target[0] - m_position[0], target[1] - m_position[1], target[2] - m_position[2] };
    const float length = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (length < 1e-3f)
        return;
    m_yaw = std::atan2(d[0], d[2]);
    float pitch = std::asin(d[1] / length);
    if (pitch > kMaxPitch)  pitch = kMaxPitch;
    if (pitch < -kMaxPitch) pitch = -kMaxPitch;
    m_pitch = pitch;
}

void FreeCamera::setMode(CameraMode mode, const float centre[3])
{
    m_mode = mode;
    switch (mode)
    {
    case CameraMode::TopDown:
        m_pitch = -kMaxPitch;
        break;
    case CameraMode::Orbit:
    {
        if (centre)
            for (int i = 0; i < 3; ++i)
                m_centre[i] = centre[i];
        const float d[3] = { m_position[0] - m_centre[0], m_position[1] - m_centre[1], m_position[2] - m_centre[2] };
        m_distance = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (m_distance < 2.0f)
            m_distance = 2.0f;
        lookAt(m_centre);
        break;
    }
    case CameraMode::FollowTerrain:
    {
        float ground = 0.0f;
        m_followHeight = m_ground && m_ground(m_groundContext, m_position[0], m_position[2], ground)
                       ? m_position[1] - ground : 30.0f;
        if (m_followHeight < 1.0f)
            m_followHeight = 1.0f;
        break;
    }
    default:
        break;
    }
}

void FreeCamera::keepAboveTerrain()
{
    float ground = 0.0f;
    if (m_ground && m_ground(m_groundContext, m_position[0], m_position[2], ground))
        m_position[1] = ground + m_followHeight;
}

void FreeCamera::place(float x, float y, float z, float yawDegrees, float pitchDegrees)
{
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
    m_yaw = yawDegrees / kDegrees;
    m_pitch = pitchDegrees / kDegrees;
    // A placed camera keeps its mode; an orbit re-centres on what it now looks at, a terrain-follower
    // takes its new height above the ground.
    if (m_mode == CameraMode::Orbit)
    {
        const float cosPitch = std::cos(m_pitch);
        const float forward[3] = { std::sin(m_yaw) * cosPitch, std::sin(m_pitch), std::cos(m_yaw) * cosPitch };
        for (int i = 0; i < 3; ++i)
            m_centre[i] = m_position[i] + forward[i] * m_distance;
    }
    else if (m_mode == CameraMode::FollowTerrain)
    {
        setMode(CameraMode::FollowTerrain, 0);
    }
    else if (m_mode == CameraMode::TopDown)
    {
        m_pitch = -kMaxPitch;
    }
}

void FreeCamera::setSpeed(float metresPerSecond)
{
    // The wheel's own range (update()).
    m_speed = metresPerSecond < 1.0f ? 1.0f : (metresPerSecond > 2000.0f ? 2000.0f : metresPerSecond);
}

float FreeCamera::yawDegrees() const
{
    return m_yaw * kDegrees;
}

float FreeCamera::pitchDegrees() const
{
    return m_pitch * kDegrees;
}

void FreeCamera::update(float seconds)
{
    const ImGuiIO& io = ImGui::GetIO();

    // Look: the right button must go down over the 3D view (not over a panel); then the look keeps
    // going while it is held, wherever the cursor wanders.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !pointerOverUi())
        m_looking = true;
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
        m_looking = false;
    if (m_looking)
    {
        m_yaw += io.MouseDelta.x * kLookSensitivity * m_lookFactor;
        if (m_mode != CameraMode::TopDown)
            m_pitch -= io.MouseDelta.y * kLookSensitivity * m_lookFactor;
        if (m_pitch > kMaxPitch)  m_pitch = kMaxPitch;
        if (m_pitch < -kMaxPitch) m_pitch = -kMaxPitch;
    }
    if (io.MouseWheel != 0.0f && !pointerOverUi() && !(m_wheelReserved && (io.KeyShift || io.KeyCtrl)))
    {
        if (m_mode == CameraMode::Orbit)
        {
            // The wheel zooms towards the centre.
            m_distance *= std::pow(1.25f, -io.MouseWheel);
            if (m_distance < 2.0f)    m_distance = 2.0f;
            if (m_distance > 3000.0f) m_distance = 3000.0f;
        }
        else
        {
            m_speed *= std::pow(1.25f, io.MouseWheel);
            if (m_speed < 1.0f)    m_speed = 1.0f;
            if (m_speed > 2000.0f) m_speed = 2000.0f;
        }
    }

    float along = 0.0f, side = 0.0f, lift = 0.0f;
    // Keys belong to the camera unless something is typed or a panel under the cursor has them; Ctrl+S,
    // Ctrl+Z ... are shortcuts.
    if ((keyboardForView() || m_looking) && !io.KeyCtrl && !io.WantTextInput)
    {
        // The keys are the shortcut table's (W/S/A/D, E/Q by default).
        along = (shortcutDown("camera.forward") ? 1.0f : 0.0f) - (shortcutDown("camera.back") ? 1.0f : 0.0f);
        side = (shortcutDown("camera.right") ? 1.0f : 0.0f) - (shortcutDown("camera.left") ? 1.0f : 0.0f);
        lift = (shortcutDown("camera.up") ? 1.0f : 0.0f) - (shortcutDown("camera.down") ? 1.0f : 0.0f);
    }
    const float step = m_speed * seconds * (io.KeyShift ? 4.0f : 1.0f);
    const float cosPitch = std::cos(m_pitch);
    const float forward[3] = { std::sin(m_yaw) * cosPitch, std::sin(m_pitch), std::cos(m_yaw) * cosPitch };
    const float level[3] = { std::sin(m_yaw), 0.0f, std::cos(m_yaw) };     // forward, flattened
    const float right[3] = { std::cos(m_yaw), 0.0f, -std::sin(m_yaw) };

    switch (m_mode)
    {
    case CameraMode::Orbit:
    {
        // W/S/A/D and E/Q move the centre, level with the ground; the camera sits `distance` back from
        // it along its view.
        for (int i = 0; i < 3; ++i)
            m_centre[i] += (level[i] * along + right[i] * side) * step;
        m_centre[1] += lift * step;
        for (int i = 0; i < 3; ++i)
            m_position[i] = m_centre[i] - forward[i] * m_distance;
        break;
    }
    case CameraMode::TopDown:
        m_pitch = -kMaxPitch;
        for (int i = 0; i < 3; ++i)
            m_position[i] += (level[i] * along + right[i] * side) * step;
        m_position[1] += lift * step;
        break;
    case CameraMode::FollowTerrain:
        for (int i = 0; i < 3; ++i)
            m_position[i] += (level[i] * along + right[i] * side) * step;
        m_followHeight += lift * step;
        if (m_followHeight < 1.0f)
            m_followHeight = 1.0f;
        keepAboveTerrain();
        break;
    default:
        for (int i = 0; i < 3; ++i)
            m_position[i] += (forward[i] * along + right[i] * side) * step;
        m_position[1] += lift * step;
        break;
    }
}

void FreeCamera::basis(float right[3], float up[3], float forward[3]) const
{
    const float cosPitch = std::cos(m_pitch);
    forward[0] = std::sin(m_yaw) * cosPitch;
    forward[1] = std::sin(m_pitch);
    forward[2] = std::cos(m_yaw) * cosPitch;
    right[0] = std::cos(m_yaw);
    right[1] = 0.0f;
    right[2] = -std::sin(m_yaw);
    up[0] = forward[1] * right[2] - forward[2] * right[1];                  // forward x right
    up[1] = forward[2] * right[0] - forward[0] * right[2];
    up[2] = forward[0] * right[1] - forward[1] * right[0];
}

void FreeCamera::cameraToWorld(float out[16]) const
{
    float right[3], up[3], forward[3];
    basis(right, up, forward);
    for (int i = 0; i < 3; ++i)
    {
        out[0 + i] = right[i];
        out[4 + i] = up[i];
        out[8 + i] = forward[i];
        out[12 + i] = m_position[i];
    }
    out[3] = out[7] = out[11] = 0.0f;
    out[15] = 1.0f;
}

void FreeCamera::worldToView(float out[16]) const
{
    // The inverse of an orthonormal basis plus a translation: the transposed rotation, and the
    // position carried through it with the sign turned.
    float right[3], up[3], forward[3];
    basis(right, up, forward);
    const float* axes[3] = { right, up, forward };
    for (int column = 0; column < 3; ++column)
    {
        for (int row = 0; row < 3; ++row)
            out[row * 4 + column] = axes[column][row];
        out[12 + column] = -(m_position[0] * axes[column][0] + m_position[1] * axes[column][1]
                             + m_position[2] * axes[column][2]);
    }
    out[3] = out[7] = out[11] = 0.0f;
    out[15] = 1.0f;
}

void FreeCamera::rayThroughPixel(float x, float y, int width, int height, float origin[3], float direction[3]) const
{
    float right[3], up[3], forward[3];
    basis(right, up, forward);
    const float tanHalf = std::tan(m_fov * 0.5f);
    const float ndcX = width > 0 ? 2.0f * x / static_cast<float>(width) - 1.0f : 0.0f;
    const float ndcY = height > 0 ? 1.0f - 2.0f * y / static_cast<float>(height) : 0.0f;
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const float viewX = ndcX * tanHalf * aspect;
    const float viewY = ndcY * tanHalf;
    float length = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        origin[i] = m_position[i];
        direction[i] = right[i] * viewX + up[i] * viewY + forward[i];
        length += direction[i] * direction[i];
    }
    length = std::sqrt(length);
    for (int i = 0; i < 3; ++i)
        direction[i] /= length;
}

bool FreeCamera::worldToPixel(const float point[3], int width, int height, float& x, float& y) const
{
    if (width <= 0 || height <= 0)
        return false;
    float right[3], up[3], forward[3];
    basis(right, up, forward);
    const float d[3] = { point[0] - m_position[0], point[1] - m_position[1], point[2] - m_position[2] };
    const float viewX = d[0] * right[0] + d[1] * right[1] + d[2] * right[2];
    const float viewY = d[0] * up[0] + d[1] * up[1] + d[2] * up[2];
    const float viewZ = d[0] * forward[0] + d[1] * forward[1] + d[2] * forward[2];
    if (viewZ < 0.05f)
        return false;
    const float tanHalf = std::tan(m_fov * 0.5f);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    x = (viewX / (viewZ * tanHalf * aspect) + 1.0f) * 0.5f * static_cast<float>(width);
    y = (1.0f - viewY / (viewZ * tanHalf)) * 0.5f * static_cast<float>(height);
    return true;
}

void FreeCamera::frame(const float centre[3], float radius)
{
    float right[3], up[3], forward[3];
    basis(right, up, forward);
    if (radius < 1.0f)
        radius = 1.0f;
    float distance = radius / std::sin(m_fov * 0.5f) * 1.2f;
    if (distance < 5.0f)
        distance = 5.0f;
    for (int i = 0; i < 3; ++i)
        m_position[i] = centre[i] - forward[i] * distance;
    // Framing in orbit mode orbits the framed object from then on.
    if (m_mode == CameraMode::Orbit)
    {
        for (int i = 0; i < 3; ++i)
            m_centre[i] = centre[i];
        m_distance = distance;
    }
    else if (m_mode == CameraMode::FollowTerrain)
    {
        setMode(CameraMode::FollowTerrain, 0);
    }
}

void FreeCamera::apply(dice::hfe::dx9rend::IRenderView* view, int width, int height) const
{
    if (!view || width <= 0 || height <= 0)
        return;
    float matrix[16];
    cameraToWorld(matrix);
    view->setViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    view->setFov(m_fov);
    view->setAspect(static_cast<float>(height) / static_cast<float>(width));
    view->setFarPlane(m_farPlane);
    view->setViewMatrix(matrix);
}

} // namespace bf2editor
