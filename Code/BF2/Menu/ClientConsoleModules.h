// Client modules owning HUD-manager and profile console bindings. [INFERRED] Names and header placement.
#ifndef MENU_CLIENTCONSOLEMODULES_H
#define MENU_CLIENTCONSOLEMODULES_H
#include "../Core/IModule.h"
#if !DEDICATED
namespace dice { namespace hfe { namespace hud {

class HudConsoleModule : public IModule {
public:
    HudConsoleModule() : m_refCount(1) {} // inlined447D74/447D7A, allocation8
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    void getClassList(std::vector<ClassRegInfo>&) override;        // 57F33A, actual RET4
    void getSingletonList(std::vector<SingletonRegInfo>&) override;// 57F33A, actual RET4
    bool init() override;                                        // 75C923, registers all228 bindings
    bool close() override;                                       // 75C936, unregisters same list
private:
    int m_refCount;
};

class ProfileConsoleModule : public IModule {
public:
    ProfileConsoleModule() : m_refCount(1) {} // inlined447DA1/447DA7, allocation8
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    void getClassList(std::vector<ClassRegInfo>&) override;        // 57F33A, actual RET4
    void getSingletonList(std::vector<SingletonRegInfo>&) override;// 57F33A, actual RET4
    bool init() override;
    bool close() override;
private:
    int m_refCount;
};

} } }
#endif
#endif
