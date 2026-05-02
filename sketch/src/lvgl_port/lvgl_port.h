#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


void lvgl_port_init(void);

// LVGL 线程安全锁（在 lvgl_port_init() 之后可用）
void lvgl_port_lock(void);
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif



#endif