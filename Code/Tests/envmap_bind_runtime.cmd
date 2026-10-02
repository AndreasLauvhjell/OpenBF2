@echo off
setlocal
rem [TEMP] x86 engine client fixture, actual extracted guarded bind and borrowed string carrier.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_OUT=%~dp0..\Build\bundled_height_20260923\bind"
if not exist "%TASK_OUT%" mkdir "%TASK_OUT%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0extract_envmap_bind.ps1" -Output "%TASK_OUT%\envmap_bind_body.inc"
if errorlevel 1 exit /b 1
cl /nologo /MD /std:c++14 /permissive /EHsc /Gy /O2 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 /FI"%~dp0..\BF2\Core\BuildConfig.h" /I"%TASK_OUT%" "%~dp0envmap_bind_runtime.cpp" /Fo"%TASK_OUT%\fixture.obj" /Fe"%TASK_OUT%\fixture.exe" /link /OPT:REF /INCREMENTAL:NO >"%TASK_OUT%\build.log" 2>&1
if errorlevel 1 (
 type "%TASK_OUT%\build.log"
 exit /b 1
)
"%TASK_OUT%\fixture.exe"
