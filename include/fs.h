#ifndef REVELA_FS_H
#define REVELA_FS_H

#include <stdbool.h>
#include <sys/stat.h>

#include "hmap.h"

typedef bool (*preremove_fn)(const char *path, void *data);

enum nmkdir_res {
	NMKDIR_ERROR,
	NMKDIR_NOOP,
	NMKDIR_CREATED,
};

/* 
 * Returns a pointer to where the basename of the file is inside path.
 */
const char *rbasename(const char *path);

/* 
 * Makes a new directory if it doesn't exist. If there were errors returns
 * false, otherwise returns true.
 */
enum nmkdir_res nmkdir(const char *path, struct stat *dstat, bool dry);

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

#define TIMEQUAL(a, b) \
	((a).tv_sec == (b).tv_sec && (a).tv_nsec == (b).tv_nsec)

/*
 * -1 if error; 0 if the timestamps are different; 1 if they are equal.
 */
int file_is_uptodate(const char *path, const struct timespec *srcmtim);

/* 
 * Sets access and modification times to the time passed.
 */
void setdatetime(const char *path, const struct timespec *mtim);

bool rmentry(const char *path, bool dry);

/* 
 * Recursively deletes extaneous files from directory, keeping files in the
 * preserved hmap. Returns -1 on error, number of deleted entries on success.
 * The number is not the total number of files on all subdirectories, but only
 * the number of files/dirs deleted from the directory pointed by path.
 */
ssize_t rmextra(const char *path, struct hmap *preserved, preremove_fn,
		void *data, bool dry);

/* 
 * Copies file(s) truncating and overwritting the file(s) in the destination
 * path if they exist and are not a directory, or creating them if they don't
 * exist. If srcpath is a directory, copies all files within that directory
 * recursively. Only copies the regular files that already exist if their
 * timestamps do not match.
 */
bool filesync(const char *restrict srcpath, const char *restrict dstpath,
		struct hmap *preserved, bool dry);

#endif
