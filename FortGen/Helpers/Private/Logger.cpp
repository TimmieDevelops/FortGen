#include "pch.h"

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    struct tm timeInfo;

    localtime_s(&timeInfo, &in_time_t);

    ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel Level)
{
    switch (Level)
    {
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "LOG";
    }
}

bool Logger::Init(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open())
        return true;
    
    if (std::filesystem::exists(filename))
    {
        std::ofstream clearFile(filename, std::ios::out | std::ios::trunc);
        if (clearFile.is_open())
            clearFile.close();
    }

    logFile.open(filename, std::ios::out | std::ios::app);

    if (logFile.is_open())
    {
        logFile << "========================================\n";
        logFile << "[" << GetTimestamp() << "] [INFO] Logger initialized successfully.\n";
        logFile << "========================================\n";
        logFile.flush();
        return true;
    }

    return false;
}

void Logger::Log(LogLevel Level, const std::string& Message)
{
    std::lock_guard<std::mutex> lock(logMutex);

    std::string timestamp = GetTimestamp();
    std::string levelStr = LogLevelToString(Level);
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + Message + "\n";

    std::cout << formattedMessage;
    std::cout.flush();

    if (logFile.is_open())
    {
        logFile << formattedMessage;
        logFile.flush();
    }
}

void Logger::Shutdown()
{
    std::lock_guard<std::mutex> lock(logMutex);

    if (logFile.is_open())
    {
        logFile << "[" << GetTimestamp() << "] [INFO] Logger shutting down.\n";
        logFile << "========================================\n";
        logFile.close();
    }
}
