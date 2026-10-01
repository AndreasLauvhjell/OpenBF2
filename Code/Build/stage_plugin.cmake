# [CHOSEN] Build-time plugin staging for the diagnostic client (2026-09-08, at the owner's request).
# Copies one of the tree's own plugin builds - and its .pdb when one sits beside it - next to the
# built BF2.exe, so that LoadLibrary resolves the DLL from the application directory before the
# game directory's retail copy. The game directory is never written.
#
# Invoked by bf2_client_plugins, an always-run dependency of bf2_client in the root CMakeLists.txt:
#   cmake -DBF2_PLUGIN_SRC=<dll> -DBF2_PLUGIN_DST_DIR=<bin_client> [-DBF2_PLUGIN_HINT=<how to build it>]
#         -P Code/Build/stage_plugin.cmake
# A plugin that has not been built yet is a note, not an error: each plugin is built by its own
# script (renddx9.cmd, swiffplayer.cmd), not by this tree, and no reconfigure is needed after
# building one - the next client build stages it, even if BF2.exe needs no relink.
if(NOT BF2_PLUGIN_SRC OR NOT BF2_PLUGIN_DST_DIR)
    message(FATAL_ERROR "stage_plugin.cmake: BF2_PLUGIN_SRC and BF2_PLUGIN_DST_DIR are required")
endif()
get_filename_component(_plugin_name "${BF2_PLUGIN_SRC}" NAME)
get_filename_component(_plugin_stem "${BF2_PLUGIN_SRC}" NAME_WE)
get_filename_component(_plugin_dir  "${BF2_PLUGIN_SRC}" DIRECTORY)
if(NOT EXISTS "${BF2_PLUGIN_SRC}")
    if(BF2_PLUGIN_HINT)
        message(STATUS "bf2_client: ${_plugin_name} not staged - ${BF2_PLUGIN_SRC} does not exist (${BF2_PLUGIN_HINT})")
    else()
        message(STATUS "bf2_client: ${_plugin_name} not staged - ${BF2_PLUGIN_SRC} does not exist")
    endif()
    return()
endif()
file(MAKE_DIRECTORY "${BF2_PLUGIN_DST_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BF2_PLUGIN_SRC}" "${BF2_PLUGIN_DST_DIR}/${_plugin_name}"
    RESULT_VARIABLE _plugin_rc
)
if(NOT _plugin_rc EQUAL 0)
    message(FATAL_ERROR "bf2_client: staging ${_plugin_name} into ${BF2_PLUGIN_DST_DIR} failed (${_plugin_rc})")
endif()
if(EXISTS "${_plugin_dir}/${_plugin_stem}.pdb")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_plugin_dir}/${_plugin_stem}.pdb" "${BF2_PLUGIN_DST_DIR}/${_plugin_stem}.pdb"
        RESULT_VARIABLE _plugin_pdb_rc
    )
    if(NOT _plugin_pdb_rc EQUAL 0)
        message(FATAL_ERROR "bf2_client: staging ${_plugin_stem}.pdb into ${BF2_PLUGIN_DST_DIR} failed (${_plugin_pdb_rc})")
    endif()
endif()
message(STATUS "bf2_client: staged ${_plugin_name} from ${_plugin_dir}")
