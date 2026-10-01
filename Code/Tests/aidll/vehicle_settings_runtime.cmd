@echo off
setlocal
rem [TEMP] Selected exact AISettings definitions; whole MaterialManager/Debug/StringUtil. Strict x86.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_VEHICLE_SETTINGS_OUT set "BF2_VEHICLE_SETTINGS_OUT=%TASK_CODE%\Build\ai_vehicle_roster_20261001\validation"
if not exist "%BF2_VEHICLE_SETTINGS_OUT%" mkdir "%BF2_VEHICLE_SETTINGS_OUT%"
python -I "%~dp0vehicle_settings_runtime.py" generate "%BF2_VEHICLE_SETTINGS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
set "O=%BF2_VEHICLE_SETTINGS_OUT%"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%O%\actual.cpp" /Fo"%O%\actual.obj" >"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\Material\MaterialManager.cpp" /Fo"%O%\MaterialManager.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIPathFinding\Material\Material.cpp" /Fo"%O%\Material.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
for %%F in (Debug StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Core\%%F.cpp" /Fo"%O%\%%F.obj" >>"%O%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /I"%O%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0vehicle_settings_runtime.cpp" /Fo"%O%\fixture.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /OUT:"%O%\vehicle_settings_runtime.exe" "%O%\actual.obj" "%O%\MaterialManager.obj" "%O%\Material.obj" "%O%\Debug.obj" "%O%\StringUtil.obj" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0vehicle_settings_runtime.py" run "%O%\vehicle_settings_runtime.exe"
exit /b %errorlevel%
:failed
type "%O%\compile.log"
if exist "%O%\link.log" type "%O%\link.log"
exit /b 1
