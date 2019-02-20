#ifndef QUECTEL_LOG_H
#define QUECTEL_LOG_H

#if 0
#include <android/log.h>
#include <syslog.h>

#define TAG "upgrade"
#define QFLASH_LOGD(fmt, args...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args))
#else
#define 	QFLASH_LOGD(format, args...) dbg_time(format, ##args); 
#endif

#define QUECTEL_TEST

#ifdef QUECTEL_TEST
#define LOG_FUN	dbg_time("%s -- %d\n", __func__, __LINE__);
#else
#define LOG_FUN
#endif

#endif
