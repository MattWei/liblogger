#ifndef __LOGER_CLIENT_H__
#define __LOGER_CLIENT_H__

#include <string>
#include <memory>
#include <iostream>

#include "Base/Unite.h"
#include "LoggerDefines.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/loglevel.h>

#ifndef THIS_MODULE
#define THIS_MODULE __FILE__
#endif

#define LOG_STR_BODY(logger, logModule, logLevel, logEvent, logData)        \
    LOG4CPLUS_SUPPRESS_DOWHILE_WARNING()									\
    do {																	\
		if (!logEvent.empty()) {											\
			log4cplus::Logger const & _l                                    \
				= log4cplus::detail::macros_get_logger (logger);            \
			if (_l.isEnabledFor(logLevel)) {								\
				log4cplus::detail::macro_forced_log (_l,                    \
					logLevel,												\
					LOG4CPLUS_STRING_TO_TSTRING(logEvent.c_str()),          \
					logModule,												\
					__LINE__,												\
					logData.c_str());										\
			}                                                               \
		}																	\
    } while(0)																\
    LOG4CPLUS_RESTORE_DOWHILE_WARNING()

#define LOG_SEND(tag, type, data, msg, ...) do { \
	log4cplus::Logger logger = \
		log4cplus::Logger::getInstance(LOG4CPLUS_STRING_TO_TSTRING(tag)); \
																	\
	size_t size = std::snprintf(NULL, 0, msg, ##__VA_ARGS__) + 1;	\
	std::unique_ptr<char[]> buff(new char[size]);					\
	std::snprintf(buff.get(), size, msg, ##__VA_ARGS__);			\
	std::string log(buff.get(), buff.get() + size - 1);				\
																	\
	LOG_STR_BODY(logger, THIS_MODULE, type, log, data);	\
} while (0)

#define LOGT(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::TRACE_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

#define LOGD(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::DEBUG_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

#define LOGI(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::INFO_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

#define LOGW(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::WARN_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

#define LOGE(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::ERROR_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

#define LOGF(logger, msg, ...) do { \
	LOG_SEND(logger, log4cplus::FATAL_LOG_LEVEL, std::string(""), msg, ##__VA_ARGS__); \
} while (0)

inline std::string toHexString(uint8_t *pData, int nSize)
{
	if (nSize <= 0 || pData == NULL)
		return "";

	int buffSize = nSize * 3 + 1;
	std::vector<char> buffer(buffSize);
	memset(&buffer[0], 0x00, buffSize);

	for (int32_t i = 0, j = 0; i < nSize; i++, j += 3)
	{
		sprintf(&buffer[j], "%02X ", pData[i]);
	}

	std::string sReturn(&buffer[0]);
	return sReturn;
}

#define LOGT_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::TRACE_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#define LOGD_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::DEBUG_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#define LOGI_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::INFO_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#define LOGW_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::WARN_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#define LOGE_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::ERROR_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#define LOGF_BUFF(logger, buff, size, msg, ...) do { \
	LOG_SEND(logger, log4cplus::FATAL_LOG_LEVEL, toHexString(buff, size), msg, ##__VA_ARGS__); \
} while (0)

#endif