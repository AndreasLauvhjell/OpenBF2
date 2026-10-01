@echo off
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
cl /nologo /c /MD /std:c++14 /EHsc /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%~dp0obj\bootmain_test.obj" "%~dp0bootmain.cpp"
set RC=%errorlevel%

rem Delete it (2026-08-20): both link steps glob obj\*.obj, so a leftover
rem bootmain_test.obj is a SECOND main() in the linked image. This is a compile check,
rem not a build.
del /q "%~dp0obj\bootmain_test.obj" >nul 2>&1
exit /b %RC%
