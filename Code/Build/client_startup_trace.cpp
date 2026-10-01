// [TEMP] [CHOSEN, not recovered] External x86 debugger for the diagnostic client.
// Stops at the first fault without allowing the game's incomplete SEH dump path to hide it.
// Usage: client_startup_trace.exe <absolute BF2.exe> <working directory> [arguments]
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <string>
#pragma comment(lib, "dbghelp.lib")

int main(int argc, char** argv)
{
    if (argc < 3) return 2;
    STICKYKEYS sticky = { sizeof(sticky) };
    TOGGLEKEYS toggle = { sizeof(toggle) };
    FILTERKEYS filter = { sizeof(filter) };
    UINT trails = 0;
    BOOL saver = FALSE;
    SystemParametersInfoA(SPI_GETSTICKYKEYS, sizeof(sticky), &sticky, 0);
    SystemParametersInfoA(SPI_GETTOGGLEKEYS, sizeof(toggle), &toggle, 0);
    SystemParametersInfoA(SPI_GETFILTERKEYS, sizeof(filter), &filter, 0);
    SystemParametersInfoA(SPI_GETMOUSETRAILS, 0, &trails, 0);
    SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &saver, 0);
    std::string command = std::string("\"") + argv[1] + "\"";
    for (int i = 3; i < argc; ++i) command += std::string(" ") + argv[i];
    STARTUPINFOA startup = { sizeof(startup) };
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    SetHandleInformation(startup.hStdInput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(startup.hStdOutput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(startup.hStdError, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessA(argv[1], &command[0], NULL, NULL, TRUE,
                        DEBUG_ONLY_THIS_PROCESS, NULL, argv[2], &startup, &process)) {
        std::printf("CreateProcess failed: %lu\n", GetLastError());
        return 2;
    }
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    std::string symbolPath(argv[1]);
    symbolPath.erase(symbolPath.find_last_of("\\/"));
    SymInitialize(process.hProcess, symbolPath.c_str(), FALSE);
    const DWORD deadline = GetTickCount() + 30000;
    bool firstBreakpoint = true;
    int result = 3;
    for (;;) {
        DEBUG_EVENT event = {};
        if (!WaitForDebugEvent(&event, 500)) {
            if (static_cast<LONG>(GetTickCount() - deadline) < 0) continue;
            std::puts("TIMEOUT (30 seconds)");
            break;
        }
        DWORD disposition = DBG_CONTINUE;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            auto& info = event.u.CreateProcessInfo;
            SymLoadModuleEx(process.hProcess, info.hFile, argv[1], NULL,
                            reinterpret_cast<DWORD64>(info.lpBaseOfImage), 0, NULL, 0);
            if (info.hFile) CloseHandle(info.hFile);
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            auto& info = event.u.LoadDll;
            SymLoadModuleEx(process.hProcess, info.hFile, NULL, NULL,
                            reinterpret_cast<DWORD64>(info.lpBaseOfDll), 0, NULL, 0);
            if (info.hFile) CloseHandle(info.hFile);
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            auto& exception = event.u.Exception;
            DWORD code = exception.ExceptionRecord.ExceptionCode;
            if (code == EXCEPTION_BREAKPOINT && firstBreakpoint) {
                firstBreakpoint = false;
            } else if (code == 0x406D1388) {
                disposition = DBG_EXCEPTION_NOT_HANDLED; // MSVC thread naming.
            } else {
                std::printf("FAULT %08lX at %p firstChance=%lu\n", code,
                            exception.ExceptionRecord.ExceptionAddress, exception.dwFirstChance);
                HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, event.dwThreadId);
                CONTEXT context = {};
                context.ContextFlags = CONTEXT_FULL;
                if (GetThreadContext(thread, &context)) {
                    std::printf("EIP=%08lX ESP=%08lX EBP=%08lX ECX=%08lX\n", context.Eip, context.Esp, context.Ebp, context.Ecx);
                    DWORD stack[16] = {};
                    ReadProcessMemory(process.hProcess, reinterpret_cast<void*>(context.Esp), stack, sizeof(stack), NULL);
                    for (unsigned i = 0; i < 16; ++i) std::printf("stack[%u]=%08lX\n", i, stack[i]);
                    STACKFRAME64 frame = {};
                    frame.AddrPC = { context.Eip, 0, AddrModeFlat };
                    frame.AddrStack = { context.Esp, 0, AddrModeFlat };
                    frame.AddrFrame = { context.Ebp, 0, AddrModeFlat };
                    for (unsigned i = 0; i < 32; ++i) {
                        char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
                        auto symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
                        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                        symbol->MaxNameLen = MAX_SYM_NAME;
                        DWORD64 offset = 0;
                        SymFromAddr(process.hProcess, frame.AddrPC.Offset, &offset, symbol);
                        IMAGEHLP_LINE64 line = { sizeof(line) };
                        DWORD lineOffset = 0;
                        SymGetLineFromAddr64(process.hProcess, frame.AddrPC.Offset, &lineOffset, &line);
                        std::printf("%08llX %s+0x%llX %s:%lu\n", frame.AddrPC.Offset, symbol->Name, offset,
                                    line.FileName ? line.FileName : "", line.LineNumber);
                        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process.hProcess, thread, &frame, &context,
                                         NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
                    }
                }
                CloseHandle(thread);
                result = 1;
                break;
            }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            std::printf("EXIT %lu\n", event.u.ExitProcess.dwExitCode);
            result = event.u.ExitProcess.dwExitCode == 0 ? 0 : 1;
            ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE);
            break;
        }
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition);
    }
    TerminateProcess(process.hProcess, 42);
    DebugActiveProcessStop(process.dwProcessId);
    WaitForSingleObject(process.hProcess, 5000);
    SymCleanup(process.hProcess);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    // A debugger stop bypasses retail's restoreDesktopSettings bracket.
    SystemParametersInfoA(SPI_SETSTICKYKEYS, sizeof(sticky), &sticky, 0);
    SystemParametersInfoA(SPI_SETTOGGLEKEYS, sizeof(toggle), &toggle, 0);
    SystemParametersInfoA(SPI_SETFILTERKEYS, sizeof(filter), &filter, 0);
    SystemParametersInfoA(SPI_SETMOUSETRAILS, trails, NULL, 0);
    SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, saver, NULL, 0);
    return result;
}
