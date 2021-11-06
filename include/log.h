#ifndef REVELA_LOG_H
#define REVELA_LOG_H

#include <errno.h>
#include <string.h>

enum log_level {
	LOG_SILENT,
	LOG_FATAL,
	LOG_ERROR,
	LOG_INFO,
	LOG_DETAIL,
	LOG_DEBUG,
};

#ifdef DEBUG
static const char *log_level_names[] = {
	"",
	"FATAL",
	"ERROR",
	"INFO",
	"DETAIL",
	"DEBUG",
};
#endif

void log_set_verbosity(enum log_level);

void log_printf(enum log_level, const char *restrict fmt, ...);

#ifdef DEBUG
#define log_print(v, fmt, ...) \
	log_printf(v, "[%s] %s:%d " fmt, log_level_names[v], \
			__FILE__, __LINE__, ##__VA_ARGS__)
#else
#define log_print(v, fmt, ...) \
	log_printf(v, fmt, ##__VA_ARGS__)
#endif

#ifdef DEBUG
#define log_printl(v, fmt, ...) \
	log_printf(v, "[%s] %s:%d " fmt "\n", log_level_names[v], \
			__FILE__, __LINE__, ##__VA_ARGS__)
#else
#define log_printl(v, fmt, ...) \
	log_printf(v, fmt "\n", ##__VA_ARGS__)
#endif

#ifdef DEBUG
#define log_printl_errno(v, fmt, ...) \
	log_printf(v, "[%s] %s:%d " fmt ": %s\n", log_level_names[v], \
			__FILE__, __LINE__, ##__VA_ARGS__, strerror(errno))
#else
#define log_printl_errno(v, fmt, ...) \
	log_printf(v, fmt ": %s\n", ##__VA_ARGS__, strerror(errno))
#endif

#endif
