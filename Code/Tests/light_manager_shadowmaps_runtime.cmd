@echo off
setlocal
rem [TEMP] Actual x86 RendDX9 LightManager loader/samplers with narrow PE host-service records.
rem Strict link. No retail DLL, filesystem level content, or game process is loaded.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hemi_draw_20260922\loader_tests"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /c /EHsc /O2 /std:c++14 /permissive /Gy /MD /DDEDICATED=0 /DBF2_IMAGE_RENDDX9 /DBF2_PLUGIN_ABI_NATIVE=0 /D_ITERATOR_DEBUG_LEVEL=0"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" "%~dp0light_manager_shadowmaps_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\build.log" 2>&1
if errorlevel 1 goto failed
for %%F in (Frustum LocalBoundingBox Mat4 Plane3 Quat Vec3) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" "%TASK_ROOT%\BF2\Math\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\build.log" 2>&1
    if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" "%TASK_ROOT%\RendDX9Abi\Vc71String.cpp" /Fo"%BF2_BUILD_OUT%\Vc71String.obj" >>"%BF2_BUILD_OUT%\build.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\light_manager_shadowmaps_runtime.exe" "%BF2_BUILD_OUT%\fixture.obj" "%BF2_BUILD_OUT%\Frustum.obj" "%BF2_BUILD_OUT%\LocalBoundingBox.obj" "%BF2_BUILD_OUT%\Mat4.obj" "%BF2_BUILD_OUT%\Plane3.obj" "%BF2_BUILD_OUT%\Quat.obj" "%BF2_BUILD_OUT%\Vec3.obj" "%BF2_BUILD_OUT%\Vc71String.obj" user32.lib >>"%BF2_BUILD_OUT%\build.log" 2>&1
if errorlevel 1 goto failed
"%BF2_BUILD_OUT%\light_manager_shadowmaps_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_BUILD_OUT%\build.log"
exit /b 1
