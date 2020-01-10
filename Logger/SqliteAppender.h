#ifndef __SQLITE_LOG_H__
#define __SQLITE_LOG_H__

#include <atlstr.h>
#include <stdint.h>
#include <vector>
#include <mutex>

#include <log4cplus/appender.h>
#include <log4cplus/spi/loggingevent.h>

#include "LoggerDefines.h"
#include "SqliteMaintainThread.h"

const static int MAX_LOG_DATA_SIZE = 300;

class SqliteAppender : public log4cplus::Appender
{
public:
	SqliteAppender(std::string databaseFile,
		int maxRow,
		int databaseMaintainInterval,
		bool immediateFlush = false);

	~SqliteAppender();

	void getLog(const LogRequestParams &param, std::vector<LogItem> &logs);

protected:
	virtual void append(const log4cplus::spi::InternalLoggingEvent& event) override;
	void close() override;

private:
	std::string & trimLog(std::string & log);
	void addLog(std::string &logTime, log4cplus::LogLevel flag, std::string logModule,
		std::string &logMsg, std::string &logData);

	zmsg_t * requestCmd(const char * cmd, const std::string & param, int timeout);
	void parseLogs(const std::string & logString, std::vector<LogItem>& logs);

	zsock_t *mPushSock;
	SqliteMaintainThread *mSqliteMaintainThread;
};

#endif