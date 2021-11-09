#include "fs.h"

#include "log.h"
#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/sendfile.h>
#endif

#define BUFSIZE 8192

#define CONTENTDIR "content"
#define DEFAULTALBUM "unorganized"

/*
 * File extensions based on which we add files to the list of images to be
 * processed.
 */
static const char *img_extensions[] = {
	"jpg",
	"jpeg",
	"png",
	"tiff",
	NULL,
};

const char *
rbasename(const char *path)
{
	char *delim = strrchr(path, '/');
	if (delim == NULL) {
		return path;
	}
	return delim + 1;
}

bool
nmkdir(const char *path, struct stat *dstat, bool dry)
{
	if (dry) {
		if (stat(path, dstat)) {
			if (errno == ENOENT) {
				log_printl(LOG_DETAIL, "Created directory %s", path);
				return true;
			}
			log_printl_errno(LOG_FATAL, "Can't read %s", path);
			return false;
		}
		if (!S_ISDIR(dstat->st_mode)) {
			log_printl(LOG_FATAL, "%s is not a directory", path);
			return false;
		}
		return true;
	}

	if(mkdir(path, 0755) < 0) {
		if (errno == EEXIST) {
			if (stat(path, dstat)) {
				log_printl_errno(LOG_FATAL, "Can't read %s", path);
				return false;
			}
			if (!S_ISDIR(dstat->st_mode)) {
				log_printl(LOG_FATAL, "%s is not a directory", path);
				return false;
			}
		} else {
			log_printl_errno(LOG_FATAL, "Can't make directory %s", path);
			return false;
		}
	} else {
		log_printl(LOG_DETAIL, "Created directory %s", path);
	}

	return true;
}

char *
joinpath(const char *restrict a, const char *restrict b)
{
	char *fpath = malloc(strlen(a) + strlen(b) + 2);
	joinpathb(fpath, a, b);

	return fpath;
}

bool
isimage(const char *fname)
{
	char *ext = strrchr(fname, '.');
	if (!ext || *(ext + 1) == '\0') return false;
	
	ext++;
	for (size_t i = 0; img_extensions[i] != NULL; i++) {
		if (!strcasecmp(ext, img_extensions[i])) return true;
	}

	return false;
}

const char *
delext(const char *restrict basename, char *restrict buf, size_t n)
{
	const char *ext = strrchr(basename, '.');
	size_t i = ext - basename;
	if (i + 1 > n) return NULL;
	memcpy(buf, basename, i);
	buf[i] = '\0';

	return ext;
}

int
file_is_uptodate(const char *path, const struct timespec *srcmtim)
{
	struct stat dststat;
	if (stat(path, &dststat)) {
		if (errno != ENOENT) {
			log_printl_errno(LOG_FATAL, "Can't read file %s", path);
			return -1;
		}
		return 0;
	} else if (dststat.st_mtim.tv_sec != srcmtim->tv_sec
			|| dststat.st_mtim.tv_nsec != srcmtim->tv_nsec) {
		return 0;
	}

	return 1;
}

void
setdatetime(const char *path, const struct timespec *mtim)
{
	struct timespec tms[] = {
		{ .tv_sec = mtim->tv_sec, .tv_nsec = mtim->tv_nsec },
		{ .tv_sec = mtim->tv_sec, .tv_nsec = mtim->tv_nsec },
	};
	if (utimensat(AT_FDCWD, path, tms, 0) == -1) {
		log_printl_errno(LOG_ERROR, "Warning: couldn't set times of %s", path);
	}
}

bool
filesync(const char *restrict srcpath, const char *restrict dstpath)
{
	int fdsrc;
	struct stat stsrc;

	fdsrc = open(srcpath, O_RDONLY);
	if (fdsrc < 0) return false;
	if (fstat(fdsrc, &stsrc)) goto dir_error;

	if (S_ISDIR(stsrc.st_mode)) {
		if (mkdir(dstpath, 0755)) {
			if (errno != EEXIST) goto dir_error;
			if (stat(dstpath, &stsrc)) goto dir_error;
			if (!S_ISDIR(stsrc.st_mode)){
				errno = ENOTDIR;
				goto dir_error;
			}
		}
		DIR *dir = fdopendir(fdsrc);
		if (dir == NULL) goto dir_error;
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			char entsrc[PATH_MAX], entdst[PATH_MAX];
			sprintf(entsrc, "%s/%s", srcpath, ent->d_name);
			sprintf(entdst, "%s/%s", dstpath, ent->d_name);
			if (filesync(entsrc, entdst)) {
				closedir(dir);
				goto dir_error;
			}
		}
		closedir(dir);

		goto dir_success;
	}

	int fddst, uptodate;
	if ((uptodate = file_is_uptodate(dstpath, &stsrc.st_mtim)) > 0) {
		goto dir_success;
	} else if (uptodate < 0) {
		goto dir_error;
	}
	fddst = open(dstpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fddst < 0) goto dir_error;

#ifdef __linux__
	ssize_t nwrote = sendfile(fddst, fdsrc, NULL, stsrc.st_size);
	if (nwrote != stsrc.st_size) goto copy_error;
#else
	char buf[BUFSIZE];
	ssize_t nread;

	while ((nread = read(fdsrc, buf, BUFSIZE)) > 0) {
		ssize_t nwrote = write(fddst, buf, nread);
		if (nread != nwrote) goto copy_error;
	}
	if (nread < 0) goto copy_error;
#endif

	struct timespec tms[] = {
		{ .tv_sec = stsrc.st_mtim.tv_sec, .tv_nsec = stsrc.st_mtim.tv_nsec },
		{ .tv_sec = stsrc.st_mtim.tv_sec, .tv_nsec = stsrc.st_mtim.tv_nsec },
	};
	futimens(fddst, tms);

	log_printl(LOG_DETAIL, "Copied %s", srcpath);

	close(fddst);
dir_success:
	close(fdsrc);
	return true;
copy_error:
	close(fddst);
dir_error:
	close(fdsrc);
	return false;
}
