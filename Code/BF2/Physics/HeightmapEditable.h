// Writable terrain tile whose height/material edits mark it modified. Header path [INFERRED].
// Heightmap is the primary base, followed by IHeightmapEditable and IConsoleSaveable.
// Preserve adjusted interface pointers and native container layout; secondary-base offsets differ
// between PE and ELF. makeScript returns false without writing; file saves use the height/material APIs.

#ifndef PHYSICS_HEIGHTMAPEDITABLE_H
#define PHYSICS_HEIGHTMAPEDITABLE_H

#include <string>

#include "../Core/IBase.h"
#include "../IO/Console/IConsoleSaveable.h"
#include "Heightmap.h"

namespace dice {
namespace hfe {
namespace io {
class IStream;
}
namespace world {

// IHeightmapEditable - IID_IHeightmapEditable = 820002 (Core/Ids.h).
class IHeightmapEditable : public IBase
{
public:
    virtual ~IHeightmapEditable() {}

    virtual bool  createHeightData(int height)                            = 0;
    virtual bool  createMaterialData(int material)                        = 0;
    virtual void  setHeightRaw(int x, int z, unsigned short height)       = 0;
    virtual void* getHeightData()                                         = 0;
    virtual void  setHeightData(void* data)                               = 0;
    virtual void  setHeightInWorldCoords(float x, float z, float height)  = 0;
    virtual void  setWorldScaledHeightInWorldCoords(float x, float z, float height, int flags) = 0;
    virtual void  setHeightInLocalCoords(float x, float z, float height, int flags)            = 0;
    virtual bool  getModified() const                                     = 0;
    virtual void  setModified(bool modified)                              = 0;
};

// Editable tile adds IHeightmapEditable and IConsoleSaveable secondary interfaces.
// The [CHOSEN] edit-buffer field is allocated but never consumed/freed by the original destructors.
// Write flags allow rise (1), fall (2), or both (3); maintain these restrictions in brush callers.
class HeightmapEditable : public Heightmap, public IHeightmapEditable, public io::IConsoleSaveable
{
public:
    HeightmapEditable();
    virtual ~HeightmapEditable();

    static IBase* create(unsigned int iid, IBase* outer);

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // IHeightmap overrides
    virtual bool  loadHeightData(const std::string& name);
    virtual bool  saveHeightData(const std::string& name) const;
    virtual bool  getModified() const;
    virtual void  setModified(bool modified);
    virtual bool  saveMaterialData(const std::string& name) const;
    virtual int   getHeightFromGrid(int x, int z) const;

    // IHeightmapEditable
    virtual bool  createHeightData(int height);
    virtual bool  createMaterialData(int material);
    virtual void  setHeightRaw(int x, int z, unsigned short height);
    virtual void* getHeightData();
    virtual void  setHeightData(void* data);
    virtual void  setHeightInWorldCoords(float x, float z, float height);
    virtual void  setWorldScaledHeightInWorldCoords(float x, float z, float height, int flags);

    virtual void  setHeightInLocalCoords(float x, float z, float height, int flags);


    // Io::IConsoleSaveable
    virtual bool  makeScript(io::IStream* stream);

    // non-virtual
    void addHeight(int x, int z, int delta, int flags);
    void setHeight(int x, int z, unsigned short height, int flags);
    void updateQuadtreeAt(int x, int z, unsigned short height);

private:
    /* 0xD4 */ unsigned char* m_editBuffer;
    /* 0xD8 */ bool           m_modified;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_HEIGHTMAPEDITABLE_H
