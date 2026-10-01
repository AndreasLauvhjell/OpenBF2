@echo off
setlocal
rem [TEMP] Actual enablePointer, native collaborators and explicit picture-dispatch observer.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_pointer_final_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
type nul >"%BF2_BUILD_OUT%\compile.log"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" %BF2_TEST_OPT%"
set "TASK_OBJECTS="
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerPointer Menu\HudAmmo Menu\HudBlinker Menu\Ref2System Core\ClassManager Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\System External\BFMeme\Stream External\BFMeme\OStream External\BFMeme\Node External\BFMeme\Pointer Math\Vec3 Math\Mat4) do (
  for %%D in ("%BF2_BUILD_OUT%\%%F.obj") do if not exist "%%~dpD" mkdir "%%~dpD"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%F.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_enable_pointer_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_enable_pointer_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_enable_pointer_runtime.pdb" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_enable_pointer_runtime.exe" exit /b 1
"%BF2_BUILD_OUT%\hud_enable_pointer_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
