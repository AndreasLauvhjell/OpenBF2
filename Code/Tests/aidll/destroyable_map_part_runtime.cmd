@echo off
setlocal
rem [TEMP] Strict MSVC x86 destroyable-map-part fixture; no server and no FORCE link.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1

set "TASK_CODE=%~dp0..\.."
if not defined BF2_DESTROYABLE_OUT set "BF2_DESTROYABLE_OUT=%TASK_CODE%\Build\ai_map_query_20261001\destroyable_validation"
if not exist "%BF2_DESTROYABLE_OUT%" mkdir "%BF2_DESTROYABLE_OUT%"

python -I "%~dp0destroyable_map_part_runtime.py" preflight "%BF2_DESTROYABLE_OUT%"
if errorlevel 1 exit /b 1

set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL=1"
type nul >"%BF2_DESTROYABLE_OUT%\compile.log"
cl %TASK_FLAGS% /W3 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\NavMesh\QuadTree.cpp" /Fo"%BF2_DESTROYABLE_OUT%\QuadTree.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /W3 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\NavMesh\QuadTreeManager.cpp" /Fo"%BF2_DESTROYABLE_OUT%\QuadTreeManager.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /W3 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Math\Mat4Inverse.cpp" /Fo"%BF2_DESTROYABLE_OUT%\Mat4Inverse.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
for %%F in (NavMesh\QuadTreeNode Cluster\ClusterGraph Material\MaterialManager) do (
  cl %TASK_FLAGS% /W3 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\%%F.cpp" /Fo"%BF2_DESTROYABLE_OUT%\%%~nF.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (Core\Debug Core\StringUtil Math\Plane3 Math\Vec3) do (
  cl %TASK_FLAGS% /W3 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\%%F.cpp" /Fo"%BF2_DESTROYABLE_OUT%\%%~nF.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /W4 /WX /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0destroyable_map_part_runtime.cpp" /Fo"%BF2_DESTROYABLE_OUT%\fixture.obj" >>"%BF2_DESTROYABLE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed

set "O=%BF2_DESTROYABLE_OUT%"
link /nologo /MACHINE:X86 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\destroyable_map_part_runtime.exe" "%O%\QuadTree.obj" "%O%\QuadTreeManager.obj" "%O%\Mat4Inverse.obj" "%O%\QuadTreeNode.obj" "%O%\ClusterGraph.obj" "%O%\MaterialManager.obj" "%O%\Debug.obj" "%O%\StringUtil.obj" "%O%\Plane3.obj" "%O%\Vec3.obj" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed

>"%O%\build.log" echo PASS real QuadTree.cpp, QuadTreeManager.cpp and Mat4Inverse.cpp compiled for AIDLL
>>"%O%\build.log" echo PASS strict MSVC x86 link without FORCE
python -I "%~dp0destroyable_map_part_runtime.py" run "%O%\destroyable_map_part_runtime.exe" "%O%"
exit /b %errorlevel%

:failed
if exist "%BF2_DESTROYABLE_OUT%\compile.log" type "%BF2_DESTROYABLE_OUT%\compile.log"
if exist "%BF2_DESTROYABLE_OUT%\link.log" type "%BF2_DESTROYABLE_OUT%\link.log"
exit /b 1
