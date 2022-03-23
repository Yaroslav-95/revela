#include "fs.h"

#include "log.h"
#include "config.h"

#include "vector.h"
#include "slice.h"

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

static void
hm_destroy_cb(const struct slice *k, void *v)
{
	free(v);
}

const char *
rbasename(const char *path)
{
	char *delim = strrchr(path, '/');
	if (delim == NULL) {
		return path;
	}
	return delim + 1;
}

enum nmkdir_res
nmkdir(const char *path, struct stat *dstat, bool dry)
{
	if (dry) {
		if (stat(path, dstat)) {
			if (errno == ENOENT) {
				log_printl(LOG_DETAIL, "Created directory %s", path);
				return NMKDIR_CREATED;
			}
			log_printl_errno(LOG_FATAL, "Can't read %s", path);
			return NMKDIR_ERROR;
		}
		if (!S_ISDIR(dstat->st_mode)) {
			log_printl(LOG_FATAL, "%s is not a directory", path);
			return NMKDIR_ERROR;
		}
		return NMKDIR_NOOP;
	}

	if(mkdir(path, 0755) < 0) {
		if (errno == EEXIST) {
			if (stat(path, dstat)) {
				log_printl_errno(LOG_FATAL, "Can't read %s", path);
				return NMKDIR_ERROR;
			}
			if (!S_ISDIR(dstat->st_mode)) {
				log_printl(LOG_FATAL, "%s is not a directory", path);
				return NMKDIR_ERROR;
			}
			return NMKDIR_NOOP;
		} else {
			log_printl_errno(LOG_FATAL, "Can't make directory %s", path);
			return NMKDIR_ERROR;
		}
	}
	log_printl(LOG_DETAIL, "Created directory %s", path);
	return NMKDIR_CREATED;
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
rmentry(const char *path, bool dry)
{
	struct stat st;
	if (stat(path, &st)) {
		log_printl_errno(LOG_ERROR, "Can't stat file %s", path);
		return false;
	}

	if (S_ISDIR(st.st_mode)) {
		DIR *dir = opendir(path);
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}

			char target[PATH_MAX];
			sprintf(target, "%s/%s", path, ent->d_name);
			if (!rmentry(target, dry)) {
				closedir(dir);
				return false;
			}
		}

		closedir(dir);
		if (dry) goto success;
		if (rmdir(path)) goto error;
		goto success;
	}

	if (dry) goto success;
	if (unlink(path)) goto error;

success:
	log_printl(LOG_DETAIL, "Deleting %s", path);
	return true;
error:
	log_printl_errno(LOG_ERROR, "Can't delete %s", path);
	return false;
}

ssize_t
rmextra(const char *path, struct hmap *preserved, preremove_fn cb,
		void *data, bool dry)
{
	ssize_t removed = 0;
	DIR *dir = opendir(path);
	if (dir == NULL) return -1;

	struct dirent *ent;
	while ((ent = readdir(dir))) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}

		if (hmap_get(preserved, ent->d_name) != NULL) continue;

		char target[PATH_MAX];
		sprintf(target, "%s/%s", path, ent->d_name);
		if (cb != NULL) {
			if (!cb(target, data)) return -1;
		}
		if (!rmentry(target, dry)) {
			closedir(dir);
			return -1;
		}
		removed++;
	}

	closedir(dir);
	return removed;
}

bool
filesync(const char *restrict srcpath, const char *restrict dstpath,
		struct hmap *preserved, bool dry)
{
	int fdsrc;
	struct stat stsrc;
	struct vector *own = NULL;
	bool cleanup = false;

	fdsrc = open(srcpath, O_RDONLY);
	if (fdsrc < 0) {
		log_printl_errno(LOG_ERROR, "Couldn't open %s", srcpath);
		return false;
	}
	if (fstat(fdsrc, &stsrc)) {
		log_printl_errno(LOG_ERROR, "Couldn't stat %s", srcpath);
		goto dir_error;
	}

	if (S_ISDIR(stsrc.st_mode)) {
		if (mkdir(dstpath, 0755)) {
			if (errno != EEXIST) {
				log_printl_errno(LOG_ERROR, "Couldn't create directory %s",
						dstpath);
				goto dir_error;
			}
			if (stat(dstpath, &stsrc)) {
				log_printl_errno(LOG_ERROR, "Couldn't stat %s", dstpath);
				goto dir_error;
			}
			if (!S_ISDIR(stsrc.st_mode)){
				log_printl(LOG_ERROR, "%s is not a directory", dstpath);
				errno = ENOTDIR;
				goto dir_error;
			}
			/* We only need to cleanup if the dir already existed */
			cleanup = true;
		}

		if (cleanup) {
			if (preserved == NULL) {
				preserved = hmap_new();
			} else {
				own = vector_new_with_cap(32);
			}
		}

		DIR *dir = fdopendir(fdsrc);
		if (dir == NULL) {
			log_printl_errno(LOG_ERROR, "Failed to open directory %s", dstpath);
			goto dir_error;
		}

		struct dirent *ent;
		while ((ent = readdir(dir))) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			char entsrc[PATH_MAX], entdst[PATH_MAX];
			sprintf(entsrc, "%s/%s", srcpath, ent->d_name);
			sprintf(entdst, "%s/%s", dstpath, ent->d_name);
			if (cleanup) {
				char *name = strdup(ent->d_name);
				hmap_set(preserved, name, name);
				if (own) {
					vector_push(own, name);
				}
			}
			if (!filesync(entsrc, entdst, NULL, dry)) {
				closedir(dir);
				return false;
			}
		}

		if (cleanup) {
			rmextra(dstpath, preserved, NULL, NULL, dry);
			if (own) {
				for (size_t i = 0; i < own->len; i++) {
					free(own->values[i]);
				}
				vector_free(own);
			} else {
				hmap_destroy(preserved, hm_destroy_cb);
			}
		}

		closedir(dir);
		return true;
	}

	int fddst, uptodate;
	if ((uptodate = file_is_uptodate(dstpath, &stsrc.st_mtim)) > 0) {
		goto success;
	} else if (uptodate < 0) {
		goto dir_error;
	}

	log_printl(LOG_DETAIL, "Copying %s", srcpath);

	if (dry) goto success;

	fddst = open(dstpath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fddst < 0) {
		log_printl_errno(LOG_ERROR, "Failed to open/create %s", dstpath);
		goto dir_error;
	}

#ifdef __linux__
	ssize_t nwrote = sendfile(fddst, fdsrc, NULL, stsrc.st_size);
	if (nwrote != stsrc.st_size) {
		log_printl_errno(LOG_ERROR, "Failed to copy %s (wrote %lu/%lu bytes)", 
				dstpath, nwrote, stsrc.st_size);
		goto copy_error;
	}
#else
	char buf[BUFSIZE];
	ssize_t nread;

	while ((nread = read(fdsrc, buf, BUFSIZE)) > 0) {
		ssize_t nwrote = write(fddst, buf, nread);
		if (nread != nwrote) {
			log_printl_errno(LOG_ERROR, "Failed to copy %s "
					"(buffer wrote %lu/%lu bytes)", 
					dstpath, nwrote, nread);
			goto copy_error;
		}
	}
	if (nread < 0) {
		log_printl_errno(LOG_ERROR, "Failed to copy %s");
		goto copy_error;
	}
#endif

	struct timespec tms[] = {
		{ .tv_sec = stsrc.st_mtim.tv_sec, .tv_nsec = stsrc.st_mtim.tv_nsec },
		{ .tv_sec = stsrc.st_mtim.tv_sec, .tv_nsec = stsrc.st_mtim.tv_nsec },
	};
	futimens(fddst, tms);

	close(fddst);
success:
	close(fdsrc);
	return true;
copy_error:
	close(fddst);
dir_error:
	close(fdsrc);
	return false;
}
