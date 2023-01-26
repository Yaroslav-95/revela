#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

static enum log_level log_verbosity = LOG_INFO;

void
log_set_verbosity(enum log_level lvl)
{
	log_verbosity = lvl;
}

void
log_printf(enum log_level lvl, const char *restrict fmt, ...)
{
	if (lvl > log_verbosity) return;
	FILE   *out = lvl < LOG_INFO ? stderr : stdout;
	va_list args;
	va_start(args, fmt);
	vfprintf(out, fmt, args);
	va_end(args);
}
