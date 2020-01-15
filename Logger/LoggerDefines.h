#pragma once

#include <log4cplus/loglevel.h>

#define MAIN_LOG_TAG "main"

struct LogRequestParams{
	log4cplus::LogLevel startLevel;
	std::string fliter;
	std::string dateStart;
	std::string dateEnd;
	uint32_t maxRows;
};

struct LogItem{
	std::string logTime;		//log����ʱ��
	log4cplus::LogLevel logLevel;		//��־����
	std::string logModule;
	std::string logMsg;
	std::string logData;
};