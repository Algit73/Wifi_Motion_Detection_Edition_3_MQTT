#include "log.h"

#include "esp_log.h"

static log_level level;

int log_initialize(log_level l)
{
	level = l;

	switch(level){
		case LOG_ERROR:
			esp_log_level_set("*", ESP_LOG_ERROR);
			break;
		case LOG_WARNING:
			esp_log_level_set("*", ESP_LOG_WARN);
			break;
		case LOG_INFO:
			esp_log_level_set("*", ESP_LOG_INFO);
			break;
		case LOG_DEBUG:
			esp_log_level_set("*", ESP_LOG_VERBOSE);
			break;
	}

	return 0;
}
