#pragma once
#include "framework.h"

enum ELogType
{
	Info,
	Warning,
	Error
};

class Utils
{
public:
	static void InitConsole();
	static void InitLogger();
	static std::string GetLogType(ELogType LogType);
	static void Logger(const std::string& Category, const std::string& Message, ELogType LogType = ELogType::Info);
private:
	inline static std::string LoggerName = "FortGen.txt";
public:
	static uintptr_t GetImageBase()
	{
		return reinterpret_cast<uintptr_t>(GetModuleHandle(0));
	}

	template<typename T>
	static T* GetDataIndex(void* Data, int Index, size_t Size)
	{
		if (!Data) return nullptr;
		return *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(Data) + Index * Size);
	}
};