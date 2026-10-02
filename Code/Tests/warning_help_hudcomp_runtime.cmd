@echo off
setlocal
rem [TEMP] Focused x86 client fixture; not part of a BF2 image.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
set "TASK_OUT=%~dp0..\Build\warning_help_hudcomp_runtime"
if not exist "%TASK_OUT%" mkdir "%TASK_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Od /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Core\BuildConfig.h""
cl %TASK_FLAGS% "%~dp0warning_help_hudcomp_runtime.cpp" "%TASK_ROOT%\Game\Objects\WarningHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\HelpHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\IObjectComponent.cpp" /Fe"%TASK_OUT%\warning_help_hudcomp_runtime.exe" /Fo"%TASK_OUT%\\" /Fd"%TASK_OUT%\warning_help_hudcomp_runtime.pdb"
if errorlevel 1 exit /b 1
"%TASK_OUT%\warning_help_hudcomp_runtime.exe"
exit /b %errorlevel%
