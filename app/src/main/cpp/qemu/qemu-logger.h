#ifndef QEMU_LOGGER_H
#define QEMU_LOGGER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_CAT_INIT,
    LOG_CAT_TCG,
    LOG_CAT_MACHINE,
    LOG_CAT_PERIPH,
    LOG_CAT_WARN,
    LOG_CAT_ERROR,
    LOG_CAT_JNI
} QemuLogCategory;

void qemu_logger_init(void);
void qemu_log(QemuLogCategory cat, const char *fmt, ...);
size_t qemu_logger_get_recent_logs(char *dest, size_t max_bytes);
void qemu_logger_clear(void);

#ifdef __cplusplus
}
#endif

#endif // QEMU_LOGGER_H
