// Publishes the null nametag manager during engine singleton setup.
// The shared INametagManager declaration preserves the prefix used by control points
// and spectator code; the differing implementation tails remain outside that interface.

#include "SingletonServer.h"

#include "../Core/ClassManager.h"

#include <map>
#include <string>

namespace dice {
namespace hfe {

extern ClassManager* classManager;

// Fallback nametag implementation for images without a registered HUD manager.
// Operations are no-ops; reference counting is inert and the object lives until
// process exit, matching dedicated-server behavior.
namespace {

class NullNametagManager : public INametagManager
{
public:
    NullNametagManager() : m_unknown04(0) {}

    // The IBase quartet - all stubs in the binary (7-10 bytes each).
    virtual void  addRef()                                  {}
    virtual int   getRef()                                  { return 1; }
    virtual int   release()                                 { return 1; }
    virtual void* queryInterface(unsigned int) const        { return 0; }   // 7 B: `xor eax,eax`

    // The nametag surface - every one a no-op, in the vtable's own order.
    virtual void  init(const std::string&, const std::string&)      {}
    virtual void  update()                                          {}
    virtual void  draw()                                            {}
    //  xor eax,eax / PE 0x7318E0 xor al,al; ret 4.
    virtual bool  addTouchItem(world::IPlayer*)                     { return false; }
    virtual void  addTouchItem(world::ControlPoint*)                {}
    virtual void  addTouchItem(world::IObject*)                     {}
    virtual void  addTouchItem(SquadOrder*, int, const Vec3&)       {}
    virtual void  addTouchItem(SquadOrder*, int, world::IObject*)   {}
    virtual void  addTouchItem(world::IObject*, int)                {}
    virtual void* getColors(int)                                    { return 0; }
    virtual void* getTransformations(int)                           { return 0; }
    virtual void* getAlphaArray(int)                                { return 0; }
    virtual void  rebufferAll()                                     {}
    virtual void  enableBuffering()                                 {}
    virtual void  clearAll()                                        {}
    virtual void  removeItem(int, void*)                            {}
    virtual void  removeItem(int, void*, int)                       {}
    // 11 B: returns the map's internal word. [CHOSEN] as the map's address here - the concrete
    // type is [UNRESOLVED], nothing in the server build consumes the value, and the null object's
    // contract is only "something stable and empty".
    virtual const void* getItems() const                            { return &m_nametags; }
    virtual bool  get3dMapOn() const                                { return false; }
    virtual void  set3dMapOn(bool)                                  {}
    //  / BF2.exe 0x5AB2A0 `ret 4`.
    virtual void  setSpectatedPlayer(world::IPlayer*)               {}
    //  xor eax,eax / BF2.exe 0x80D430 `xor eax,eax; ret`.
    virtual world::IPlayer* getSpectatedPlayer() const              { return 0; }
    //  / BF2.exe 0x5AB2B0 `xor al,al; ret`.
    virtual bool  isDemoPlayback() const                            { return false; }

private:
    /* +0x04 */ int m_unknown04;
    /* +0x08 */ std::map<std::string, int> m_nametags;   // [UNRESOLVED] key/value types; the ctor
                                                         // only builds an empty _Rb_tree header
};

} // anonymous namespace

// Installs the registered NametagManager or a process-lifetime null implementation.
// There is no matching SingletonServer teardown.
INametagManager* SingletonServer::m_iNametagManager = 0;     // ds:0x8AB0980 - attested class-static

void SingletonServer::init()
{
    m_iNametagManager = static_cast<INametagManager*>(
        classManager->getSingleton("NametagManager"));

    if (!m_iNametagManager)
        m_iNametagManager = new NullNametagManager();
}

} // namespace hfe
} // namespace dice
