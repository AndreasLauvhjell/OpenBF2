@echo off
setlocal
rem [TEMP] Force-linked production-path aircraft readback. Expects an isolated full build.
if not defined BF2_BUILD_OUT exit /b 2
if not defined BF2_AIRCRAFT_FIXTURE_DIR (
  echo Set BF2_AIRCRAFT_FIXTURE_DIR to the observer input directory, or use tier2\run.py.
  exit /b 2
)
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "ROOT=%~dp0..\.."
set "OUT=%BF2_AIRCRAFT_FIXTURE_DIR%"
if not exist "%OUT%\bootmain.cpp" exit /b 2
if not exist "%OUT%\stock_expectations.tsv" exit /b 2
if not exist "%OUT%\runtime" mkdir "%OUT%\runtime"
cl /nologo /c /MD /Z7 /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%ROOT%\Code\BF2\Core\BuildConfig.h" /Fo"%OUT%\bootmain.obj" "%OUT%\bootmain.cpp" > "%OUT%\bootcompile.log" 2>&1
if errorlevel 1 (type "%OUT%\bootcompile.log" & exit /b 1)
cl /nologo /c /MD /Z7 /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%ROOT%\Code\BF2\Core\BuildConfig.h" /Fo"%OUT%\observer.obj" "%~dp0aircraft_runtime.cpp" > "%OUT%\observercompile.log" 2>&1
if errorlevel 1 (type "%OUT%\observercompile.log" & exit /b 1)
link /nologo /FORCE /DEBUG /MAP:"%OUT%\runtime.map" /PDB:"%OUT%\runtime.pdb" /IMPLIB:"%OUT%\runtime.lib" /out:"%OUT%\runtime\bf2_w32ded.exe" /subsystem:console /NODEFAULTLIB:python23.lib "%OUT%\bootmain.obj" "%OUT%\observer.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" "%ROOT%\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib > "%OUT%\link.log" 2>&1
if not exist "%OUT%\runtime\bf2_w32ded.exe" exit /b 1
copy /y "%ROOT%\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.dll" "%OUT%\runtime\dice_py.dll" >nul
exit /b 0
