#pragma once

#include <mutex>

#include <log4cplus/initializer.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/ndc.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/win32consoleappender.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/appender.h>

#include "LoggerDefines.h"

using namespace log4cplus;

class LoggerService
{
public:
	static LoggerService* getInstance();
	static void release();

	void addLogger(const char *tag, log4cplus::LogLevel logLevel /*=ALL_LOG_LEVEL*/);

	void addConsoleAppender(const char *tag);

	/*filename 不要以.log结尾*/
	void addFileAppender(const char *tag, std::string filename, int maxBackupIndex);
	void addRemoteAppenderServer(const char *tag, const std::string &ip, const int &port);
	void addSqliteAppender(const char *tag, const std::string & databaseFile,
		const int & databaseMaxRow, const int & databaseMaintainInterval, 
		bool immediateFlush = false);

	void removeAllAppender(const char *tag);

	void getSqliteLog(
		const char *loggerTag, const LogRequestParams &param, std::vector<LogItem> &logs);

private:
	LoggerService();
	~LoggerService();

	static std::mutex mInstanceMutex;
	static LoggerService* mInstance;
};

