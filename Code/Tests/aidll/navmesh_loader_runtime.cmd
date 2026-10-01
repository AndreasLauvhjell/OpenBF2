@echo off
setlocal
rem [TEMP] The navmesh loader: whole production loader/query TUs with the real Debug, strict x86
rem link, driven over every stock level's .qtr/.clb pair (extracted read-only from each server.zip)
rem and the synthetic version / island-count cases.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_NAVMESH_LOADER_OUT set "BF2_NAVMESH_LOADER_OUT=%TASK_CODE%\Build\claude_ai_c04nav_20260929\navmesh_loader"
if not exist "%BF2_NAVMESH_LOADER_OUT%" mkdir "%BF2_NAVMESH_LOADER_OUT%"
python -I "%~dp0navmesh_loader_runtime.py" extract "%BF2_NAVMESH_LOADER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
type nul >"%BF2_NAVMESH_LOADER_OUT%\compile.log"
for %%F in (NavMesh\QuadTreeManager NavMesh\QuadTree NavMesh\QuadTreeNode Cluster\ClusterGraph Material\MaterialManager) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\%%F.cpp" /Fo"%BF2_NAVMESH_LOADER_OUT%\%%~nF.obj" >>"%BF2_NAVMESH_LOADER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (Mat4Inverse Plane3 Vec3) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Math\%%F.cpp" /Fo"%BF2_NAVMESH_LOADER_OUT%\%%F.obj" >>"%BF2_NAVMESH_LOADER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (Debug StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Core\%%F.cpp" /Fo"%BF2_NAVMESH_LOADER_OUT%\%%F.obj" >>"%BF2_NAVMESH_LOADER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0navmesh_loader_runtime.cpp" /Fo"%BF2_NAVMESH_LOADER_OUT%\fixture.obj" >>"%BF2_NAVMESH_LOADER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
set "O=%BF2_NAVMESH_LOADER_OUT%"
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\navmesh_loader_runtime.exe" "%O%\QuadTreeManager.obj" "%O%\QuadTree.obj" "%O%\QuadTreeNode.obj" "%O%\ClusterGraph.obj" "%O%\MaterialManager.obj" "%O%\Mat4Inverse.obj" "%O%\Plane3.obj" "%O%\Vec3.obj" "%O%\Debug.obj" "%O%\StringUtil.obj" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0navmesh_loader_runtime.py" run "%BF2_NAVMESH_LOADER_OUT%\navmesh_loader_runtime.exe" "%BF2_NAVMESH_LOADER_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_NAVMESH_LOADER_OUT%\compile.log"
if exist "%BF2_NAVMESH_LOADER_OUT%\link.log" type "%BF2_NAVMESH_LOADER_OUT%\link.log"
exit /b 1
