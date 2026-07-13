#include "pch.h"

void Utils::InitConsole()
{
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);
}

void Utils::InitLogger()
{
	if (std::filesystem::exists(LoggerName)) {
		std::ofstream ofs(LoggerName, std::ofstream::trunc);
		ofs.close();
	}
	else {
		std::ofstream ofs(LoggerName);
		ofs.close();
	}
}

std::string Utils::GetLogType(ELogType LogType)
{
	std::string Information = "Info";

	switch (LogType)
	{
	case ELogType::Warning:
		Information = "Warning";
		break;
	case ELogType::Error:
		Information = "Error";
		break;
	}

	return Information;
}

void Utils::Logger(const std::string& Category, const std::string& Message, ELogType LogType)
{
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tmBuf;
	localtime_s(&tmBuf, &t);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	std::ostringstream timestamp;
	timestamp << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms.count();
	std::ofstream ofs(LoggerName, std::ios_base::app);
	std::string Information = GetLogType(LogType);
	//printf(Information.c_str()); // was testing
	ofs << "[" << timestamp.str() << "] [" << Information << "] [" << Category << "]: " << Message << std::endl;
	printf("[%s] [%s] [%s]: %s\n", timestamp.str().c_str(), Information.c_str(), Category.c_str(), Message.c_str());
}
