@echo off
setlocal
rem [TEMP] Strict Object/descriptor fixture, then bounded diagnostic registry fixture.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_registry_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%F in (Object TypeDescriptor NamedObjectRegistry System) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /d1reportSingleClassLayoutRef2System /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\Ref2System.cpp" /Fo"%BF2_BUILD_OUT%\Ref2System.obj" >"%BF2_BUILD_OUT%\ref2system_layout.txt" 2>&1
if errorlevel 1 type "%BF2_BUILD_OUT%\ref2system_layout.txt"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_registry_runtime.cpp" /Fo"%BF2_BUILD_OUT%\foundation_fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\foundation.exe" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\foundation_fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\foundation.exe"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /DTEST_REGISTRY=1 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_registry_runtime.cpp" /Fo"%BF2_BUILD_OUT%\registry_fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\registry.exe" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\registry_fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\registry.exe"
exit /b %errorlevel%
