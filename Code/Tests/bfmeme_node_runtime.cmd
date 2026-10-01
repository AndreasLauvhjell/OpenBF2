@echo off
setlocal
rem [TEMP] Actual Node prefix and three-slot visitor; uncalled /FORCE frontiers.
rem
rem 2026-09-16: dispatchNodeEvent20 now dispatches to Bf2ObjectMarkerNode::clearShowFlagsForHud
rem and Bf2ButtonNode::releasePointerForHud, so Node.obj carries a link edge into Menu/ and
rem this narrow link reports FOUR new LNK2019 lines for those two classes' symbols. THAT IS
rem EXPECTED, not a regression: this fixture drives event types 1-6 and the generic set only -
rem never 20 - so the seam is uncalled here and the symbols fall under the /FORCE contract
rem this suite already states. Adding the two Menu TUs was TRIED and reverted: they drag in
rem Bf2PictureNode, formatString and more, trading four unresolved symbols for a larger set
rem without exercising anything new. Every REAL image links both TUs - syntaxcheck.cmd and
rem CMakeLists.txt each list Node.cpp and Bf2ButtonNode.cpp, and build.cmd enumerates
rem recursively - so the edge is satisfied where it matters.
rem If this fixture is ever made to drive event 20, add those TUs and follow the cascade.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_node_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (Object TypeDescriptor Stream Node) do (
  cl %TASK_FLAGS% /d1reportSingleClassLayoutNode /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /d1reportSingleClassLayoutNodeVisitor /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_node_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\bfmeme_node_runtime.exe" /PDB:"%BF2_BUILD_OUT%\bfmeme_node_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\bfmeme_node_runtime.map" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\bfmeme_node_runtime.exe"
exit /b %errorlevel%
