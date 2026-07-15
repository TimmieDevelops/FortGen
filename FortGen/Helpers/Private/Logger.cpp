#include "pch.h"
#include "Helpers/Public/Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    struct tm timeInfo;
    #if defined(_WIN32)
    localtime_s(&timeInfo, &in_time_t);
    #else
    localtime_r(&in_time_t, &timeInfo);
    #endif
    ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    default: return "LOG";
    }
}

bool Logger::Init(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open())
    {
        return true;
    }
    logFile.open(filename, std::ios::out | std::ios::app);
    if (logFile.is_open())
    {
        logFile << "\n========================================\n";
        logFile << "[" << GetTimestamp() << "] [INFO] Logger initialized successfully.\n";
        logFile << "========================================\n";
        logFile.flush();
        return true;
    }
    return false;
}

void Logger::Log(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::string timestamp = GetTimestamp();
    std::string levelStr = LogLevelToString(level);
    std::string formattedMessage = "[" + timestamp + "] [" + levelStr + "] " + message + "\n";

    // Write to standard console output
    std::cout << formattedMessage;
    std::cout.flush();

    // Write to file
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
