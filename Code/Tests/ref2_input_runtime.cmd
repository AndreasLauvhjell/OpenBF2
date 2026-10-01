@echo off
setlocal
rem [TEMP] Real client Ref2 input/Pointer/EventQueue fixture, no D3D initialization.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\ref2_input_20260905_obj"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%F in (Object TypeDescriptor Stream OStream System Pointer DynamicIndexPointer EventQueue Node) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
    if errorlevel 1 exit /b 1
)
for %%F in (Mat4 Vec3) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Math\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\Ref2System.cpp" /Fo"%BF2_BUILD_OUT%\Ref2System.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0ref2_input_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
rem /FORCE is limited to uncalled type/stream/resource/render/Node visitor frontiers.
rem Every Ref2 preUpdate, Pointer dispatch and queue-allocation body exercised is real.
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\ref2_input_runtime.exe" /PDB:"%BF2_BUILD_OUT%\ref2_input_runtime.pdb" "%BF2_BUILD_OUT%\fixture.obj" "%BF2_BUILD_OUT%\Ref2System.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\Pointer.obj" "%BF2_BUILD_OUT%\DynamicIndexPointer.obj" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\Mat4.obj" "%BF2_BUILD_OUT%\Vec3.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\ref2_input_runtime.exe"
exit /b %errorlevel%
