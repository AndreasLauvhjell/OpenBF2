// Editor build contract; include first in every editor translation unit. Editor-only
// code uses new designs while shared game code retains its compatibility requirements.
#ifndef BF2EDITOR_FRAMEWORK_EDITORCONFIG_H
#define BF2EDITOR_FRAMEWORK_EDITORCONFIG_H

// Code/BF2/Core/BuildConfig.h is force-included by the build and already rejects a missing dedicated.
#if DEDICATED
#  error "BF2Editor is a client-side image: compile it with /DDEDICATED=0 (Code/Tools/BF2Editor/CMakeLists.txt)."
#endif

#if !defined(BF2_IMAGE_EDITOR) || !BF2_IMAGE_EDITOR
#  error "BF2Editor sources are compiled with BF2_IMAGE_EDITOR=1 (Code/Tools/BF2Editor/CMakeLists.txt)."
#endif

// The editor links the tree's x86 client object set, so it is an x86 Windows image for now.
#if !defined(_WIN32) || defined(_WIN64)
#  error "BF2Editor builds for Windows x86 only (docs/building.md)."
#endif

#endif // BF2EDITOR_FRAMEWORK_EDITORCONFIG_H
