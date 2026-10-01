@echo off
setlocal EnableDelayedExpansion
rem [TEMP] The SwiffHost hosting fixture (swiff_host_server_runtime.cpp): Menu\SwiffHost.cpp's hostServer,
rem requestHostServer and getServerSettingsInfo with the REAL MapList, ServerSettings, settings repository
rem and ClassManager (its lookup observed). Linked /FORCE:UNRESOLVED: SwiffHost's other collaborators
rem stay unresolved and are never reached; the list of them is printed after the run. Client image
rem (DEDICATED=0). No network, no files.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
set "TASK_GS=%~dp0..\BF2\External\GameSpy"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\swiff_phase5_20260929\host_server_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_PY=%~dp0..\BF2\External\python-embedded\py-src-2.3.4"
set TASK_FLAGS=/nologo /O2 /EHsc /DPy_NO_ENABLE_SHARED /I"%TASK_PY%\Include" /I"%TASK_PY%\PC" /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /D_CRT_SECURE_NO_WARNINGS /FI"%TASK_ROOT%\Core\BuildConfig.h"
set TASK_CFLAGS=/nologo /O2 /MD /Z7 /W0 /DDEDICATED=0 /D_CRT_SECURE_NO_WARNINGS /FI"%TASK_ROOT%\Core\BuildConfig.h"
type nul >"%BF2_BUILD_OUT%\compile.log"
set TASK_OBJS=
for %%F in ("Menu\SwiffHost.cpp" "Game\MapList.cpp" "Game\GameServer\ServerSettings.cpp" "Core\SettingsRepostitory.cpp" "Core\ClassManager.cpp" "Core\Debug.cpp" "Core\StringUtil.cpp") do (
  cl %TASK_FLAGS% /c "%TASK_ROOT%\%%~F" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 goto :compilefail
  set TASK_OBJS=!TASK_OBJS! "%BF2_BUILD_OUT%\%%~nF.obj"
)
cl %TASK_FLAGS% /c "%~dp0swiff_host_server_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 goto :compilefail
rem The constructor registers the bf2_* query keys through qr2_register_key.
cl %TASK_CFLAGS% /c "%TASK_GS%\qr2\qr2regkeys.c" /Fo"%BF2_BUILD_OUT%\gs_qr2regkeys.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 goto :compilefail
set TASK_OBJS=!TASK_OBJS! "%BF2_BUILD_OUT%\gs_qr2regkeys.obj"
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\swiff_host_server_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" "%BF2_BUILD_OUT%\fixture.obj" %TASK_OBJS% ws2_32.lib advapi32.lib user32.lib shell32.lib winmm.lib kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
findstr /C:"unresolved external symbol" "%BF2_BUILD_OUT%\link.log" >"%BF2_BUILD_OUT%\unresolved.log"
if not exist "%BF2_BUILD_OUT%\swiff_host_server_runtime.exe" (type "%BF2_BUILD_OUT%\link.log" & goto :fail)
"%BF2_BUILD_OUT%\swiff_host_server_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
for /f %%N in ('find /c /v "" ^<"%BF2_BUILD_OUT%\unresolved.log"') do echo forced unresolved externals: %%N (listed in unresolved.log)
if not "%TASK_RESULT%"=="0" goto :fail
echo SWIFF-HOST-SERVER-RUNTIME OK
exit /b 0
:compilefail
type "%BF2_BUILD_OUT%\compile.log" | findstr /I /C:"error"
:fail
echo SWIFF-HOST-SERVER-RUNTIME FAILED
exit /b 1
