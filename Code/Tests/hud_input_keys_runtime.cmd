@echo off
setlocal
rem [TEMP] x86 actual input parsers; optional HUD plus fixture binding source.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_input_keys_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (IO\Input\InputMapStream Core\StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /DINPUT_STREAM_ONLY /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_input_keys_runtime.cpp" /Fo"%BF2_BUILD_OUT%\streams.obj"
if errorlevel 1 exit /b 1
link /nologo /INCREMENTAL:NO /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\streams.exe" "%BF2_BUILD_OUT%\streams.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\InputMapStream.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\streams.exe"
if errorlevel 1 exit /b 1
for %%s in (Menu\BF2HudManager Menu\HudBlinker IO\Localization\ExcelLexicon) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_input_keys_runtime.cpp" /Fo"%BF2_BUILD_OUT%\keys.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\keys.exe" "%BF2_BUILD_OUT%\keys.obj" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\InputMapStream.obj" "%BF2_BUILD_OUT%\ExcelLexicon.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\keys.exe"
exit /b %errorlevel%
