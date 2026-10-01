// Path: implementation attested by RendDX9.dll 1.50 string10222428.
// PE-only renderer class, table102224FC. Names without RTTI are [INFERRED].
#ifndef BF2_REND_ENVMAPMANAGER_H
#define BF2_REND_ENVMAPMANAGER_H
#ifdef BF2_IMAGE_RENDDX9
#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"
#include "../../../RendDX9Abi/Vc71Map.h"
#include "../../../RendDX9Abi/Vc71Vector.h"
#include <cstddef>
namespace dice { namespace hfe {
namespace dx9rend { struct EngineTexture; }
namespace Rend {
class EnvMapObject;
class EnvMapObjectTemplate;
struct EnvMapEntry
{
    Vec3 position;
    bool flag;
    unsigned char pad[3];
    dx9rend::EngineTexture* texture;
    EnvMapEntry();
};
static_assert(sizeof(EnvMapEntry)==0x14,"EnvMap entry allocation");
class EnvMapManager : public IBase
{
public:
    EnvMapManager();
    ~EnvMapManager(); // non-virtual: no destructor slot in this table
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void reset();
    virtual EnvMapObject* createObject(EnvMapObjectTemplate*);
    virtual void queueRemoval(EnvMapObject*);
    virtual EnvMapObjectTemplate* getTemplate(const vc71::string&); // +1C,101BFC60
    virtual void removeTemplate(EnvMapObjectTemplate*);         // +20,101BF0C0 retail empty
    virtual void setEnabled(bool);
    virtual bool getEnabled() const;
    virtual void setPerPixelEnabled(bool);                     // +2C,101C1F50
    virtual bool getPerPixelEnabled() const;
    virtual bool loadInfo(const vc71::string&);
    virtual void clear();
    void flushRemovals();
    void clearObjects();
    void clearMaps();
    void onPostLoad();
    void update(float);
    void removeEntry(EnvMapEntry*);
    // The editor's capture and writer. Non-virtual: the `generateEnvMaps` and
    // `save` console executors (101C21D7 / 101C2244) call them directly on the published manager
    // and discard the answer. [INFERRED] names, from those bindings.
    bool generateEnvMaps();
    bool save();

    int m_refCount;                             // +04
    vc71::vector_layout m_objects;               // +08
    vc71::map_layout m_templates;                // +18, case-insensitive string -> template
    vc71::vector_layout m_pendingRemoval;        // +24
    vc71::map_layout m_map34;                    // +34,24-byte nodes
    vc71::map_layout m_map40;                    // +40,24-byte nodes
    vc71::map_layout m_maps;                     // +4C,string ->20-byte map record
    unsigned char m_untouched58[8];              // +58,constructor does not initialize
    unsigned short m_templateCount;             // +60
    bool m_enabled;                             // +62
    unsigned char m_pad63;
    dx9rend::EngineTexture* m_currentTexture;    // +64,borrowed
    bool m_perPixelEnabled;                     // +68
    unsigned char m_pad69[3];
};
static_assert(sizeof(EnvMapManager)==0x6C,"EnvMapManager factory allocation");
static_assert(offsetof(EnvMapManager,m_currentTexture)==0x64,"Bundled/Terrain texture read");
extern IBase* g_EnvMapManager;
IBase* __fastcall createEnvMapManager(unsigned int iid,IBase* outer);
} } }
#endif
#endif
