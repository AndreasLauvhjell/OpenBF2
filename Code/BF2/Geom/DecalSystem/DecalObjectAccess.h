// Path: [INFERRED] narrow shared ABI access beside the attested DecalManager.cpp.
// The concrete owner is identified by RendDX9.dll's DecalObject.cpp assertion at 102218B0,
// ctor 101AC7B0 and table 10221860. This does not publish a guessed positional interface.
#ifndef BF2_GEOM_DECALSYSTEM_DECALOBJECTACCESS_H
#define BF2_GEOM_DECALSYSTEM_DECALOBJECTACCESS_H

#include "../../Math/Mat4.h"

#if defined(_MSC_VER) && defined(_M_IX86)
#define BF2_DECAL_OBJECT_CALL __thiscall
#else
#define BF2_DECAL_OBJECT_CALL
#endif

namespace dice { namespace hfe { namespace geom { namespace decal_object {

template<class Function>
inline Function slot(void* object, unsigned int peIndex, unsigned int elfIndex)
{
    // Logical indices, not byte offsets divided by native pointer size. The ELF owner
    // has two destructor entries; MSVC has one, including the native x64 build.
#if defined(_MSC_VER)
    const unsigned int index = peIndex;
#else
    const unsigned int index = elfIndex;
#endif
    return reinterpret_cast<Function>((*static_cast<void***>(object))[index]);
}

// PCO ELF819A970 (+1C) / dedicated PE59763C (+18); renderer target101AE4B0.
// The two trailing arguments are POINTERS: helper101ADF80 dereferences the collision
// mesh's table and the face-info record (lod byte0, face u16+2). Their source constness
// and the decal-template interface remain [UNRESOLVED], so they stay opaque here.
// The returned ID is a 32-bit unsigned value (PCO's ELF vector<uint> instantiation).
inline unsigned int add(void* object, const Mat4* transform, void* decalTemplate,
                        float scale, void* collisionMesh, void* collisionFaceInfo)
{
    typedef unsigned int (BF2_DECAL_OBJECT_CALL* Function)(
        void*, const Mat4*, void*, float, void*, void*);
    return slot<Function>(object, 6, 7)(
        object, transform, decalTemplate, scale, collisionMesh, collisionFaceInfo);
}

// PCO ELF819AA9B (+20) / dedicated PE5976CD (+1C); renderer target101ACEB0.
// The whole 32-bit ID is consumed; no pointer-sized ID or narrowing to a byte is imposed.
inline void remove(void* object, unsigned int decalId)
{
    typedef void (BF2_DECAL_OBJECT_CALL* Function)(void*, unsigned int);
    slot<Function>(object, 7, 8)(object, decalId);
}

// Armor ELF82190BD (+18) / client PE6857AD (+14); renderer target101AC840.
// This no-argument operation drains both decal lists and resets the ring cursors.
inline void clear(void* object)
{
    typedef void (BF2_DECAL_OBJECT_CALL* Function)(void*);
    slot<Function>(object, 5, 6)(object);
}

// ~GameLogic ELF80CB112 (+2C) / client PE4F61EF (+28); ~SimpleObject ELF81EE9E8 (+2C) / client
// PE627D72 (+28); renderer target101AC6B0 (15 B): `mov eax, ecx; mov ecx, g_DecalManager;
// push eax; call [edx+18h]` - the object hands ITSELF to the manager's deletion queue
// (IDecalManager::enqueue, +0x18: pushed onto the +0x4C list unless +0x2A4 is set), and
// DecalManager::update (101248E0 -> 101247B0) drains that queue through each object's
// scalar-deleting destructor (slot 0, flag 1). So this is the owner's LAST call: neither BF2.exe
// owner touches the pointer again (SimpleObject zeroes its member). [INFERRED] name.
inline void release(void* object)
{
    typedef void (BF2_DECAL_OBJECT_CALL* Function)(void*);
    slot<Function>(object, 10, 11)(object);
}

} } } }
#undef BF2_DECAL_OBJECT_CALL
#endif
