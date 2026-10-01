@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Actual Node/Picture prefix and real TransformNode constructor/metadata.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_node_pointer_continue_20260906\runtime_o2"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0
set "TASK_SOURCES=Object TypeDescriptor Stream OStream Node NodeState InputNodeVisitors PictureNode TransformNode Effect Quad System Pointer EventQueue NamedObjectRegistry"
type nul >"%BF2_BUILD_OUT%\compile.log"
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (%TASK_SOURCES%) do (
 cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\External\BFMeme\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
 if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
 echo "%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0node_picture_prefix_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\node_picture_prefix_runtime.exe" /MAP:"%BF2_BUILD_OUT%\fixture.map" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\node_picture_prefix_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
