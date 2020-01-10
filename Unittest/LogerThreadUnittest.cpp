#include "pch.h"

#define THIS_MODULE "LogThreadTest"

/*
#include "LogerThread.h"
#include "LogerClient.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/appender.h>
#include <log4cplus/spi/loggingevent.h>

class TestAppender : public log4cplus::Appender
{
public:
	TestAppender() {}
	~TestAppender() {
		destructorImpl();
	}

	void setTestData(const log4cplus::LogLevel &level, const std::string &logModule,
		const int &line, const std::string &logMsg, const std::string &logData) {
		mLevel = level;
		mLogModule = logModule;
		mLine = line;
		mLogMsg = logMsg;
		mLogData = logData;
	}
protected:
	virtual void append(const log4cplus::spi::InternalLoggingEvent& event);
	virtual void close() {}
private:
	void verifyLog(const log4cplus::LogLevel &level, const std::string &logModule,
		const int &line, const std::string &logMsg, const std::string &logData) {
		ASSERT_EQ(mLevel, level);
		ASSERT_EQ(mLogModule, logModule);
		ASSERT_EQ(mLine, line);
		ASSERT_EQ(mLogMsg, logMsg);
		ASSERT_EQ(mLogData, logData);
	}

	log4cplus::LogLevel mLevel;
	std::string mLogModule;
	int mLine; 
	std::string mLogMsg;
	std::string mLogData;
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

	verifyLog(level, logModule, line, logMsg, data);
}

TEST(LogerThreadTestCase, TestLogParams) {

	log4cplus::Initializer();

	log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("test"));
	logger.setLogLevel(ALL_LOG_LEVEL);

	TestAppender *testAppender = new TestAppender();
	SharedAppenderPtr testAppend(testAppender);
	testAppend->setName(L"append for format test");
	logger.addAppender(testAppend);

	std::string testModule("LogerThreadTestCase");
	std::string testMsg("Helloworld");
	std::string testData("22345");
	int line = 86;
	testAppender->setTestData(TRACE_LOG_LEVEL, testModule, line, testMsg, testData);

	LOG_STR_BODY(logger, testModule, line, testMsg, TRACE_LOG_LEVEL, testData);	

}

TEST(LogerThreadTestCase, TestLogerThread) {

	LogerThread logerThread;

	std::string appPath = "C:\\Workspace\\sources\\VSProject\\IntevioAppServer\\Debug\\";
	logerThread.start(appPath, 1000, 10000);

	//for (int j = 0; j < 1000; ++j) {
		for (int i = 0; i < 1; ++i) {
			LOGT("loger test %s %s %d", "666", "TRACE", i);
			LOGD("loger test %s %s %d", "666", "DEBUG", i);
			LOGI("loger test %s %s %d", "666", "INFO", i);
			LOGW("loger test %s %s %d", "666", "WARN", i);
			LOGE("loger test %s %s %d", "666", "ERROR", i);
			LOGF("loger test %s %s %d", "666", "FATA", i);
			Sleep(10);
		}
	//}

}
	*/