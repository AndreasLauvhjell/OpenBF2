@echo off
call "%~dp0..\vsdevcmd.cmd" x86
pushd "%~dp0"
cl /nologo /std:c++14 /permissive /EHsc /wd4996 /DDEDICATED=1 /FI"%~dp0..\..\BF2\Core\BuildConfig.h" /I"%~dp0..\..\BF2\IO" boot.cpp "%~dp0..\..\BF2\IO\BitStream.cpp" /Fe:boot.exe
if errorlevel 1 ( popd & exit /b 1 )
"%~dp0boot.exe"
set RC=%errorlevel%
popd
exit /b %RC%
