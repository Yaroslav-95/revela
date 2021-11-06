#ifndef REVELA_FS_H
#define REVELA_FS_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

/* 
 * Returns a pointer to where the basename of the file is inside path.
 */
const char *rbasename(const char *path);

/* 
 * Makes a new directory if it doesn't exist. If there were errors returns
 * false, otherwise returns true.
 */
bool nmkdir(const char *path, struct stat *dstat, bool dry);

#define joinpathb(buf, a, b) sprintf(buf, "%s/%s", a, b)

/*
 * Joins two paths into one, e.g. /hello, word -> /hello/world
 * This function allocates a new string and returns it.
 */
char *joinpath(const char *restrict a, const char *restrict b);

bool isimage(const char *fname);

/*
 * Removes extension from file name "e.g. pepe.gif -> pepe. Returns a pointer to
 * location of the extension in the string.
 */
const char *delext(const char *restrict basename, char *restrict dest, size_t n);

/*
 * -1 if error; 0 if the timestamps are different; 1 if they are equal.
 */
int file_is_uptodate(const char *path, const struct timespec *srcmtim);

/* Sets access and modification times to the time passed */
void setdatetime(const char *path, const struct timespec *mtim);

#endif
