#ifndef UPTIME_H_
#define UPTIME_H_

#include "FreeRTOS.h"
#include "task.h"

#define getUptimeMs() (xTaskGetTickCount() * portTICK_PERIOD_MS)

#endif /* UPTIME_H_ */