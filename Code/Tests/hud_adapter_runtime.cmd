@echo off
setlocal
rem [TEMP] Isolated client x86 fixture; named unexercised frontiers remain unresolved.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_adapter_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
rem Registry entry visitors now instantiate the real DeletionStream family.
for %%F in (Menu\HudInformationAdapter Menu\BF2HudManager Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\System External\BFMeme\NamedObjectRegistry External\BFMeme\Stream) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_adapter_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_adapter_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_adapter_runtime.pdb" "%BF2_BUILD_OUT%\HudInformationAdapter.obj" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\HudInformationLayer.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\EventManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_adapter_runtime.exe"
exit /b %errorlevel%
