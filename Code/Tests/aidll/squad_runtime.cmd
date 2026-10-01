@echo off
setlocal
rem [TEMP] [CHOSEN, not recovered] Whole production member/formation TUs; strict x86 link.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SQUAD_OUT set "BF2_SQUAD_OUT=%TASK_CODE%\Build\ai_squad_20260927\member_runtime"
if not exist "%BF2_SQUAD_OUT%" mkdir "%BF2_SQUAD_OUT%"
python "%~dp0squad_runtime.py" generate "%BF2_SQUAD_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIBot\SquadMember.cpp" /Fo"%BF2_SQUAD_OUT%\SquadMember.obj" >"%BF2_SQUAD_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIBot\SquadFormation.cpp" /Fo"%BF2_SQUAD_OUT%\SquadFormation.obj" >>"%BF2_SQUAD_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_SQUAD_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0squad_runtime.cpp" /Fo"%BF2_SQUAD_OUT%\fixture.obj" >>"%BF2_SQUAD_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SQUAD_OUT%\squad_runtime.exe" "%BF2_SQUAD_OUT%\SquadMember.obj" "%BF2_SQUAD_OUT%\SquadFormation.obj" "%BF2_SQUAD_OUT%\fixture.obj" >"%BF2_SQUAD_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python "%~dp0squad_runtime.py" run "%BF2_SQUAD_OUT%\squad_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_SQUAD_OUT%\compile.log"
if exist "%BF2_SQUAD_OUT%\link.log" type "%BF2_SQUAD_OUT%\link.log"
exit /b 1
