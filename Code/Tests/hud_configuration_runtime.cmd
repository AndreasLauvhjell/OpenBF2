@echo off
setlocal
rem [TEMP] Focused real HUD configuration dispatch; unrelated /FORCE symbols uncalled.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_atlas_20260906\configuration_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Od /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 %HUD_CONFIGURATION_EXTRA_FLAGS%"
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerConfiguration Menu\HudBlinker Menu\HudRadio Menu\Ref2System External\BFMeme\System External\BFMeme\OStream External\BFMeme\Stream External\BFMeme\Object External\BFMeme\TypeDescriptor) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_configuration_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_configuration_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_configuration_runtime.pdb" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\BF2HudManagerConfiguration.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\HudRadio.obj" "%BF2_BUILD_OUT%\Ref2System.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\hud_configuration_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RUNTIME_RESULT%
