// Code/Tools/BF2Editor/App/EditorMain.cpp

// BF2Editor.exe's entry point. Editor-only code (D3).

// The editor links the whole client object set, and Game/Main/BF2.cpp in it defines the game's
// WinMain beside the BF2 class and globals the engine needs, so that object cannot be left out.
// The editor therefore enters through wWinMain (the link passes /entry:wWinMainCRTStartup); the
// game's WinMain stays linked and is never called, as it already is in the dedicated image's
// console build.
#include "Framework/EditorConfig.h"

#include "App/CommandLine.h"
#include "App/EditorApp.h"
#include "Framework/EditorLog.h"

#include <windows.h>

#include <string>
#include <vector>

namespace
{

// The engine's strings and paths are ansi (the retail engine is an mbcs build). A command-line
// character the ansi code page cannot hold is an error, not a silent '?'.
bool toAnsi(const wchar_t* text, std::string& out)
{
    out.clear();
    if (!text || !*text)
        return true;
    BOOL usedDefault = FALSE;
    const int size = WideCharToMultiByte(CP_ACP, 0, text, -1, 0, 0, 0, &usedDefault);
    if (size <= 0 || usedDefault)
        return false;
    std::vector<char> buffer(static_cast<size_t>(size));
    if (WideCharToMultiByte(CP_ACP, 0, text, -1, &buffer[0], size, 0, 0) != size)
        return false;
    out.assign(&buffer[0]);
    return true;
}

} // namespace

int WINAPI wWinMain(HINSTANCE /*instance*/, HINSTANCE /*previous*/, PWSTR commandLineW, int /*show*/)
{
    std::string commandLine;
    bf2editor::EditorOptions options;
    if (toAnsi(commandLineW, commandLine))
        options = bf2editor::parseCommandLine(commandLine);
    else
        options.errors.push_back("the command line has characters the ANSI code page cannot represent; "
                                 "the engine's file names are ANSI");

    int result;
    {
        bf2editor::EditorApp app(options);
        result = app.run();
    }

    // A Windows-subsystem process started from Explorer has no stderr: show what it would have said.
    // Batch runs never block on a dialog.
    if (!bf2editor::hasStandardError() && !options.isBatch())
    {
        if (result != bf2editor::ExitOk && !bf2editor::errorSummary().empty())
            MessageBoxA(0, bf2editor::errorSummary().c_str(), "BF2Editor", MB_OK | MB_ICONERROR);
        else if (result == bf2editor::ExitOk && options.help)
            MessageBoxA(0, bf2editor::usageText().c_str(), "BF2Editor", MB_OK | MB_ICONINFORMATION);
    }
    return result;
}
