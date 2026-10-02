@echo off
setlocal
rem [TEMP][CHOSEN, not recovered] Actual renderer-view runtime, x86, no D3D device.
rem Usage: render_view_runtime.cmd [native|compat] [O2|Od]
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
set "TASK_ABI=native"
set "TASK_OPT=O2"
if /i "%~1"=="compat" set "TASK_ABI=compat"
if /i "%~2"=="Od" set "TASK_OPT=Od"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_render_view_20260906\runtime_%TASK_ABI%_%TASK_OPT%"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_CRT=/MD /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
set "TASK_MEMORY="
if "%TASK_ABI%"=="compat" (
 set "TASK_CRT=/MT /DBF2_PLUGIN_ABI_NATIVE=0"
 set "TASK_MEMORY=%BF2_BUILD_OUT%\Memory.lib"
 lib /nologo /def:"%TASK_ROOT%\Code\RendDX9Abi\Memory.def" /machine:x86 /out:"%BF2_BUILD_OUT%\Memory.lib" >"%BF2_BUILD_OUT%\imports.log" 2>&1
 if errorlevel 1 exit /b 1
)
lib /nologo /def:"%TASK_ROOT%\Code\RendDX9Abi\D3dx9_25.def" /machine:x86 /out:"%BF2_BUILD_OUT%\d3dx9_25.lib" >>"%BF2_BUILD_OUT%\imports.log" 2>&1
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /%TASK_OPT% /EHsc /std:c++14 /permissive %TASK_CRT% /Z7 /Gy /W4 /DDEDICATED=0 /DBF2_IMAGE_RENDDX9 /D_CRT_SECURE_NO_WARNINGS"
for %%F in (BF2\RendMisc\RenderView BF2\Math\Frustum BF2\Math\Plane3 RendDX9Abi\BfMemory RendDX9Abi\Frontier Tests\render_view_runtime) do (
 cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
 if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\render_view_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" "%BF2_BUILD_OUT%\*.obj" "%BF2_BUILD_OUT%\d3dx9_25.lib" %TASK_MEMORY% >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
rem Read-only system DLL resolution; the reference game directory is never written.
set "PATH=D:\B2BF Launcher\Battlefield2;%PATH%"
"%BF2_BUILD_OUT%\render_view_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
