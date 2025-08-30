/*
 ============================================================================
 Name        : hev-misc.h
 Author      : hev <r@hev.cc>
 Copyright   : Copyright (c) 2022 - 2025 xyz
 Description : Misc
 ============================================================================
 */

#ifndef __HEV_MISC_H__
#define __HEV_MISC_H__

#include <hev-task.h>
#include <hev-task-io.h>

#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x[0]))

#include <stdarg.h>

// 日志和结果输出函数声明（所有平台通用）
void jni_log_output(const char* format, ...);
void jni_result_output(const char* format, ...);
void set_jni_log_functions(void (*log_func)(const char*, va_list), void (*result_func)(const char*, va_list));

// 停止执行检查函数（所有平台通用）
int should_stop_execution(void);
void set_stop_execution(int should_stop);

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "NatmapJNI"
#endif

#define LOG(T) \
    do { \
        jni_log_output("[" #T "] %s %s:%d", __FUNCTION__, __FILE__, __LINE__); \
    } while(0)

#define LOGV(T, F, ...)                                                   \
    do { \
        jni_log_output("[" #T "] %s %s:%d " F, __FUNCTION__, __FILE__, \
                 __LINE__, __VA_ARGS__); \
    } while(0)

#ifndef container_of
#define container_of(ptr, type, member)               \
    ({                                                \
        void *__mptr = (void *)(ptr);                 \
        ((type *)(__mptr - offsetof (type, member))); \
    })
#endif

#define io_yielder hev_io_yielder

/**
 * hev_io_yielder:
 * @type: yield type
 * @data: data
 *
 * The task io yielder.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_io_yielder (HevTaskYieldType type, void *data);

/**
 * hev_reuse_port:
 * @port: port number
 *
 * Set reuse port flag for remote socket file descriptor in other process.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_reuse_port (const char *port);

/**
 * hev_tcp_cca:
 * @algo: congestion control algorithm
 *
 * Set the TCP congestion control algorithm.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_tcp_cca (int fd, const char *algo);

/**
 * hev_run_daemon:
 *
 * Run as daemon.
 *
 * Returns: returns zero on successful, otherwise returns -1.
 */
int hev_run_daemon (void);

#endif /* __HEV_MISC_H__ */
