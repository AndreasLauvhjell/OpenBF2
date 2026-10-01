// [TEMP] [CHOSEN, not recovered] - SCAFFOLDING. Shared declarations of the OpenAL logging shim.
#ifndef BF2AUDIO_DIFF_ALSHIM_H
#define BF2AUDIO_DIFF_ALSHIM_H

// Called by every export before it touches the real library: loads wrap_oal_real.dll (from
// BF2_ALSHIM_REAL, or beside the shim) and opens the log (BF2_ALSHIM_LOG). cdecl, no arguments,
// so the naked trampolines can `call` it without disturbing their own argument frames.
extern "C" void __cdecl ensureLoaded();

// Generated: resolves the pass-through pointers from the real library.
void resolveTrampolines(void* (*resolve)(const char*));

#endif
