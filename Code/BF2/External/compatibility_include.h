// Path:     [CHOSEN, not recovered] - upstream's own designated integrator seam.
//           base/tu_config.h:15 includes "compatibility_include.h" UNCONDITIONALLY, and the vendored
//           27-Feb-2006 tu-testbed drop shipped WITHOUT it (a full tu-testbed checkout carries one at
//           this level; this partial drop does not). So the drop cannot compile at all until this
//           file exists - creating it is COMPLETION of the upstream contract, not a modification of
//           upstream source. Nothing here is reverse-engineered DICE code; the routing DECISIONS are
//           evidenced (see below) but the file itself is scaffolding for OUR build.
//
// Contents: the three tu-testbed allocator hooks + tu_error_exit + the four TU_CONFIG_LINK_TO_*
//           switches. tu_config.h documents each hook as "#define ... in compatibility_include.h if
//           you want something different" - this is that file.
//
// =================================================================================================
// Allocator ownership
//
// Every gameswf allocation in the shipped SwiffPlayer.dll tail-calls sub_1006DB60, which is
//     void* bf_malloc_wrapper(unsigned size) { return dice::hfe::memory::bf_malloc(size, g_nameId); }
// and the frees go through sub_1006DBA0 / sub_1006DBB0 -> dice::hfe::memory::bf_free. The 16-byte
// module, the 36-byte as_object, the 56-byte as_function, the 36-byte tu_file, the 1120-byte player
// - all of them. The rebuilt plugin preserves paired allocation/free through these hooks, using
// the shared native CRT in Code/RendDX9Abi/BfMemory.h.
//
// [CHOSEN, not recovered] Retail heap routing and named allocation buckets are retired under
// https://github.com/B2BF/OpenBF2/issues/7. Host and plugin allocations now use the same /MD CRT.
//
// =================================================================================================
// WHY THE FOUR LINK SWITCHES ARE 0
//
//   ZLIB    0  - ATTESTED, and RE-CONFIRMED 2026-09-01 by a controlled string scan of the
//                shipped DLL: BF2.exe carries zlib inflate strings (positive control) while
//                SwiffPlayer.dll carries NONE of 8 probes -> zlib is not linked at all. The
//                zipped-SWF "TU_CONFIG_LINK_TO_ZLIB is 0" string is present; all three
//                IMAGE/jpeg error strings are absent - which stock gameswf cannot produce
//                with ZLIB 0, so DICE REPLACED the embedded-image decode rather than
//                toggling it (the DLL never owns pixels; images resolve by name through
//                ISwiffRender::loadTexture). DO NOT turn this on. See RW-85.
//   JPEGLIB 0  - ATTESTED 2026-09-01 (was [UNRESOLVED: swf-jpeg-static-link]): the shipped
//                DLL has no "Independent JPEG Group" banner and no "not linked to jpeglib"
//                string. define_bits_jpeg tag-loader traces DO survive while
//                define_bits_lossless does not - same conclusion as ZLIB. Keep at 0.
//   LIBPNG  0  - no libpng vendored; external .png refs are resolved by the render side, not decoded.
//   LIBXML  0  - [UNRESOLVED: swf-xml-static-link], leaning gameswf-internal: the lone XML namespace
//                string is referenced from gameswf's own XML region, and libxml2 is not a dependent.
//
// =================================================================================================

#ifndef BF2_EXTERNAL_COMPATIBILITY_INCLUDE_H
#define BF2_EXTERNAL_COMPATIBILITY_INCLUDE_H

// -------------------------------------------------------------------------------------------------
// Optional-external-library switches. Set BEFORE tu_config.h reads its #ifndef defaults. This drop
// is only ever built for the SwiffPlayer image, so these are unconditional (not image-gated).
// -------------------------------------------------------------------------------------------------
#define TU_CONFIG_LINK_TO_ZLIB    0
#define TU_CONFIG_LINK_TO_JPEGLIB 0
#define TU_CONFIG_LINK_TO_LIBPNG  0
#define TU_CONFIG_LINK_TO_LIBXML  0

#if defined(BF2_IMAGE_SWIFFPLAYER)

// -------------------------------------------------------------------------------------------------
// The engine-heap routing. Declarations only; the bodies live in the SwiffPlayer image's own memory
// TU (Stage 2) and forward to dice::hfe::memory::bf_malloc / bf_free with the "externDll" bucket id.
// tu_realloc is bf_malloc + memcpy(min) + bf_free, because the shipped image imports bf_malloc and
// bf_free but NO bf_realloc - that is how DICE must have grown a block, and old_size (which
// tu_realloc is handed) is what makes it possible.
// -------------------------------------------------------------------------------------------------
namespace swiffplayer {
namespace mem {

void* tuMalloc(unsigned int size);
void* tuRealloc(void* oldPtr, unsigned int newSize, unsigned int oldSize);
void  tuFree(void* oldPtr);

// tu_error_exit: fatal, must not return. Routes to the image's frontier log + abort rather than the
// upstream default (fprintf + exit(code)), so an early-boot gameswf abort is captured in the
// SwiffPlayer log like every other frontier.
void  tuErrorExit(int errorCode, const char* errorMessage);

} // namespace mem
} // namespace swiffplayer

#define tu_malloc(size)                          (::swiffplayer::mem::tuMalloc((unsigned int)(size)))
#define tu_realloc(old_ptr, new_size, old_size)  (::swiffplayer::mem::tuRealloc((old_ptr), (unsigned int)(new_size), (unsigned int)(old_size)))
#define tu_free(old_ptr, old_size)               (::swiffplayer::mem::tuFree((old_ptr)))
#define tu_error_exit(error_code, error_message) (::swiffplayer::mem::tuErrorExit((error_code), (error_message)))

#else // no image define

// Bare compile (e.g. a stray syntax check). Route to the CRT so the drop still builds without pulling
// base/dlmalloc.c into any manifest. The SwiffPlayer image never takes this branch.
#include <cstdlib>
#include <cstring>

#define tu_malloc(size)                          (std::malloc((size)))
#define tu_realloc(old_ptr, new_size, old_size)  (std::realloc((old_ptr), (new_size)))
#define tu_free(old_ptr, old_size)               (std::free((old_ptr)))
// tu_error_exit is left to tu_config.h's own fprintf+exit default in this branch.

#endif // BF2_IMAGE_SWIFFPLAYER

#endif // BF2_EXTERNAL_COMPATIBILITY_INCLUDE_H
