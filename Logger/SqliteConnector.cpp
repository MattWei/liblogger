/*功能:Sqlite数据库本地操作类
*修改:
*备注:
*/

#include "SqliteConnector.h"

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

// trim from start
static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}

SqliteConnector::SqliteConnector(void)
	: mDb(NULL)
{
}

SqliteConnector::~SqliteConnector(void)
{
	close();
}

void SqliteConnector::setErrorMsg(std::string csMsg)
{
	std::lock_guard<std::mutex> lck(mErrorMutex);
	mError = csMsg;
}

std::string SqliteConnector::getErrorMsg()
{
	std::lock_guard<std::mutex> lck(mErrorMutex);
	return mError;
}

uint64_t SqliteConnector::getLastInsertRowId()
{
	return (uint64_t)sqlite3_last_insert_rowid(mDb);
}

void SqliteConnector::clearError()
{
	std::lock_guard<std::mutex> lck(mErrorMutex);
	mError = "";
}

void SqliteConnector::setSqlExecError(std::string sqlStatement)
{
	std::string error((char *)sqlite3_errmsg(mDb) + std::string("\n") + sqlStatement);
	setErrorMsg(error);
}

/*************************************************
功能:
	配置数据库

参数:

返回值:

备注:	
*************************************************/
void SqliteConnector::configure()
{
	sqlite3_exec(mDb, "PRAGMA synchronous=OFF", 0, 0, NULL);//如果有定期备份的机制，而且少量数据丢失可接受，用OFF
	//sqlite3_exec(m_pDb, "PRAGMA page_size = 4096",0,0, NULL);//只有在未创建数据库时才能设置
	sqlite3_exec(mDb, "PRAGMA cache_size=8000", 0, 0, NULL); //cache大小
	sqlite3_exec(mDb, "PRAGMA case_sensitive_like=1", 0, 0, NULL);//搜索中文字串
	//sqlite3_exec(m_pDb, "PRAGMA temp_store=MEMORY",0, 0, NULL);//临时表和索引存放于内存中

	sqlite3_soft_heap_limit64(DB_MEMORY_LIMIT_HUGE); //DB_MEMORY_LIMIT_SMALL
}


/*************************************************
功能:
	写log

参数:
	szSql		-	SQL语句


返回值:

备注:	
*************************************************/
bool SqliteConnector::writeLog(std::string sqlStatement)
{
	std::lock_guard<std::mutex> lck(mSqlMutex);

	sqlStatement = trim(sqlStatement);

	if (sqlStatement.empty() || NULL == mDb) 
	{
		setErrorMsg("Error parameters.");
		return false;
	}	

	try
	{
		int nResult;
		sqlite3_stmt *stmt = NULL;
		
		clearError();
		nResult = sqlite3_prepare(mDb, sqlStatement.c_str(), -1, &stmt, NULL);
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(sqlStatement);
			sqlite3_finalize(stmt);
			return false;
		}

		nResult = sqlite3_step(stmt);
		if (nResult != SQLITE_DONE)
		{
			setSqlExecError(sqlStatement);
			sqlite3_finalize(stmt);
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::writeLog()");
		return false;
	}
}


/*************************************************
功能:
	关闭数据库

参数:

返回值:

备注:	
*************************************************/
void SqliteConnector::close()
{
	std::lock_guard<std::mutex> lck(mSqlMutex);
	if (mDb != NULL)
	{
		sqlite3_close(mDb);
		mDb = NULL;
	}

}

#if 0
/*************************************************
功能:
	保存Blob字段

参数:
	szSql		-	SQL语句
	nBlobCol	-	Blob列号
	uBlob		-	Blob字段的内容
	nLen		-	Blob字段的长度


返回值:

备注:	
	SQL语句格式: insert into t_test ( ID, MEM) values(1, ? )"
*************************************************/
bool SqliteConnector::writeBlob(std::string sqlStatement, int nBlobCol, const PBYTE uBlob, int nLen)
{ 
	std::lock_guard<std::mutex> lck(mSqlMutex);

	sqlStatement = trim(sqlStatement);

	if (sqlStatement.empty() || NULL == uBlob || nLen < 0 || NULL == mDb)
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	sqlite3_stmt *stat;
	int nResult;
	
	try
	{
		setErrorMsg("");

		nResult = sqlite3_prepare(mDb, sqlStatement.c_str(), -1, &stat, NULL);
		if (nResult != SQLITE_OK || NULL == stat)
		{
			setSqlExecError(sqlStatement);
			return false;
		}

		nResult = sqlite3_bind_blob(stat, nBlobCol, (void *)uBlob, nLen, NULL );
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(sqlStatement);
			return false;
		}
	
		nResult = sqlite3_step( stat );

		nResult = sqlite3_finalize( stat );
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(sqlStatement);
			return false;
		}

		return true;
	}
	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::writeBlob()");
		return false;
	}
}



/*************************************************
功能:
	获取blob的长度

参数:
	szSql		-	SQL语句
	nBlobCol	-	Blob列号
	nLen		-	返回Blob字段的长度


返回值:

备注:	
*************************************************/
bool SqliteConnector::readBlobLen(std::string sqlStatement, int nBlobCol,  int &nLen)
{
	sqlite3_stmt *stat;
	int nResult;

	std::lock_guard<std::mutex> lck(mSqlMutex);

	sqlStatement = trim(sqlStatement);
	if (sqlStatement.empty() || NULL == mDb || nBlobCol < 0)
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	try
	{
		setErrorMsg("");
		nResult = sqlite3_prepare(mDb, sqlStatement.c_str(), -1, &stat, NULL );
		if (nResult != SQLITE_OK || NULL == stat)
		{
			setSqlExecError(sqlStatement);
			nLen = 0;
			return false;
		}

		nResult = sqlite3_step( stat );

		nLen = sqlite3_column_bytes(stat, nBlobCol); 

		sqlite3_finalize(stat);

		if (nLen >= 0)
		{
			return true;
		}
		else
		{
			setSqlExecError(sqlStatement);
			nLen = 0;
			return false;
		}
	}

	catch(...)
	{
		//if (!g_bMainServer)	m_hSemSql.UnLock();
		setErrorMsg("Fatal error, SqliteConnector::readBlobLen()");
		nLen = 0;
		return false;
	}
}


/*************************************************
功能:
	获取blob字段的内容

参数:
	szSql		-	SQL语句
	nBlobCol	-	Blob列号
	uBlob		-	返回Blob字段的内容
	nReadLen	-	返回Blob字段的长度


返回值:

备注:	
*************************************************/
bool SqliteConnector::readBlob(std::string sqlStatement, int nBlobCol, PBYTE uBlob, int &nReadLen)
{
	sqlite3_stmt *stat;
	int nResult;

	std::lock_guard<std::mutex> lck(mSqlMutex);

	sqlStatement = trim(sqlStatement);
	if (NULL == uBlob || sqlStatement.empty() || nBlobCol < 0 || NULL == mDb)
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	setErrorMsg("");
	try
	{
		nResult = sqlite3_prepare(mDb, sqlStatement.c_str(), -1, &stat, NULL );
		if (nResult != SQLITE_OK || NULL == stat)
		{
			setSqlExecError(sqlStatement);
			return false;
		}
		
		nResult = sqlite3_step(stat);
		
		nReadLen = sqlite3_column_bytes(stat, nBlobCol); 
		if (nReadLen < 0)
		{
			setSqlExecError(sqlStatement);
			return false;
		}
		else if (0 == nReadLen)
		{
			return true;
		}
		else
		{
			void *buf = (PBYTE)sqlite3_column_blob(stat, nBlobCol);

			if (buf == NULL)
			{
				setSqlExecError(sqlStatement);
				nResult = sqlite3_finalize( stat );

				return false;
			}

			memcpy((void *)uBlob, buf, nReadLen);

			nResult = sqlite3_finalize( stat );

			if (nResult != SQLITE_OK)
			{
				setSqlExecError(sqlStatement);
				return false;
			}
	
			return true;
		}
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::readBlob()");
		return FALSE;
	}
}
#endif

bool SqliteConnector::getItemData2(const char *szSql, int nColumn, std::string &csReturn)
{
	int nResult;
	sqlite3_stmt *stmt = NULL;
	char *pVaulue;

	csReturn = "";

	std::lock_guard<std::mutex> lck(mSqlMutex);
	if (NULL == szSql || nColumn < 0 || NULL == mDb) 
	{
		//m_hSemSql.UnLock();
		setErrorMsg("Error parameters.");
		return false;
	}

	setErrorMsg("");
			
	try
	{			
		//编译sql
		nResult = sqlite3_prepare(mDb, szSql, -1, &stmt, NULL); //(const void **)&tail);
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);

			return false;
		}
		

		//执行sql
		nResult = sqlite3_step(stmt);

		if (nResult == SQLITE_DONE)  //没有数据
		{
			sqlite3_finalize(stmt);
			return true;
		}

		if (nResult != SQLITE_ROW)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);

			return false;
		}

		//总列数
		int ncols = sqlite3_column_count(stmt);
		if (ncols <= 0)
		{
			sqlite3_finalize(stmt);
			setErrorMsg("Column must be more than 0!");
			return false;
		}


		//返回数据
		for (int i=0; i<ncols; i++)
		{
			if (i == nColumn)
			{
				pVaulue = (char *)sqlite3_column_text(stmt, i);

				if (NULL == pVaulue)
					csReturn = "";
				else
					csReturn = std::string(pVaulue);

				break;
			}
			else
			{
				sqlite3_column_text(stmt, i);
			}
		}
		

		sqlite3_finalize(stmt);

		return true;
	}
	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::getItemData2()");
		return false;
	}


	return false;
}

bool SqliteConnector::getItemData2(const char *szSql, SqliteExecResult &result)
{
	int nResult;
	sqlite3_stmt *stmt = NULL;
	char *pValue;

	std::lock_guard<std::mutex> lck(mSqlMutex);

	if (NULL == szSql || NULL == mDb)
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	setErrorMsg("");
	try
	{
		//编译
		nResult = sqlite3_prepare(mDb, szSql, -1, &stmt, NULL); 
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);

			return false;
		}

		//执行
		nResult = sqlite3_step(stmt);
		if (nResult == SQLITE_DONE) //没有数据
		{
			sqlite3_finalize(stmt);
			return true;
		}

		if (nResult != SQLITE_ROW)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);

			return false;
		}

		//总列数
		int ncols = sqlite3_column_count(stmt);
		if (ncols <= 0)
		{
			sqlite3_finalize(stmt);
			setErrorMsg("Column must be more than 0!");
			return false;
		}

		while (nResult == SQLITE_ROW)
		{
			std::vector<std::string> columns;
			for(int i = 0; i <ncols; i++)
			{
				pValue = (char *)sqlite3_column_text(stmt,i);
				if (pValue == NULL)
					columns.push_back("");
				else
					columns.push_back(pValue);
			}
			result.push_back(columns);

			nResult = sqlite3_step(stmt);
		}

		sqlite3_finalize(stmt);

		return true;
	}
	catch(...)
	{
		//if (!g_bMainServer)	m_hSemSql.UnLock();
		setErrorMsg("Fatal error, CHNSqlite::GetItemData2()");
		return false;
	}
}

//run transaction
bool SqliteConnector::runTransSql(const char *szSql)
{
	int nResult;
	sqlite3_stmt *stmt = NULL;

	std::lock_guard<std::mutex> lck(mSqlMutex);

	if (NULL == szSql || NULL == mDb) 
	{
		setErrorMsg("Error parameters.");
		return false;
	}


	clearError();

	try
	{

		nResult = sqlite3_prepare(mDb, szSql, -1, &stmt, NULL);
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);
			return false;
		}

		nResult = sqlite3_step(stmt);
		if (nResult != SQLITE_DONE)
		{
			setSqlExecError(szSql);
			sqlite3_finalize(stmt);
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::runTransSql()");
		return false;
	}
}

bool SqliteConnector::open2(std::string szdatabase)
{
	int nResult;

	try
	{
		close();

		nResult = sqlite3_open(szdatabase.c_str(), &mDb);

		if ( nResult != SQLITE_OK )
		{
			setSqlExecError("");
			sqlite3_close(mDb);
			return false;
		}

		clearError();

		configure();
	
		return true;
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::open2");
		return false;
	}
	
}

//执行多条没有返回结果的Sql语句, 如insert, delete, ...
bool SqliteConnector::runSql2(const std::vector<std::string> &vtSql)
{
	int nResult;
	sqlite3_stmt *stmt = NULL;

	std::lock_guard<std::mutex> lck(mSqlMutex);

	if (NULL == mDb) 
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	if (vtSql.empty()) 
	{
		return true;
	}
	
	try
	{
		clearError();

		for (size_t i=0; i < vtSql.size(); i++)
		{
			std::string sqlStatement = const_cast<std::string&>(vtSql[i]);
			trim(sqlStatement);
			if (sqlStatement.empty()) {
				continue;
			}

			nResult = sqlite3_prepare(mDb, sqlStatement.c_str(), -1, &stmt, NULL);
			if (nResult != SQLITE_OK)
			{
				setSqlExecError(sqlStatement);
				sqlite3_finalize(stmt);
				return false;
			}

			nResult = sqlite3_step(stmt);
			if (nResult != SQLITE_DONE)
			{
				setSqlExecError(sqlStatement);
				sqlite3_finalize(stmt);	
				return false;
			}

			sqlite3_finalize(stmt);
		}

		return true;
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::runSql2");
		return false;
	}

	return false;
}

//执行单条没有返回结果的Sql语句, 如insert, delete, ...
bool SqliteConnector::runSql2(std::string sqlStatement)
{
	int nResult;
	sqlite3_stmt *stmt = NULL;

	std::lock_guard<std::mutex> lck(mSqlMutex);
	sqlStatement = trim(sqlStatement);

	if (sqlStatement.empty() || NULL == mDb)
	{
		setErrorMsg("Error parameters.");
		return false;
	}

	clearError();
	try
	{
		nResult = sqlite3_prepare_v2(mDb, sqlStatement.c_str(), -1, &stmt, NULL);
		if (nResult != SQLITE_OK)
		{
			setSqlExecError(sqlStatement);
			sqlite3_finalize(stmt);

			return false;
		}

		nResult = sqlite3_step(stmt);

		if (nResult != SQLITE_DONE)
		{
			setSqlExecError(sqlStatement);
			sqlite3_finalize(stmt);
			return false;
		}

		sqlite3_finalize(stmt);

		return true;
	}

	catch(...)
	{
		setErrorMsg("Fatal error, SqliteConnector::runSql2");
		return false;
	}

	return false;
}

//回收数据库空间 
void SqliteConnector::vacuum(std::string csDb)
{
	//CHNSqlite db;

	//db.Open2(csDb);
	//db.RunSql2(_T("VACUUM;"));
	//db.Close();
}