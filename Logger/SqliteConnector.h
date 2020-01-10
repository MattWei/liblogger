#pragma once

#ifndef _SQLITE_CONNECTOR_H_
#define _SQLITE_CONNECTOR_H_

//#include <afx.h>
#include <vector>

/*************************************************
功能:
	单机版的sqlite

创建日期:
作者:
备注:	
*************************************************/

#include <sqlite3/sqlite3.h>
#include <mutex>

//typedef int (*PGET_SQL_RESULT)(void *data, int n_columns, char **column_values, char **column_names);
#define DB_MEMORY_LIMIT_HUGE	(200 * 1024 * 1024)
#define DB_MEMORY_LIMIT_LARGE	(100 * 1024 * 1024)
#define DB_MEMORY_LIMIT_MEDIUM	(50 * 1024 * 1024)
#define DB_MEMORY_LIMIT_SMALL	(20 * 1024 * 1024)

typedef std::vector<std::vector<std::string>> SqliteExecResult;

class SqliteConnector
{
private:
	std::mutex mErrorMutex;
	std::mutex mSqlMutex;

	sqlite3 *mDb;
	std::string mError;

	void setErrorMsg(std::string csMsg);
	void clearError();
	void setSqlExecError(std::string sqlStatement);

public:

	SqliteConnector(void);
	~SqliteConnector(void);

	bool open2(std::string database);
	void close(); //关闭数据库
	bool writeLog(std::string sqlStatement);
	void configure();

	bool getItemData2(const char *szSql, int nColumn, std::string &csReturn);
	bool getItemData2(const char *szSql, SqliteExecResult &result);

	bool runTransSql(const char *szSql);

	bool runSql2(const std::vector<std::string> &vtSql);
	bool runSql2(std::string csSql);

	//bool writeBlob(std::string csSql, int nBlobCol, const PBYTE uBlob, int nLen);
	//bool readBlobLen(std::string csSql, int nBlobCol,  int &nLen);
	//bool readBlob(std::string csSql, int nBlobCol, PBYTE uBlob, int &nReadLen);

	std::string getErrorMsg();

	uint64_t getLastInsertRowId();

	static void vacuum(std::string csDb);
};

#endif

