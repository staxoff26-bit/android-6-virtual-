#include "qemu-logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <android/log.h>

#define TAG "QemuNativeEngine"
#define LOG_BUFFER_SIZE 65536 // 64 KB ring buffer
#define MAX_LINE_LEN    512

static char s_log_buffer[LOG_BUFFER_SIZE];
static size_t s_log_pos = 0;
static pthread_mutex_t s_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_initialized = 0;

static const char* cat_to_string(QemuLogCategory cat) {
    switch (cat) {
        case LOG_CAT_INIT:    return "INIT";
        case LOG_CAT_TCG:     return "TCG";
        case LOG_CAT_MACHINE: return "MACH";
        case LOG_CAT_PERIPH:  return "DEV";
        case LOG_CAT_WARN:    return "WARN";
        case LOG_CAT_ERROR:   return "ERR";
        case LOG_CAT_JNI:     return "JNI";
        default:              return "INFO";
    }
}

static android_LogPriority cat_to_android_priority(QemuLogCategory cat) {
    switch (cat) {
        case LOG_CAT_WARN:  return ANDROID_LOG_WARN;
        case LOG_CAT_ERROR: return ANDROID_LOG_ERROR;
        default:            return ANDROID_LOG_INFO;
    }
}

void qemu_logger_init(void) {
    pthread_mutex_lock(&s_log_mutex);
    memset(s_log_buffer, 0, sizeof(s_log_buffer));
    s_log_pos = 0;
    s_initialized = 1;
    pthread_mutex_unlock(&s_log_mutex);
}

void qemu_log(QemuLogCategory cat, const char *fmt, ...) {
    char msg[MAX_LINE_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    // Also mirror to Android logcat
    __android_log_print(cat_to_android_priority(cat), TAG, "[%s] %s", cat_to_string(cat), msg);

    // Timestamp
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    char full_line[MAX_LINE_LEN + 64];
    int line_len = snprintf(full_line, sizeof(full_line), "[%4ld.%03ld][%s] %s\n",
                            ts.tv_sec % 10000, ts.tv_nsec / 1000000, cat_to_string(cat), msg);

    pthread_mutex_lock(&s_log_mutex);
    if (!s_initialized) {
        s_log_pos = 0;
        s_initialized = 1;
    }

    if (line_len > 0) {
        size_t len = (size_t)line_len;
        if (s_log_pos + len >= LOG_BUFFER_SIZE) {
            // Wrap or shift: shift left by half buffer to retain recent history
            size_t shift = LOG_BUFFER_SIZE / 2;
            memmove(s_log_buffer, s_log_buffer + shift, s_log_pos - shift);
            s_log_pos -= shift;
            memset(s_log_buffer + s_log_pos, 0, LOG_BUFFER_SIZE - s_log_pos);
        }
        memcpy(s_log_buffer + s_log_pos, full_line, len);
        s_log_pos += len;
        s_log_buffer[s_log_pos] = '\0';
    }
    pthread_mutex_unlock(&s_log_mutex);
}

size_t qemu_logger_get_recent_logs(char *dest, size_t max_bytes) {
    if (!dest || max_bytes == 0) return 0;
    pthread_mutex_lock(&s_log_mutex);
    size_t to_copy = s_log_pos;
    if (to_copy >= max_bytes) {
        to_copy = max_bytes - 1;
    }
    if (to_copy > 0) {
        memcpy(dest, s_log_buffer, to_copy);
    }
    dest[to_copy] = '\0';
    pthread_mutex_unlock(&s_log_mutex);
    return to_copy;
}

void qemu_logger_clear(void) {
    pthread_mutex_lock(&s_log_mutex);
    s_log_pos = 0;
    s_log_buffer[0] = '\0';
    pthread_mutex_unlock(&s_log_mutex);
}
