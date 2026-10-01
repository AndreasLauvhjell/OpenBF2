@echo off
setlocal
rem [TEMP] Actual HUD/Ref2/Pointer input and snapshot-copy fixture.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_pointer_finish_20260906\input_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC""
set "TASK_SOURCES=Menu\BF2HudManager Menu\BF2HudManagerInput Menu\HudBlinker Menu\Ref2System External\BFMeme\Node External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\System External\BFMeme\Stream External\BFMeme\OStream External\BFMeme\Pointer External\BFMeme\DynamicIndexPointer External\BFMeme\EventQueue Math\Mat4 Math\Vec3 Core\StringUtil Core\Debug"
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%F in (%TASK_SOURCES%) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_game_input_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (%TASK_SOURCES%) do echo "%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_game_input_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\hud_game_input_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RUNTIME_RESULT%
