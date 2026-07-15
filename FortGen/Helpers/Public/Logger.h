#pragma once
#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel
{
    Info,
    Warning,
    Error
};

class Logger
{
private:
    static std::ofstream logFile;
    static std::mutex logMutex;
    static std::string GetTimestamp();
    static std::string LogLevelToString(LogLevel level);

public:
    static bool Init(const std::string& filename = "fortgen_log.txt");
    static void Log(LogLevel level, const std::string& message);
    static void Shutdown();
};
