// [TEMP] [CHOSEN, not recovered] Native map/device fixture, not production startup.
// Real manager, all real device TUs; fake devices model only ordered dispatch and
// ownership. No device initialization bit is requested and no hardware is touched.
#include <map>
#include <string>
#include <vector>
#define private public
#include "../BF2/IO/Input/DX8InputDeviceManager.h"
#undef private
#include "../BF2/Core/Ids.h"
#include <cstdio>

namespace dice { namespace hfe {
class BF2Render; BF2Render* g_bf2Render = 0;
namespace io { IBase* inputDeviceManager = 0; IBase* controlMapManager = 0; }
} }
using namespace dice::hfe;
using namespace dice::hfe::io;
std::vector<int> calls;
class FixtureDevice : public IInputDevice {
public:
    explicit FixtureDevice(int id) : id(id), refs(1), lastDt(0) {}
    void addRef() override { ++refs; }
    int getRef() override { return refs; }
    int release() override { calls.push_back(100 + id); return --refs; }
    void* queryInterface(unsigned int) const override { return 0; }
    InputDeviceTypes getType() const override { return IDTUndefined; }
    bool setCooperativeMode(InputDeviceMode) override { return false; }
    InputDeviceMode getCooperativeMode() const override { return IDM_EXCLUSIVE; }
    bool activate() override { calls.push_back(id); return false; }
    bool deActivate() override { calls.push_back(10 + id); return false; }
    void update(float dt) override { calls.push_back(20 + id); lastDt = dt; }
    bool isActive() const override { return false; }
    bool getNextInput(InputDeviceAxes&, InputDeviceButtons&, InputDeviceKeys&, bool&) override { return false; }
    int id, refs; float lastDt;
};
int main()
{
    unsigned checks = 0, failures = 0;
    auto check = [&](bool condition, const char* name) {
        ++checks; if (!condition) { ++failures; std::printf("FAIL: %s\n", name); }
    };
    FixtureDevice mouse(1), keyboard(2), controller(4);
    {
        DX8InputDeviceManager manager;
        check(manager.getRef() == 1 && manager.getInputDevices() == 0 && manager.getActiveInputDevices() == 0, "constructor state");
        check(manager.queryInterface(IID_IBase) == &manager && manager.queryInterface(IID_IInputDeviceManager) == &manager &&
            !manager.queryInterface(0), "queryInterface without addref");
        manager.addRef(); check(manager.release() == 1, "reference round trip");
        check(manager.getAllDevices() == &manager.m_devices, "native container reference");
        unsigned int mask = 0;
        check(manager.initDevices(&mask) && mask == 0, "empty mask succeeds");
        mask = 0x30;
        check(!manager.initDevices(&mask) && mask == 0, "unsupported requested bits narrow mask and fail");
        check(!manager.getInputDevice(IDF_KEYBOARD), "lookup miss");
        manager.m_devices.insert(std::make_pair(IDF_GAME_CONTROLLER, &controller));
        manager.m_devices.insert(std::make_pair(IDF_MOUSE, &mouse));
        manager.m_devices.insert(std::make_pair(IDF_KEYBOARD, &keyboard));
        check(manager.getInputDevices() == 7 && manager.getActiveInputDevices() == 0, "stored devices independent of init mask");
        check(manager.getInputDevice(IDF_KEYBOARD) == &keyboard && !manager.getInputDevice(static_cast<InputDeviceFlags>(3)), "lookup uses exact key");
        check(manager.activateDevices(5) && calls == std::vector<int>({1,4}), "activation ignores failures and follows map order");
        calls.clear();
        check(manager.deActivateDevices(6) && calls == std::vector<int>({12,14}), "deactivation masks and ignores failures");
        calls.clear(); manager.update(0.125f);
        check(calls == std::vector<int>({21,22,24}) && mouse.lastDt == 0.125f && keyboard.lastDt == 0.125f && controller.lastDt == 0.125f,
              "update forwards dt to all stored devices in order");
        manager.m_activeDevices = 0x400;
        check(manager.getActiveInputDevices() == 0x400, "active getter reads stored mask");
        mask = 0;
        check(manager.initDevices(&mask) && manager.getInputDevices() == 7 && manager.getActiveInputDevices() == 0,
              "reinitialization does not clear previous map entries");
        calls.clear();
    }
    check(calls == std::vector<int>({101,102,104}) && !mouse.refs && !keyboard.refs && !controller.refs, "ordered destructor releases each stored reference once");
    check(!DX8InputDeviceManager::create(IID_IInputDeviceManager, &mouse), "factory rejects aggregation");
    check(!DX8InputDeviceManager::create(0, 0), "factory releases unsupported IID object");
    IBase* result = DX8InputDeviceManager::create(IID_IInputDeviceManager, 0);
    check(result && result->getRef() == 1 && result->release() == 0, "factory constructs and destroys real manager");
    std::printf("DX8InputDeviceManager: %u/%u checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
