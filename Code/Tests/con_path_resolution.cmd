@echo off
rem ==============================================================================================
rem  con_path_resolution.cmd - validate OldConsole's script path resolution against REAL content.
rem
rem  This is not a compile gate. It is the external oracle: the rule recovered from
rem  ELF 0x844E9E0 (include) and 0x844D980 (run) is replayed over every `include`/`run` statement
rem  in the shipped BF2 corpus and each result is checked against the files that actually exist,
rem  loose and inside the four archives ServerArchives.con mounts.
rem
rem  It exists because this subsystem is the ONE part of the reconstruction with evidence outside
rem  the two binaries, and because that evidence has already caught real defects - including the
rem  missing m_useRelativePaths block in run(), which this script scores at 1 of 469.
rem
rem  Usage:  con_path_resolution.cmd ["path\to\mods\bf2"]
rem  Default corpus path, and BF2_CONTENT_ROOT, are documented in the .py.
rem ==============================================================================================
setlocal
python "%~dp0con_path_resolution.py" %1
endlocal
