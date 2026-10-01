// D3DX9 effect call adapters, [CHOSEN, not recovered] source.
// Only observed slots are exposed. Raw dispatch avoids inventing declarations for unused methods;
// preserve each slot offset and calling convention. SetValue is +0x50; SetTechnique is +0xE8.

#ifndef RENDDX9ABI_D3DXEFFECT_H
#define RENDDX9ABI_D3DXEFFECT_H

namespace renddx9abi {

// Calls one ID3DXEffect slot by byte offset. All D3DX methods are __stdcall.
template <typename Fn>
inline Fn d3dxSlot(void* effect, unsigned byteOffset)
{
    void** vtbl = *reinterpret_cast<void***>(effect);
    return reinterpret_cast<Fn>(vtbl[byteOffset / 4]);
}

typedef unsigned long (__stdcall* D3dxAddRefFn)(void*);
typedef unsigned long (__stdcall* D3dxReleaseFn)(void*);
typedef long (__stdcall* D3dxGetParameterDescFn)(void*, void* handle, void* descOut);
typedef long (__stdcall* D3dxGetTechniqueDescFn)(void*, void* handle, void* descOut);
typedef void* (__stdcall* D3dxGetParameterFn)(void*, void* parent, unsigned index);
typedef void* (__stdcall* D3dxGetTechniqueFn)(void*, unsigned index);
typedef void* (__stdcall* D3dxGetTechniqueByNameFn)(void*, const char* name);
typedef void* (__stdcall* D3dxGetAnnotationByNameFn)(void*, void* handle, const char* name);
typedef long (__stdcall* D3dxSetTechniqueFn)(void*, void* handle);
typedef long (__stdcall* D3dxGetBoolFn)(void*, void* handle, int* out);
typedef long (__stdcall* D3dxGetIntFn)(void*, void* handle, int* out);
typedef long (__stdcall* D3dxGetIntArrayFn)(void*, void* handle, int* out, unsigned count);
typedef long (__stdcall* D3dxValidateTechniqueFn)(void*, void* handle);
typedef long (__stdcall* D3dxSetStateManagerFn)(void*, void* manager);
typedef long (__stdcall* D3dxBeginFn)(void*, unsigned* passes, unsigned flags);
typedef long (__stdcall* D3dxBeginPassFn)(void*, unsigned index);
typedef long (__stdcall* D3dxEndPassFn)(void*);
typedef long (__stdcall* D3dxEndFn)(void*);
typedef long (__stdcall* D3dxSetValueFn)(void*, void* handle, const void* data, unsigned bytes);
typedef long (__stdcall* D3dxSetBoolFn)(void*, void* handle, int value);
typedef long (__stdcall* D3dxSetBoolArrayFn)(void*, void* handle, const int* data, unsigned count);
typedef long (__stdcall* D3dxSetIntFn)(void*, void* handle, int value);
typedef long (__stdcall* D3dxSetIntArrayFn)(void*, void* handle, const int* data, unsigned count);
typedef long (__stdcall* D3dxSetFloatFn)(void*, void* handle, float value);
typedef long (__stdcall* D3dxSetFloatArrayFn)(void*, void* handle, const float* data,
                                              unsigned count);
typedef long (__stdcall* D3dxSetVectorFn)(void*, void* handle, const void* vec4);
typedef long (__stdcall* D3dxSetVectorArrayFn)(void*, void* handle, const void* data,
                                               unsigned count);
typedef long (__stdcall* D3dxSetMatrixFn)(void*, void* handle, const void* matrix);
typedef long (__stdcall* D3dxSetMatrixArrayFn)(void*, void* handle, const void* data,
                                               unsigned count);
typedef long (__stdcall* D3dxSetTextureFn)(void*, void* handle, void* baseTexture);
typedef long (__stdcall* D3dxSetMatrixPointerArrayFn)(void*, void* handle,
                                                       const void* const* matrices,
                                                       unsigned count);          // +0xA8
typedef long (__stdcall* D3dxCommitChangesFn)(void*);      // +0x104
typedef long (__stdcall* D3dxSetArrayRangeFn)(void*, void* handle, unsigned start, unsigned end);

} // namespace renddx9abi

#endif // RENDDX9ABI_D3DXEFFECT_H
