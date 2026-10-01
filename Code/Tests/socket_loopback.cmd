@rem SCAFFOLDING TEST - not recovered source and not part of the BF2 build.
@rem Binds the RECOVERED UDPSocket group to a real loopback port. Zero-client, 127.0.0.1 only.
@rem
@rem Intermediates go to an OWNED subdirectory, never loose into Code\Tests - a `del *.obj` here
@rem would take another agent's objects with it.
@echo off
setlocal
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
pushd "%~dp0"
if not exist "obj_socket_loopback" mkdir "obj_socket_loopback"
cl /nologo /EHsc /O2 /std:c++14 /permissive /Gy /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" socket_loopback.cpp ^
  ..\BF2\IO\Network\NetworkLayer\Socket\UDPSocket.cpp ^
  ..\BF2\Core\Debug.cpp ..\BF2\Core\StringUtil.cpp ^
  /Fo"obj_socket_loopback\\" /Fe"obj_socket_loopback\socket_loopback.exe" ^
  /link Ws2_32.lib > "obj_socket_loopback\build.log" 2>&1
if errorlevel 1 (
    echo   BUILD FAILED - see Code\Tests\obj_socket_loopback\build.log
    popd
    exit /b 1
)
"obj_socket_loopback\socket_loopback.exe"
set RC=%errorlevel%
popd
exit /b %RC%
