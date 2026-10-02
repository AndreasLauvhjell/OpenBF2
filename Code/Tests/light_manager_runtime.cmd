@echo off
setlocal
rem [TEMP] Focused x86 RendDX9 LightManager/XpLight runtime with PE-layout dependency records.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\light_manager_runtime_20260916"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"

cl /nologo /EHsc /O2 /std:c++14 /permissive /Gy /MD ^
  /DDEDICATED=0 /DBF2_IMAGE_RENDDX9 /DBF2_PLUGIN_ABI_NATIVE=0 /D_ITERATOR_DEBUG_LEVEL=0 ^
  /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" ^
  "%TASK_ROOT%\Tests\light_manager_runtime.cpp" ^
  "%TASK_ROOT%\BF2\Math\Frustum.cpp" ^
  "%TASK_ROOT%\BF2\Math\LocalBoundingBox.cpp" ^
  "%TASK_ROOT%\BF2\Math\Mat4.cpp" ^
  "%TASK_ROOT%\BF2\Math\Plane3.cpp" ^
  "%TASK_ROOT%\BF2\Math\Quat.cpp" ^
  "%TASK_ROOT%\BF2\Math\Vec3.cpp" ^
  "%TASK_ROOT%\RendDX9Abi\Vc71String.cpp" ^
  /Fe"%BF2_BUILD_OUT%\light_manager_runtime.exe" ^
  /link /INCREMENTAL:NO user32.lib >"%BF2_BUILD_OUT%\build.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\build.log"
  exit /b 1
)

"%BF2_BUILD_OUT%\light_manager_runtime.exe"
exit /b %errorlevel%
