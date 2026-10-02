@echo off
setlocal
rem [TEMP] Native client map/header compile. Never constructs or publishes map.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_map_owner_20260906\compile1"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
cl /nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /d1reportSingleClassLayoutBf2MenuMapNode /c "%TASK_ROOT%\Code\BF2\Menu\Bf2MenuMapNode.cpp" /Fo"%BF2_BUILD_OUT%\Bf2MenuMapNode.obj" >"%BF2_BUILD_OUT%\compile.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\compile.log"
exit /b %TASK_RESULT%
