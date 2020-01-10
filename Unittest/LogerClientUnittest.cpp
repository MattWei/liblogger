#include "pch.h"

#include <czmq.h>

#define THIS_MODULE "LogerUnittest"

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

#include <log4cplus/appender.h>
#include <log4cplus/spi/loggingevent.h>

#include "Logger/LoggerDefines.h"
#include "Logger/LoggerClient.h"
#include "Logger/LoggerService.h"

#include "Logger/czmp_log_protocol.pb.h"

using namespace log4cplus;
using ::testing::NotNull;
using ::testing::_;

class LogerTestCase : public testing::Test
{
public:
	LogerTestCase() {
		printf("Test case create\n");
	}

protected:
	void SetUp() override {
		printf("SetUp\n");
		LoggerService *logger = LoggerService::getInstance();
		logger->removeAllAppender(MAIN_LOG_TAG);
		logger->addLogger(MAIN_LOG_TAG, ALL_LOG_LEVEL);
	}
	void TearDown() override {
		printf("TearDown\n");
		LoggerService::getInstance()->removeAllAppender(MAIN_LOG_TAG);

		google::protobuf::ShutdownProtobufLibrary();
	}
};


class LogVerifier {
public:
	virtual ~LogVerifier() {}
	virtual void verify(const log4cplus::LogLevel &level, const std::string &logModule,
		const int &line, const std::string &logMsg, const std::string &logData) = 0;
};

class LogVerifyMock : public LogVerifier {
public:
	virtual ~LogVerifyMock() {}

	MOCK_METHOD5(verify, void(const log4cplus::LogLevel &level, const std::string &logModule,
		const int &line, const std::string &logMsg, const std::string &logData));
};

class TestAppender : public log4cplus::Appender
{
public:
	TestAppender(LogVerifier *verifier) : mVerifier(verifier) {}
	~TestAppender() {
		destructorImpl(); 
	}

protected:
	virtual void append(const log4cplus::spi::InternalLoggingEvent& event);
	virtual void close() {}
private:
	LogVerifier *mVerifier;
};

void TestAppender::append(const log4cplus::spi::InternalLoggingEvent& event)
{
	log4cplus::LogLevel level = event.getLogLevel();

	std::string logTime = LOG4CPLUS_TSTRING_TO_STRING(
		helpers::getFormattedTime(L"%Y-%m-%d %H:%M:%S.%q", event.getTimestamp())
	);

	std::string logModule = LOG4CPLUS_TSTRING_TO_STRING(event.getFile());
	int pos = logModule.find_last_of('\\');
	if (pos != std::string::npos) {
		logModule = logModule.substr(pos + 1);
	}
	pos = logModule.find_last_of(".");
	logModule = logModule.substr(0, pos);

	int line = event.getLine();
	std::string logMsg = LOG4CPLUS_TSTRING_TO_STRING(event.getMessage());
	std::string data = LOG4CPLUS_TSTRING_TO_STRING(event.getFunction());

	std::cout << logMsg << std::endl;

	mVerifier->verify(level, logModule, line, logMsg, data);
}

TEST_F(LogerTestCase, TestClientSend) {
	LogVerifyMock verifier;
	SharedAppenderPtr testAppend(new TestAppender(&verifier));

	testAppend->setName(L"append for format test");
	log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(MAIN_LOG_TAG));
	logger.addAppender(testAppend);

	LOGT(MAIN_LOG_TAG, "");

	EXPECT_CALL(verifier, 
		verify(log4cplus::TRACE_LOG_LEVEL, THIS_MODULE, _, "loger test", ""))
		.Times(1);
	LOGT(MAIN_LOG_TAG, "loger test");

	EXPECT_CALL(verifier,
		verify(log4cplus::DEBUG_LOG_LEVEL, THIS_MODULE, _, "loger test 1", ""))
		.Times(1);
	LOGD(MAIN_LOG_TAG, "loger test %d", 1);

	EXPECT_CALL(verifier,
		verify(log4cplus::INFO_LOG_LEVEL, THIS_MODULE, _, "loger test 33333", ""))
		.Times(1);
	LOGI(MAIN_LOG_TAG, "loger test %s", "33333");

	EXPECT_CALL(verifier,
		verify(log4cplus::WARN_LOG_LEVEL, THIS_MODULE, _, "loger test 1 33333", ""))
		.Times(1);
	LOGW(MAIN_LOG_TAG, "loger test %d %s", 1, "33333");

	EXPECT_CALL(verifier,
		verify(log4cplus::ERROR_LOG_LEVEL, THIS_MODULE, _, "loger test 中文测试", ""))
		.Times(1);
	LOGE(MAIN_LOG_TAG, "loger test %s", "中文测试");

	EXPECT_CALL(verifier,
		verify(log4cplus::FATAL_LOG_LEVEL, THIS_MODULE, _, "loger test 666 中文测试 2", ""))
		.Times(1);
	LOGF(MAIN_LOG_TAG, "loger test %s %s %d", "666", "中文测试", 2);

}

TEST_F(LogerTestCase, TestClientSendWithBuff) {
	LogVerifyMock verifier;
	SharedAppenderPtr testAppend(new TestAppender(&verifier));

	testAppend->setName(L"Buff appender");
	log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(MAIN_LOG_TAG));
	logger.addAppender(testAppend);

	EXPECT_CALL(verifier,
		verify(log4cplus::TRACE_LOG_LEVEL, THIS_MODULE, _, "loger test", ""))
		.Times(1);
	LOGT_BUFF(MAIN_LOG_TAG, nullptr, 0, "loger test");

	uint8_t buff1[1];
	buff1[0] = 0x03;
	EXPECT_CALL(verifier,
		verify(log4cplus::DEBUG_LOG_LEVEL, THIS_MODULE, _, "loger test 1", "03 "))
		.Times(1);
	LOGD_BUFF(MAIN_LOG_TAG, buff1, 1, "loger test %d", 1);

	uint8_t buff2[10];
	std::stringstream ss;
	for (int i = 0; i < 10; ++i) {
		buff2[i] = i;
		ss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << i << " ";
	}
	
	printf("Buff2: %s\n", toHexString(buff2, 10).c_str());
	printf("Data:  %s\n", ss.str().c_str());
	EXPECT_CALL(verifier,
		verify(log4cplus::INFO_LOG_LEVEL, THIS_MODULE, _, "loger test 33333", ss.str().c_str()))
		.Times(1);
	LOGI_BUFF(MAIN_LOG_TAG, buff2, 10, "loger test %s", "33333");

	EXPECT_CALL(verifier,
		verify(log4cplus::WARN_LOG_LEVEL, THIS_MODULE, _, "loger test 1 33333", ss.str().c_str()))
		.Times(1);
	LOGW_BUFF(MAIN_LOG_TAG, buff2, 10, "loger test %d %s", 1, "33333");

	EXPECT_CALL(verifier,
		verify(log4cplus::ERROR_LOG_LEVEL, THIS_MODULE, _, "loger test 中文测试", ss.str().c_str()))
		.Times(1);
	LOGE_BUFF(MAIN_LOG_TAG, buff2, 10, "loger test %s", "中文测试");

	uint8_t buff3[4096];
	std::stringstream ss2;
	for (int i = 0; i < 4096; ++i) {
		uint8_t tmp = 3;
		buff3[i] = tmp;
		ss2 << "03 ";
		//ss2 << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << tmp << " ";
	}
	EXPECT_CALL(verifier,
		verify(log4cplus::FATAL_LOG_LEVEL, THIS_MODULE, _, "loger test 666 中文测试 2", ss2.str().c_str()))
		.Times(1);
	LOGF_BUFF(MAIN_LOG_TAG, buff3, 4096, "loger test %s %s %d", "666", "中文测试", 2);
}

TEST_F(LogerTestCase, TestConsoleAppender) {
	LoggerService::getInstance()->addConsoleAppender(MAIN_LOG_TAG);
	LOGT(MAIN_LOG_TAG, "loger test");
	LOGD(MAIN_LOG_TAG, "loger test %d", 1);
	LOGI(MAIN_LOG_TAG, "loger test %s", "33333");
	LOGW(MAIN_LOG_TAG, "loger test %d %s", 1, "abcedfg");
	LOGE(MAIN_LOG_TAG, "loger test %s", "中文测试");
	LOGF(MAIN_LOG_TAG, "loger test %s %s %d", "abcedfg", "中文测试", 2);

	uint8_t tmp[10];
	for (int i = 0; i < 10; ++i) {
		tmp[i] = i;
	}
	LOGI_BUFF(MAIN_LOG_TAG, tmp, 10, "loger test %s", "33333");
}

TEST_F(LogerTestCase, DISABLED_TestFileAppender) {
	LoggerService::getInstance()->addFileAppender(MAIN_LOG_TAG,
		"C:\\Workspace\\sources\\VSProject\\IntevioAppServer\\Debug\\LogArch\\test", 
		5);

	size_t times = 60 * 10;
	for (size_t count = 0; count < times; ++count) {
		LOGT(MAIN_LOG_TAG, "loger test");
		LOGD(MAIN_LOG_TAG, "loger test %d", 1);
		LOGI(MAIN_LOG_TAG, "loger test %s", "33333");
		LOGW(MAIN_LOG_TAG, "loger test %d %s", 1, "abcedfg");
		LOGE(MAIN_LOG_TAG, "loger test %s", "中文测试");
		LOGF(MAIN_LOG_TAG, "loger test %s %s %d", "abcedfg", "中文测试", 2);

		uint8_t tmp[4096];
		for (int i = 0; i < 4096; ++i) {
			tmp[i] = i;
		}
		LOGI_BUFF(MAIN_LOG_TAG, tmp, 4096, "loger test %s", "33333");
		Sleep(1000);

		printf("File appender %d\n", count);
	}
}

std::string getTimeString(int duration = 0)
{
	log4cplus::helpers::Time timestamp = log4cplus::helpers::now();
	if (duration > 0) {
		time_t nowTime = log4cplus::helpers::to_time_t(timestamp);
		nowTime += duration;
		timestamp = log4cplus::helpers::from_time_t(nowTime);
	}
	
	return LOG4CPLUS_TSTRING_TO_STRING(
		helpers::getFormattedTime(L"%Y-%m-%d %H:%M:%S", timestamp)
	);
}

inline void initSqliteLogTest(bool immediateFlush = true)
{
	const char *testDbFile =
		"C:\\Workspace\\sources\\VSProject\\IntevioAppServer\\Debug\\Data\\test_log.db";
	zsys_file_delete(testDbFile);
	
	LoggerService *service = LoggerService::getInstance();
	service->addSqliteAppender(MAIN_LOG_TAG,
		testDbFile,
		1000,
		60,
		immediateFlush);
}

std::vector<LogItem> getLogs(const std::string &startTime, const std::string &endTime,
	int maxRow, log4cplus::LogLevel startLevel, std::string fliter = "")
{
	LogRequestParams params;
	params.maxRows = maxRow;
	params.startLevel = startLevel;

	params.dateStart = startTime;// getTimeString(0);
	params.dateEnd = endTime; // getTimeString(10);

	params.fliter = fliter;

	std::vector<LogItem> logs;
	logs.reserve(params.maxRows);

	LoggerService *service = LoggerService::getInstance();
	service->getSqliteLog(MAIN_LOG_TAG, params, logs);

	return logs;
}

inline void verifyLog(
	LogItem &logItem, log4cplus::LogLevel level, std::string logMsg, std::string logdata = "")
{
	ASSERT_EQ(logItem.logModule, std::string(THIS_MODULE));
	ASSERT_EQ(logItem.logLevel, level);
	ASSERT_EQ(logItem.logMsg, logMsg);

	ASSERT_EQ(logItem.logData.size(), logdata.size());
	if (!logdata.empty())
		ASSERT_EQ(logItem.logData, logdata);
}

TEST_F(LogerTestCase, TestSqliteAppenderImmediateFlush) {
	
	initSqliteLogTest();

	std::string now = getTimeString(0);
	LOGT(MAIN_LOG_TAG, "loger test");
	std::vector<LogItem> logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 0);

	now = getTimeString(0);
	LOGD(MAIN_LOG_TAG, "loger test %d", 1);
	logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 0);

	now = getTimeString(0);
	LOGI(MAIN_LOG_TAG, "loger test %s", "33333");
	Sleep(50);
	LOGW(MAIN_LOG_TAG, "loger test %d %s", 1, "abcedfg");
	Sleep(50);
	LOGE(MAIN_LOG_TAG, "loger test %s", "中文测试");
	Sleep(50);
	LOGF(MAIN_LOG_TAG, "loger test %s %s %d", "abcedfg", "中文测试", 2);
	Sleep(50);

	logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 4);

	verifyLog(logs[0], FATAL_LOG_LEVEL, std::string("loger test abcedfg 中文测试 2"));
	verifyLog(logs[1], ERROR_LOG_LEVEL, std::string("loger test 中文测试"));
	verifyLog(logs[2], WARN_LOG_LEVEL, std::string("loger test 1 abcedfg"));
	verifyLog(logs[3], INFO_LOG_LEVEL, std::string("loger test 33333"));
}

TEST_F(LogerTestCase, TestSqliteAppenderImmediateFlushWithData) {
	initSqliteLogTest();

	uint8_t tmp[4096];
	std::stringstream ss;
	for (int i = 0; i < 4096; ++i) {
		tmp[i] = i;
		if (i < 100) {
			char hexString[4];
			memset(hexString, 0x00, 4);
			sprintf(hexString, "%02X ", tmp[i]);
			ss << hexString;
		}		
	}
	ss << "...";

	std::string now = getTimeString();
	LOGI_BUFF(MAIN_LOG_TAG, tmp, 4096, "loger test %s", "33333");

	std::vector<LogItem> logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 1);

	verifyLog(logs[0], INFO_LOG_LEVEL, std::string("loger test 33333"), ss.str());
}


TEST_F(LogerTestCase, TestSqliteAppenderDelayFlushWithTimeout) {
	initSqliteLogTest(false);

	uint8_t tmp[4096];
	std::stringstream ss;
	for (int i = 0; i < 4096; ++i) {
		tmp[i] = i;
		if (i < 100) {
			char hexString[4];
			memset(hexString, 0x00, 4);
			sprintf(hexString, "%02X ", tmp[i]);
			ss << hexString;
		}
	}
	ss << "...";

	std::string now = getTimeString();
	LOGI_BUFF(MAIN_LOG_TAG, tmp, 4096, "loger test %s", "33333");

	std::vector<LogItem> logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 0);

	Sleep(1000);
	logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 1);
	verifyLog(logs[0], INFO_LOG_LEVEL, std::string("loger test 33333"), ss.str());
}

TEST_F(LogerTestCase, DISABLED_TestSqliteAppenderDelayFlushWithFull) {
	initSqliteLogTest(false);

	std::string now = getTimeString();
	for (size_t i = 0; i < 3000; ++i) {
		LOGI(MAIN_LOG_TAG, "loger test %s", "33333");
	}

	std::vector<LogItem> logs = getLogs(now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL);
	ASSERT_EQ(logs.size(), 10);
	verifyLog(logs[0], INFO_LOG_LEVEL, std::string("loger test 33333"));
}


TEST_F(LogerTestCase, DISABLED_TestSqliteAppenderMaintian) {
	initSqliteLogTest(false);

	uint8_t tmp[4096];
	std::stringstream ss;
	for (int i = 0; i < 4096; ++i) {
		tmp[i] = i;
		if (i < 100) {
			char hexString[4];
			memset(hexString, 0x00, 4);
			sprintf(hexString, "%02X ", tmp[i]);
			ss << hexString;
		}
	}
	ss << "...";

	std::string now = getTimeString();
	for (size_t i = 0; i < 13000; ++i) {
		LOGI_BUFF(MAIN_LOG_TAG, tmp, 4096, "loger test %s", "33333");
	}

	Sleep(500);
	std::vector<LogItem> logs = getLogs(now, getTimeString(10), 4000, log4cplus::INFO_LOG_LEVEL);
	ASSERT_TRUE(!logs.empty());
	verifyLog(logs[0], INFO_LOG_LEVEL, std::string("loger test 33333"));
}


TEST_F(LogerTestCase, TestSqliteAppenderGetLogWithFilter) {
	initSqliteLogTest(true);

	std::string now = getTimeString(0);
	LOGI(MAIN_LOG_TAG, "loger test %s", "33333");
	LOGW(MAIN_LOG_TAG, "loger test %d %s", 1, "abcedfg");
	LOGE(MAIN_LOG_TAG, "loger test %s", "中文测试");
	Sleep(10);
	LOGF(MAIN_LOG_TAG, "loger test %s %s %d", "abcedfg", "中文测试", 2);

	Sleep(100);
	std::vector<LogItem> logs = getLogs(
		now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL, "33333");
	ASSERT_EQ(logs.size(), 1);
	verifyLog(logs[0], INFO_LOG_LEVEL, std::string("loger test 33333"));

	logs = getLogs(
		now, getTimeString(10), 10, log4cplus::INFO_LOG_LEVEL, "中文测试");
	ASSERT_EQ(logs.size(), 2);
	verifyLog(logs[0], FATAL_LOG_LEVEL, std::string("loger test abcedfg 中文测试 2"));
	verifyLog(logs[1], ERROR_LOG_LEVEL, std::string("loger test 中文测试"));
}
