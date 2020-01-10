#include "LoggerService.h"

#include "SqliteAppender.h"
#include "Base/Timer.h"
#include "LoggerDefines.h"

#define PRINT_TO_CONSOLE

const static tchar *SQLITE_APPENDER_NAME = L"SqliteAppender";

using namespace log4cplus;

std::mutex LoggerService::mInstanceMutex;
LoggerService* LoggerService::mInstance = nullptr;

LoggerService* LoggerService::getInstance()
{
	std::lock_guard<std::mutex> lck(mInstanceMutex);
	if (!mInstance) {
		mInstance = new LoggerService();
	}

	return mInstance;
}

void LoggerService::release()
{
	std::lock_guard<std::mutex> lck(mInstanceMutex);
	if (mInstance) {
		delete mInstance;
	}

	mInstance = nullptr;
}

void LoggerService::addLogger(const char * tag, log4cplus::LogLevel logLevel)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));
	logger.setLogLevel(logLevel);
}

LoggerService::LoggerService()
{
	log4cplus::Initializer();
}

LoggerService::~LoggerService()
{
	Logger::shutdown();
}

void LoggerService::removeAllAppender(const char *tag)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));
	logger.removeAllAppenders();
}

void LoggerService::addFileAppender(
	const char *tag, std::string filename, int maxBackupIndex)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));

	std::string filenamePattern = filename + "_%d{yyyy-MM-dd-HH-mm}.log";
	std::wstring pattern = L"[%D{%Y-%m-%d %H:%M:%S.%q}] [%b] [%-5p] %m\t%M%n";
	TimeBasedRollingFileAppender *fileAppender = new TimeBasedRollingFileAppender(
		L"",
		LOG4CPLUS_STRING_TO_TSTRING(filenamePattern.c_str()),
		maxBackupIndex,
		false,
		true,
		true,
		false);

	SharedAppenderPtr fileAppenderPtr(fileAppender);
	fileAppenderPtr->setName(L"FileAppender");
	fileAppenderPtr->setLayout(std::unique_ptr<PatternLayout>(new PatternLayout(pattern)));
	logger.addAppender(fileAppenderPtr);
}

void LoggerService::addConsoleAppender(const char *tag)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));
	std::wstring pattern = L"[%D{%Y-%m-%d %H:%M:%S.%q}] [%b] [%-5p] %m\t%M%n";

	SharedAppenderPtr consoleAppend(new Win32ConsoleAppender());
	consoleAppend->setName(L"ConsoleAppender");
	consoleAppend->setLayout(std::unique_ptr<PatternLayout>(new PatternLayout(pattern)));
	logger.addAppender(consoleAppend);
}

void LoggerService::addRemoteAppenderServer(
	const char *tag, const std::string &ip, const int &port)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));
	SharedAppenderPtr socketAppender(new SocketAppender(
		LOG4CPLUS_STRING_TO_TSTRING(ip.c_str()), port, L"RemoteAppender"));
	logger.addAppender(socketAppender);
}

void LoggerService::addSqliteAppender(const char *tag, const std::string & databaseFile,
	const int & databaseMaxRow, const int & databaseMaintainInterval, bool immediateFlush)
{
	log4cplus::Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag));
	SqliteAppender *appender = new SqliteAppender(
		databaseFile, databaseMaxRow, databaseMaintainInterval, immediateFlush);
	SharedAppenderPtr sqliteAppenderPtr(appender);
	sqliteAppenderPtr->setName(SQLITE_APPENDER_NAME);
	sqliteAppenderPtr->setThreshold(log4cplus::INFO_LOG_LEVEL);
	logger.addAppender(sqliteAppenderPtr);
}

void LoggerService::getSqliteLog(
	const char *loggerTag, const LogRequestParams &param, std::vector<LogItem> &logs)
{
	Logger logger = Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(loggerTag));
	SharedAppenderPtr appenderPtr = logger.getAppender(SQLITE_APPENDER_NAME);
	SqliteAppender *appender = static_cast<SqliteAppender *>(appenderPtr.get());

	appender->getLog(param, logs);
}