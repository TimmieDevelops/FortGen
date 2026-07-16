#pragma once
#include "framework.h"
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

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
private:
	static std::string GetTimestamp();
	static std::string LogLevelToString(LogLevel Level);
public:
	static bool Init(const std::string& filename = "fortgen_log.txt");
	static void Log(LogLevel Level, const std::string& Message);
	static void Shutdown();
};
