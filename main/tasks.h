#ifndef _TASKS_H_
#define _TASKS_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void gpio_task(void* param);
void i2c_task(void* param);
void bp32_task(void* params);

#ifdef __cplusplus
}
#endif

#endif // _TASKS_H_