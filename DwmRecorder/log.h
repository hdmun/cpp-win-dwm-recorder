#ifndef LOG_H
#define LOG_H

enum class LogLevel : unsigned {
	Trace = 1,
	Debug,
	Info,
	Warn,
	Error,
};

#define LOG_BUFFER_SIZE 1024

#define TRACE_LOG(format, ...) _log(LogLevel::Trace, L"%s [TRACE] [%hs(%hs:%d)] >> " format L"\n", getTimestamp().c_str(), file_name(__FILE__), __func__, __LINE__, __VA_ARGS__)
#define DEBUG_LOG(format, ...) _log(LogLevel::Debug, L"%s [DEBUG] [%hs(%hs:%d)] >> " format L"\n", getTimestamp().c_str(), file_name(__FILE__), __func__, __LINE__, __VA_ARGS__)
#define INFO_LOG(format, ...) _log(LogLevel::Info, L"%s [INFO] [%hs(%hs:%d)] >> " format L"\n", getTimestamp().c_str(), file_name(__FILE__), __func__, __LINE__, __VA_ARGS__)
#define WARN_LOG(format, ...) _log(LogLevel::Warn, L"%s [WARN] [%hs(%hs:%d)] >> " format L"\n", getTimestamp().c_str(), file_name(__FILE__), __func__, __LINE__, __VA_ARGS__)
#define ERROR_LOG(format, ...) _log(LogLevel::Error, L"%s [ERROR] [%hs(%hs:%d)] >> " format L"\n", getTimestamp().c_str(), file_name(__FILE__), __func__, __LINE__, __VA_ARGS__)

extern bool isLoggingEnabled;
extern LogLevel logSeverityLevel;
extern std::wstring logFilePath;

inline  void _log(LogLevel logLvl, const wchar_t* format, ...)
{
	if (isLoggingEnabled && logLvl >= logSeverityLevel) {
		wchar_t buffer[LOG_BUFFER_SIZE];
		va_list args;
		va_start(args, format);
		vswprintf(buffer, LOG_BUFFER_SIZE, format, args);
		if (!logFilePath.empty()) {
			std::wofstream logFile(logFilePath, std::ios_base::app | std::ios_base::out);
			if (logFile.is_open()) {
				logFile << buffer;
				logFile.close();
			}
		}

		OutputDebugStringW(buffer);
		va_end(args);
	}
}

constexpr const char* file_name(const char* path)
{
	const char* file = path;
	while (*path) {
		if (*path++ == '\\') {
			file = path;
		}
	}
	return file;
}

#pragma warning(push)
#pragma warning(disable:4505)
static std::wstring getTimestamp()
{
	// get a precise timestamp as a string
	const auto now = std::chrono::system_clock::now();
	const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		now.time_since_epoch()) % 1000;
	std::wstringstream nowSs;

	std::tm time;
	localtime_s(&time, &nowAsTimeT);
	nowSs
		<< std::put_time(&time, L"%Y-%m-%d %H:%M:%S")
		<< '.' << std::setfill(L'0') << std::setw(3) << nowMs.count();
	return nowSs.str();
}
#pragma warning(pop)

#endif //LOG_H
