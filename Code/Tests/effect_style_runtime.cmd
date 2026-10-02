@echo off
setlocal
rem [TEMP] Native x86 client Effect/Style fixture. Uncalled link frontiers are recorded.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_effect_style_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Od /fp:precise /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%F in (External\BFMeme\Effect External\BFMeme\Style External\BFMeme\Quad External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\System External\BFMeme\Stream Menu\Font Menu\DifFontMetadata) do (
    cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0effect_style_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\effect_style_runtime.exe" /PDB:"%BF2_BUILD_OUT%\effect_style_runtime.pdb" "%BF2_BUILD_OUT%\Effect.obj" "%BF2_BUILD_OUT%\Style.obj" "%BF2_BUILD_OUT%\Quad.obj" "%BF2_BUILD_OUT%\Data.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Font.obj" "%BF2_BUILD_OUT%\DifFontMetadata.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\effect_style_runtime.exe" >"%BF2_BUILD_OUT%\runtime_force.log" 2>&1
set "TASK_RUNTIME_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime_force.log"
exit /b %TASK_RUNTIME_RESULT%
