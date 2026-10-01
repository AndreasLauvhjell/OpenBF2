@echo off
setlocal
rem [TEMP] Strict x86 polygon query fixture; no server and no FORCE link.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_MAP_POLYGON_OUT set "BF2_MAP_POLYGON_OUT=%TASK_CODE%\Build\ai_map_query_20261001\validation_audit\map_polygon"
if not exist "%BF2_MAP_POLYGON_OUT%" mkdir "%BF2_MAP_POLYGON_OUT%"
python -I "%~dp0map_polygon_runtime.py" generate "%BF2_MAP_POLYGON_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
type nul >"%BF2_MAP_POLYGON_OUT%\compile.log"
for %%F in (NavMesh\QuadTreeManager NavMesh\QuadTree NavMesh\QuadTreeNode NavMesh\MapProperties Cluster\ClusterGraph Material\MaterialManager) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\%%F.cpp" /Fo"%BF2_MAP_POLYGON_OUT%\%%~nF.obj" >>"%BF2_MAP_POLYGON_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (Core\Debug Core\StringUtil Math\Mat4Inverse Math\Plane3 Math\Vec3 AI\AIDLL\AI\Main\IAIPathfinding) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\%%F.cpp" /Fo"%BF2_MAP_POLYGON_OUT%\%%~nF.obj" >>"%BF2_MAP_POLYGON_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_MAP_POLYGON_OUT%\actual.cpp" /Fo"%BF2_MAP_POLYGON_OUT%\actual.obj" >>"%BF2_MAP_POLYGON_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_MAP_POLYGON_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0map_polygon_runtime.cpp" /Fo"%BF2_MAP_POLYGON_OUT%\fixture.obj" >>"%BF2_MAP_POLYGON_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
set "O=%BF2_MAP_POLYGON_OUT%"
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\map_polygon_runtime.exe" "%O%\actual.obj" "%O%\fixture.obj" "%O%\QuadTreeManager.obj" "%O%\QuadTree.obj" "%O%\QuadTreeNode.obj" "%O%\MapProperties.obj" "%O%\ClusterGraph.obj" "%O%\MaterialManager.obj" "%O%\IAIPathfinding.obj" "%O%\Mat4Inverse.obj" "%O%\Plane3.obj" "%O%\Vec3.obj" "%O%\Debug.obj" "%O%\StringUtil.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0map_polygon_runtime.py" run "%O%\map_polygon_runtime.exe" "%O%"
exit /b %errorlevel%
:failed
type "%BF2_MAP_POLYGON_OUT%\compile.log"
if exist "%BF2_MAP_POLYGON_OUT%\link.log" type "%BF2_MAP_POLYGON_OUT%\link.log"
exit /b 1
