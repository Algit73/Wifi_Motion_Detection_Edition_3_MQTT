#ifndef __LOG_H__

#define __LOG_H__

#include "esp_log.h"

typedef enum{
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG
}log_level;

int log_initialize(log_level);

#define ERROR	E
#define WARNING	W
#define INFO	I
#define DEBUG	D

#define CONCAT(a, b)	a##b

#define log(level, tag, ...) CONCAT(ESP_LOG, level)(tag, ##__VA_ARGS__)

#endif /* end of include guard: __LOG_H__ */
