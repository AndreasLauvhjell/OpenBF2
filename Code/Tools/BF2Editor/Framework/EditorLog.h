// Code/Tools/BF2Editor/Framework/EditorLog.h

// The editor's own diagnostics. Until the engine is up (WP3) there is no BF2Log, so every line goes
// to stderr (flushed, for runs whose stderr is captured) and to the debugger (OutputDebugStringA).
// A log file joins them once the editor knows its game root.
#ifndef BF2EDITOR_FRAMEWORK_EDITORLOG_H
#define BF2EDITOR_FRAMEWORK_EDITORLOG_H

#include "Framework/EditorConfig.h"

#include <deque>
#include <string>

namespace bf2editor
{

enum class LogLevel
{
    Info,
    Warning,
    Error
};

// printf-style; a newline is appended.
void logLine(LogLevel level, const char* format, ...);

// An unfinished editor feature was asked for. agents.md "Editor": unfinished features fail
// visibly - they log and refuse, and never report a success they did not achieve. The line reads
// "[FRONTIER] <qualifiedName>: <detail>" so the repository's frontier searches find it. The caller
// then refuses (returns failure); nothing continues as if the feature had run.
void reportFrontier(const char* qualifiedName, const char* detail);

// True when stderr is connected to something (a console, a pipe or a file). A Windows-subsystem
// process started from Explorer has no stderr, so fatal messages then need a message box.
bool hasStandardError();

// Every Error line logged so far, newline-separated (for a final message box).
const std::string& errorSummary();

// The most recent lines (the last few thousand), oldest first, for the editor's Log panel.
struct LogEntry
{
    LogLevel    level;
    std::string text;
};
const std::deque<LogEntry>& recentLog();
// The Log panel's Clear (the log file keeps every line).
void clearRecentLog();

// Options > Write the log to a file: every line
// from now on appended to `path` as well; an empty path closes it. False with `error` when it cannot be opened.
bool setLogFile(const std::string& path, std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_EDITORLOG_H
