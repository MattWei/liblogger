#ifndef __CZMQ_DEFINES_H__
#define __CZMQ_DEFINES_H__

#include <string>

#include <czmq.h>

const static std::string ZACTOR_EXIT_CMD = "$TERM";

inline int zmsg_pop_int(zmsg_t *msg)
{
	zframe_t *frame = zmsg_pop(msg);
	int value = 0;
	memcpy(&value, zframe_data(frame), sizeof(int));
	zframe_destroy(&frame);

	return value;
}

inline int zmsg_add_int(zmsg_t *msg, int value)
{
	return zmsg_addmem(msg, &value, sizeof(int));
}

inline std::string zmsg_pop_stdstring(zmsg_t *msg)
{
	char *value = zmsg_popstr(msg);
	if (value == NULL) {
		return "";
	}

	std::string stdValue(value);
	freen(value);
	return stdValue;
}

inline int zmsg_add_stdstring(zmsg_t *msg, const std::string &data)
{
	return zmsg_addstr(msg, data.c_str());
}

#endif