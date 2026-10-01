// Side-DLL handshake exports. Compile this interface into loadable modules, not the engine.
// initDll borrows host services, registers modules, then initializes them; deinitDll tears them down.
// The exported names use C linkage inside dice::hfe. Services cross the ClassManager registry.
// The original initDll parameter is IClassManager*; this tree uses ClassManager* for its matching
// virtual surface. Preserve that ABI when introducing a distinct interface type.

#ifndef CORE_DLLMODULE_H        // [INFERRED] guard style
#define CORE_DLLMODULE_H

namespace dice {
namespace hfe {

class ClassManager;             // Core/ClassManager.h - attested parameter type is IClassManager*
class IModule;                  // Core/IModule.h      - IBase -> IClassServer -> IModule

// Null-terminated module table supplied by each loadable image; constructors do not self-register.
// The dedicated executable supplies no table and must not link this DLL initialization path.
extern IModule* dllmodules[];

// Returns the null-terminated module-table length, caching nonzero results for the image lifetime.
int getNoModules();

extern "C" {

// Binds engine globals, registers this image's modules, and initializes them.
// Returns whether the required singletons were found; module-init failures do not affect the result.
bool initDll(ClassManager* pClsMgr);

// Shut every module down in reverse order, unregister each from the host, and release it.

// return type [UNRESOLVED], and deliberately so. Itanium does not encode return types, and unlike
// initDll this body materialises no deliberate value - the decompiler's `result` is just whatever
// the last `release()` left in eax. `void` is chosen because nothing calls it internally and the
// export is invoked for effect. The same reasoning that corrected IModule::close to `bool` (every
// implementation ends `mov eax, 1`) argues for `void` here, where no such store exists.
void deinitDll();

} // extern "C"

} // namespace hfe
} // namespace dice

#endif // CORE_DLLMODULE_H
