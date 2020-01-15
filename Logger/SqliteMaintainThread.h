#pragma once

#include "Base/Timer.h"
#include "Base/ZactorThread.h"

#include "LoggerDefines.h"
#include "SqliteConnector.h"

const static char *LOGER_SERVICE_URI = "inproc://logger_inproc";
const static char *SQLITE_LOGGER_CTRL_INPROC = "inproc://sqlite_log_ctrl.inproc";
const static char *GET_LOG_CMD = "GetLog";

class SqliteMaintainThread: public ZactorThread
{
public:
	SqliteMaintainThread(std::string databaseFile,
		int maxRow,
		int databaseMaintainInterval,
		bool immediateFlush);
	~SqliteMaintainThread();

private:
	bool openSqlDatabase();
	bool createTable();
	void resetDatabase();

	virtual void run(zsock_t * pipe) override;
	void saveLog(zsock_t * sock, std::vector<LogItem> &logs);
	void saveSingleLogToSqlite(const LogItem &log);
	void saveLogToSqlite(std::vector<LogItem> &logs);
	void processInsertLogStatements(const std::vector<std::string> &logs);

	void backupLog();
	void maintainSqliteFile();

	void processCtrl(zsock_t * ctrlSock);

	void getLog(const std::string &paramString, zsock_t * ctrlSock);

	int mMaxRow;
	int mMaintainIntevio;
	std::string mSqliteLogFile;
	bool mImmediateFlush;

	SqliteConnector mSqlDb;

	Timer mMaintainSqliteTimer;	
};
