#include <cstdlib>
// Test scaffolding. [CHOSEN, not recovered] readback/observation fixture, excluded from production.
// Called after the actual BF2::init -> startGame -> GameServer stock Wake startup.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../../BF2/IO/Console/Console.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Scene/Object/ObjectUtil.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Game/Objects/RotationalBundleLeaves.h"
#include "../../BF2/Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Physics/ICollisionMesh.h"
#include "../../BF2/Core/Ids.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace {
int idleChecks = 0;
int idleFailures = 0;
bool boundsPass = false;
bool aircraftBounds()
{
    int checks = 0, failures = 0;
    std::vector<IObject*> pending;
    for (const auto& entry : objectManager->getAllRegisteredObjects())
    {
        IObject* root = entry.second;
        if (!root || root->getParent() || !root->getTemplate()) continue;
        std::string name = root->getTemplate()->getName();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
        if (name != "air_f35b" && name != "air_j10" && name != "ahe_ah1z"
            && name != "ahe_z10" && name != "chthe_z8") continue;
        if (!root->queryComponent(IID_IArmorGLComp, IID_IArmorGLComp)) continue;
        pending.push_back(root);
    }
    while (!pending.empty())
    {
        IObject* wheel = pending.back();
        pending.pop_back();
        for (IObject* child = wheel->getChild(); child; child = object_detail::LayoutAccess::sibling(child))
            pending.push_back(child);
        if (!wheel->getTemplate() || wheel->getTemplate()->getClassID() != CID_SpringTemplate) continue;
        ICollisionMesh* mesh = static_cast<ICollisionMesh*>(object_detail::LayoutAccess::collisionMesh(wheel));
        if (!mesh || !mesh->hasLod(1)) continue;
        const Vec3* vertices = static_cast<const Vec3*>(mesh->getVertexVector(1));
        const float radius = mesh->getBoundingRadius(1);
        bool encloses = std::isfinite(radius) && radius > 0.0f;
        for (unsigned int i = 0; i < mesh->getVertexCount(1); ++i)
        {
            const Vec3& v = vertices[i];
            if (v.x*v.x + v.y*v.y + v.z*v.z > radius*radius + 0.000001f)
                encloses = false;
        }
        ++checks;
        if (!encloses)
        {
            ++failures;
            std::printf("[aircraft] BOUNDS_FAIL %s radius=%g does not enclose its tyre vertices\n",
                        wheel->getTemplate()->getName().c_str(), radius);
        }
    }
    std::printf("[aircraft] BOUNDS checks=%d failures=%d\n", checks, failures);
    return checks > 0 && failures == 0;
}
std::string console(const std::string& command)
{
    std::string reply;
    io::mainConsole->executeLine(command, reply, false, false, false, false, false);
    return reply;
}
std::vector<float> numbers(std::string text)
{
    std::replace(text.begin(), text.end(), '/', ' ');
    std::istringstream stream(text);
    std::vector<float> result;
    float value;
    while (stream >> value) result.push_back(value);
    if (!stream.eof()) result.clear();
    return result;
}
bool valuesMatch(const std::string& actual, const std::string& expected)
{
    const auto a = numbers(actual), e = numbers(expected);
    if (a.empty() || a.size() != e.size()) return false;
    for (size_t i=0; i<a.size(); ++i)
        if (!std::isfinite(a[i]) || std::fabs(a[i]-e[i]) > 0.00001f * (1.0f+std::fabs(e[i])))
            return false;
    return true;
}
}

bool aircraftCheckBindings()
{
    std::ifstream input(std::getenv("BF2_TIER2_EXPECTATIONS"));
    int checks = 0, failures = 0, unloaded = 0;
    IObjectTemplate* saved = objectTemplateManager->getActiveObjectTemplate();
    std::string line;
    while (std::getline(input, line))
    {
        std::istringstream fields(line);
        std::string name, property, expected;
        fields >> name >> property >> expected;
        IObjectTemplate* objectTemplate = objectTemplateManager->getTemplate(name);
        if (!objectTemplate) { ++unloaded; continue; }
        objectTemplateManager->setActiveObjectTemplate(objectTemplate);
        const std::string actual = console("ObjectTemplate." + property);
        ++checks;
        if (!valuesMatch(actual, expected))
        {
            ++failures;
            std::printf("[aircraft] MISMATCH %s.%s expected=%s actual=%s\n",
                        name.c_str(), property.c_str(), expected.c_str(), actual.c_str());
        }
    }
    // Exercise the nontrivial retail setter through the console, on an isolated real template.
    {
        WingTemplate wing;
        objectTemplateManager->setActiveObjectTemplate(&wing);
        console("ObjectTemplate.setRegulateToLift 0");
        ++checks;
        if (!wing.m_liftRegulated || wing.m_regulateToLift != 0.0f) ++failures;
        console("ObjectTemplate.setLiftRegulated 0");
        console("ObjectTemplate.setWingToRegulatorRatio -0.25");
        ++checks;
        if (wing.m_liftRegulated || wing.m_wingToRegulatorRatio != -0.25f) ++failures;
        objectTemplateManager->setActiveObjectTemplate(saved);
    }
    std::printf("[aircraft] BINDINGS checks=%d failures=%d unloaded=%d\n", checks, failures, unloaded);
    std::fflush(stdout);
    return checks > 200 && failures == 0;
}

void aircraftSnapshot(int frame)
{
    if (frame == 300) boundsPass = aircraftBounds();
    for (const auto& entry : objectManager->getAllRegisteredObjects())
    {
        IObject* object = entry.second;
        if (!object || object->getParent() || !object->getTemplate()
            || object->getTemplate()->getClassID() != CID_PlayerControlObjectTemplate) continue;
        std::string name = object->getTemplate()->getName();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
        if (name.find("air_") != 0 && name.find("ahe_") != 0 && name.find("the_") != 0
            && name.find("chthe_") != 0 && name.find("ushe_") != 0) continue;
        const Vec3 position = object->getAbsolutePosition();
        IPhysicsNode* physics = static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(object));
        IArmorGLComp* armor = static_cast<IArmorGLComp*>(object->queryComponent(IID_IArmorGLComp, IID_IArmorGLComp));
        if (!armor) continue; // Inactive cached roots have no live armor component.
        Vec3 speed = physics ? physics->getPositionalSpeed() : Vec3::getZero();
        std::printf("[aircraft] IDLE frame=%d name=%s id=%u pos=%.5f/%.5f/%.5f speed=%.5f/%.5f/%.5f hp=%.3f sleep=%d\n",
                    frame, name.c_str(), entry.first, position.x, position.y, position.z,
                    speed.x, speed.y, speed.z, armor ? armor->getHitPoints() : -1.0f,
                    physics ? physics->getIsSleeping() : -1);
        if (frame == 300 || frame == 900)
        {
            ++idleChecks;
            if (!physics || !physics->getIsSleeping()
                || !std::isfinite(speed.x) || !std::isfinite(speed.y) || !std::isfinite(speed.z)
                || speed.x * speed.x + speed.y * speed.y + speed.z * speed.z > 0.000001f
                || armor->getHitPoints() != armor->getMaxHitPoints())
                ++idleFailures;
        }
    }
    std::fflush(stdout);
}

bool aircraftCheckIdle()
{
    std::printf("[aircraft] IDLE_CHECK checks=%d failures=%d (requires both 300 and 900 frame samples)\n",
                idleChecks, idleFailures);
    std::fflush(stdout);
    return idleChecks == 10 && idleFailures == 0 && boundsPass;
}
