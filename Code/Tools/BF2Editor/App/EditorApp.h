// Code/Tools/BF2Editor/App/EditorApp.h

// The editor application: it owns the run from a parsed command line to the process exit code.
// Editor-only code (D3). An unfinished phase reports [FRONTIER] and refuses (exit code 3).
#ifndef BF2EDITOR_APP_EDITORAPP_H
#define BF2EDITOR_APP_EDITORAPP_H

#include "Framework/EditorConfig.h"
#include "App/CommandLine.h"
#include "App/EditorWindow.h"
#include "App/EngineHost.h"

namespace bf2editor
{

// Process exit codes. The retail editor's own crash exit is not reused.
enum ExitCode
{
    ExitOk             = 0,
    ExitBadCommandLine = 2,     // refused: the command line has errors (see stderr)
    ExitNotImplemented = 3,     // refused: a phase the run needs is not written yet ([FRONTIER])
    ExitEngineFailed   = 4      // the engine did not come up (the failing step is on stderr)
};

class EditorApp
{
public:
    explicit EditorApp(const EditorOptions& options);
    ~EditorApp();

    int run();

private:
    EditorApp(const EditorApp&);
    EditorApp& operator=(const EditorApp&);

    void logOptions() const;

    // WP3 (E2, engine half): bring the engine up the editor's way (EngineHost): modules, file system,
    // mod paths and archives, localization, ModInfo, network; no menu, Swiff, profiles or HUD.
    bool bootEngine();
    void shutdownEngine();

    // A batch run: open the level, run the script, save the parts asked for, exit.
    int runBatch();
    // An interactive run: the frame loop with the ImGui shell (WP4).
    int runInteractive();

    EditorOptions m_options;
    EditorWindow  m_window;         // declared before m_engine: destroyed after the engine lets go of it
    EngineHost    m_engine;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_EDITORAPP_H
