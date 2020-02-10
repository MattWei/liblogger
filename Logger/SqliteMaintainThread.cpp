#include "SqliteMaintainThread.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "czmp_log_protocol.pb.h"
#include "Base/czmq_defines.h"

#define DEFAULT_MAX_SQLITE_LOG_ROW 50000
#define DEFAULT_LOG_SIZE_MAINTAIN_INTERVAL 18000
#define DEFAULT_MAX_SQLITE_LOG_FILE 1000
#define MAX_LOG_QUEUE_SIZE	3000

#define LOG_DATABASE_DIR "Data"
#define LOG_ARCH_DIR "LogArch"
#define LOG_DB_FILE "IntevioSystem_log.db"

SqliteMaintainThread::SqliteMaintainThread(std::string databaseFile,
	int maxRow,
	int databaseMaintainInterval,
	bool immediateFlush)
	: mMaxRow(maxRow)
	, mMaintainIntevio(databaseMaintainInterval)
	, mImmediateFlush(immediateFlush)
	, mSqliteLogFile(databaseFile)
{
	//mSqliteLogFile = databaseDir + "\\" + LOG_DATABASE_DIR "\\" LOG_DB_FILE;
}

SqliteMaintainThread::~SqliteMaintainThread()
{
	stop();
}

bool SqliteMaintainThread::openSqlDatabase()
{
	mSqlDb.close();
	if (!mSqlDb.open2(mSqliteLogFile)) {
		printf("Open sqlite log file:%s fail\n", mSqliteLogFile.c_str());
		return false;
	}

	return createTable();
}

bool SqliteMaintainThread::createTable()
{
	std::vector<std::string> vtSql;
	std::stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS T_Log ("
		<< "Id INTEGER PRIMARY KEY NOT NULL, "
		<< "LogDate TIMESTAMP NOT NULL, "
		<< "LogType INTEGER NOT NULL, "
		<< "LogModule VARCHAR(128) NOT NULL, "
		<< "LogMsg VARCHAR(512) NOT NULL, "
		<< "LogData VARCHAR(512)"
		<< ");";

	vtSql.emplace_back(ss.str());
	ss.str("");
	ss << "CREATE INDEX IF NOT EXISTS [IDX_T_LOG_LogData] ON [T_Log] ("
		<< "[LogDate]  ASC"
		<< ");";
	vtSql.emplace_back(ss.str());

	mSqlDb.runTransSql("BEGIN;");
	if (mSqlDb.runSql2(vtSql)) {
		mSqlDb.runTransSql("COMMIT;");
		return true;
	}
	else {
		mSqlDb.runTransSql("ROLLBACK;");
		return false;
	}
}

void SqliteMaintainThread::resetDatabase()
{
	mSqlDb.close();
	zsys_file_delete(mSqliteLogFile.c_str());
	openSqlDatabase();
}

bool SqliteMaintainThread::onStarted()
{
	if (!openSqlDatabase()) {
		sendSignal(-1);
		return false;
	}

	sendSignal(0);
	return true;
}

void SqliteMaintainThread::run()
{
	printf("SqliteMaintainThread start\n");

	zsock_t *pullSock = zsock_new_pull(LOGER_SERVICE_URI);
	assert(pullSock);
	addPollerSock(pullSock);

	zsock_t *ctrlSock = zsock_new_rep(SQLITE_LOGGER_CTRL_INPROC);
	assert(ctrlSock);
	addPollerSock(ctrlSock);

	std::vector<LogItem> logs;
	logs.reserve(MAX_LOG_QUEUE_SIZE * 2);

	mMaintainSqliteTimer.start();

	Timer saveLogTimer;
	if (!mImmediateFlush) {
		saveLogTimer.start();
	}
	
	while (!zsys_interrupted) {
		zsock_t *which = pollerWait(100);
		if (isExit(which)) {
			break;
		}
		if (which == pullSock) {
			saveLog(pullSock, logs);
		}
		if (which == ctrlSock) {
			processCtrl(ctrlSock);
		}

		if (!mImmediateFlush && 
			(logs.size() >= MAX_LOG_QUEUE_SIZE || saveLogTimer.isTimeout(100))) {
			saveLogToSqlite(logs);
			saveLogTimer.start();
		}

		maintainSqliteFile();
	}

	onExit();
	zsock_destroy(&pullSock);
	zsock_destroy(&ctrlSock);

	saveLogToSqlite(logs);

	printf("SqliteMaintainThread exit\n");
}

void SqliteMaintainThread::saveLog(zsock_t *sock, std::vector<LogItem> &logs)
{
	zmsg_t *msg = zmsg_recv(sock);
	if (!msg) {
		return;
	}

	LogItem logDetail;
	//Log 格式: [Log time][Log source][Log type][Log msg][Log data]
	logDetail.logTime = zmsg_pop_stdstring(msg);
	logDetail.logModule = zmsg_pop_stdstring(msg);
	logDetail.logLevel = static_cast<log4cplus::LogLevel>(zmsg_pop_int(msg));
	logDetail.logMsg = zmsg_pop_stdstring(msg);
	logDetail.logData = "";
	if (zmsg_size(msg) > 0) {
		logDetail.logData = zmsg_pop_stdstring(msg);
	}
	zmsg_destroy(&msg);

	if (mImmediateFlush) {
		saveSingleLogToSqlite(logDetail);
	}
	else {
		logs.emplace_back(logDetail);
	}
}

void SqliteMaintainThread::saveSingleLogToSqlite(const LogItem & log)
{
	if (log.logMsg.empty()) {
		return;
	}

	std::vector<std::string> sqlBuff;
	sqlBuff.reserve(1);

	//写数据库
	std::stringstream ss;
	ss << "INSERT INTO T_Log (LogDate, LogType, LogModule, LogMsg, LogData) VALUES ('"
		<< log.logTime << "', "
		<< log.logLevel << ", '"
		<< log.logModule << "', '"
		<< log.logMsg << "', '"
		<< log.logData << "');";

	sqlBuff.emplace_back(ss.str());
	processInsertLogStatements(sqlBuff);
}

void SqliteMaintainThread::saveLogToSqlite(std::vector<LogItem> &logs)
{
	if (logs.empty()) {
		return;
	}

	std::vector<std::string> sqlBuff;
	sqlBuff.reserve(logs.size());

	for (auto it = logs.rbegin(); it != logs.rend(); ++it)
	{
		//写数据库
		std::stringstream ss;
		ss << "INSERT INTO T_Log (LogDate, LogType, LogModule, LogMsg, LogData) VALUES ('"
			<< it->logTime << "', "
			<< it->logLevel << ", '"
			<< it->logModule << "', '"
			<< it->logMsg << "', '"
			<< it->logData << "');";

		sqlBuff.emplace_back(ss.str());
	}

	logs.clear();

	processInsertLogStatements(sqlBuff);
	sqlBuff.clear();
}

void SqliteMaintainThread::processInsertLogStatements(const std::vector<std::string>& logs)
{
	mSqlDb.runTransSql("BEGIN;");
	if (mSqlDb.runSql2(logs)) {
		mSqlDb.runTransSql("COMMIT;");
	}
	else {
		mSqlDb.runTransSql("ROLLBACK;");
		resetDatabase();
	}
}

void SqliteMaintainThread::maintainSqliteFile()
{
	//日志大小检查太频繁会拉高CPU使用率,所以需要加定时开子线程操作
	if (mMaintainSqliteTimer.isTimeout(mMaintainIntevio * 1000)) {
		backupLog();
		mMaintainSqliteTimer.start();
	}
}

void SqliteMaintainThread::backupLog()
{
	SqliteExecResult result;
	std::string sql("SELECT COUNT(1) FROM T_Log;");

	mSqlDb.getItemData2(sql.c_str(), result);
	if (result.empty()) {
		return;
	}

	int count = std::stoi(result[0][0]);
	if (mMaxRow > 0 && count > mMaxRow + 10000) {
		//数据库超出直接删除，不用导出。
		std::stringstream ss;
		ss << "DELETE FROM T_Log WHERE LogDate NOT IN "
			<< "(SELECT LogDate FROM T_Log ORDER BY LogDate DESC LIMIT "
			<< mMaxRow << ");";

		if (!mSqlDb.runSql2(ss.str())) {
			resetDatabase();
		}
	}
}

void SqliteMaintainThread::processCtrl(zsock_t *ctrlSock)
{
	zmsg_t *msg = zmsg_recv(ctrlSock);
	if (!msg) {
		return;
	}

	std::string cmd = zmsg_pop_stdstring(msg);
	std::string param = zmsg_pop_stdstring(msg);
	zmsg_destroy(&msg);

	if (cmd == std::string(GET_LOG_CMD)) {
		getLog(param, ctrlSock);
	}
}

void SqliteMaintainThread::getLog(const std::string &paramString, zsock_t *ctrlSock)
{
	LogGetterParams param;
	param.ParseFromString(paramString);

	std::stringstream ss;
	ss << "SELECT LogDate, LogType, LogModule, LogMsg, LogData FROM T_Log "
		<< "WHERE LogDate >= '" << param.datestart()
		<< "' AND LogDate <= '" << param.dateend() << "' "
		<< "AND LogType >= " << param.startlevel() << " ";

	std::string fliter = param.fliter();
	if (!fliter.empty()) {
		std::transform(fliter.begin(), fliter.end(), fliter.begin(),
			[](unsigned char c) { return std::tolower(c); });

		ss << "AND LOWER(LogMsg) like '%" << fliter << "%' ";
	}
	
	ss << "ORDER BY LogDate DESC LIMIT " << param.maxrows() << ";";
	
	LogVector logs;
	SqliteExecResult res;
	if (mSqlDb.getItemData2(ss.str().c_str(), res)) {
		for (auto it = res.begin(); it != res.end(); ++it) {
			LogDetail *log = logs.add_log();
			log->set_logtime(it->at(0));
			log->set_loglevel(std::stoi(it->at(1)));
			log->set_logmodule(it->at(2));
			log->set_logmsg(it->at(3));

			std::string &logData = it->at(4);
			if (!logData.empty()) {
				log->set_logdata(logData);
			}
		}
	}

	zmsg_t *resMsg = zmsg_new();
	zmsg_addstr(resMsg, GET_LOG_CMD);

	std::string logString = logs.SerializePartialAsString();
	zmsg_addstr(resMsg, logString.c_str());

	zmsg_send(&resMsg, ctrlSock);
}

