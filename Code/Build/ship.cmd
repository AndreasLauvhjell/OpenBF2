@echo off
rem ===========================================================================
rem THE SHIPPABLE RELEASE BUILD of bf2_w32ded.exe - optimised, stripped, staged.
rem
rem   Code\Tests\syntaxcheck.cmd   parses only (/Zs)
rem   Code\Build\build.cmd         DIAGNOSTIC compile + link; its log is the work list
rem   Code\Build\bootrun.cmd       the bounded HARNESS image (bootmain.cpp, probes, fault handler)
rem   Code\Build\ship.cmd          THIS. The production entry point, /O2, no PDB, no map beside it.
rem
rem This is only the toolchain wrapper. Everything the build actually does - including deriving the
rem engine-image manifest from build.cmd rather than keeping a second copy of it - lives in
rem Code\Build\ship.ps1, and its header explains every flag that differs from build.cmd.
rem
rem   usage:  ship.cmd                      full build into Code\Build\bin_ship
rem           ship.cmd -SkipCompile         relink the existing objects only
rem ===========================================================================
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0ship.ps1" %*
exit /b %errorlevel%
