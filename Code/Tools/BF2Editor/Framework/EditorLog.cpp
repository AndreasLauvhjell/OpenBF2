// Code/Tools/BF2Editor/Framework/EditorLog.cpp - see EditorLog.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/EditorLog.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <string>

namespace bf2editor
{

namespace
{

std::string& errorLines()
{
    static std::string lines;
    return lines;
}

const size_t kRecentLimit = 4000;

FILE* g_logFile = 0;

void toFile(const std::string& line)
{
    if (!g_logFile)
        return;
    std::fputs(line.c_str(), g_logFile);
    std::fflush(g_logFile);
}

std::deque<LogEntry>& recentLines()
{
    static std::deque<LogEntry> lines;
    return lines;
}

void remember(LogLevel level, const std::string& text)
{
    std::deque<LogEntry>& lines = recentLines();
    LogEntry entry = { level, text };
    lines.push_back(entry);
    if (lines.size() > kRecentLimit)
        lines.pop_front();
}

const char* levelTag(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Warning: return "warning: ";
    case LogLevel::Error:   return "error: ";
    default:                return "";
    }
}

void emit(LogLevel level, const std::string& text)
{
    const std::string line = std::string("[BF2Editor] ") + levelTag(level) + text + "\n";
    std::fputs(line.c_str(), stderr);
    std::fflush(stderr);
    toFile(line);
    OutputDebugStringA(line.c_str());
    remember(level, text);
    if (level == LogLevel::Error)
    {
        errorLines() += text;
        errorLines() += "\n";
    }
}

} // namespace

void logLine(LogLevel level, const char* format, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, format);
    const int written = std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (written < 0)
        emit(LogLevel::Error, std::string("log formatting failed for: ") + format);
    else
        emit(level, buffer);        // vsnprintf truncates and terminates an over-long line
}

void reportFrontier(const char* qualifiedName, const char* detail)
{
    const std::string line = std::string("[FRONTIER] ") + qualifiedName + ": " + detail;
    std::fprintf(stderr, "%s\n", line.c_str());
    std::fflush(stderr);
    toFile(line + "\n");
    OutputDebugStringA((line + "\n").c_str());
    remember(LogLevel::Error, line);
    errorLines() += line;
    errorLines() += "\n";
}

bool setLogFile(const std::string& path, std::string& error)
{
    if (g_logFile)
    {
        std::fclose(g_logFile);
        g_logFile = 0;
    }
    if (path.empty())
        return true;
    g_logFile = std::fopen(path.c_str(), "a");
    if (!g_logFile)
    {
        error = "cannot open " + path + " for the log";
        return false;
    }
    return true;
}

bool hasStandardError()
{
    const HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
    if (handle == 0 || handle == INVALID_HANDLE_VALUE)
        return false;
    return GetFileType(handle) != FILE_TYPE_UNKNOWN;
}

const std::string& errorSummary()
{
    return errorLines();
}

const std::deque<LogEntry>& recentLog()
{
    return recentLines();
}

void clearRecentLog()
{
    recentLines().clear();
}

} // namespace bf2editor
