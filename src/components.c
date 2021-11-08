#include "components.h"

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "fs.h"
#include "log.h"
#include "site.h"

#define MAXTIME \
	((unsigned long long)1 << ((sizeof(time_t) * CHAR_BIT) - 1)) - 1

#define TSTAMP_CMP(T, a, b) \
	(((T *)a)->tstamp > ((T *)b)->tstamp) - (((T *)a)->tstamp < ((T *)b)->tstamp)

/*
 * Reverses the order of the parent directories, changes '/' and spaces to '-',
 * lowers the case of letters. E.g. "2019/Spain/Santiago de Compostela" ->
 * "santiago-de-compostela-spain-2019". Returns pointer to slug inside of url.
 */
static const char *
slugify(const char *path, const char *base_url, char **url)
{
	size_t baselen = strlen(base_url),
		   pathlen, totlen;

	if (!strcmp(path, CONTENTDIR)) {
		pathlen = strlen(DEFAULTALBUM);
		totlen = baselen + pathlen + 2;
		*url = malloc(totlen);
		strncpy(*url, base_url, baselen);
		(*url)[baselen] = '/';
		strncpy(*url + baselen + 1, DEFAULTALBUM, pathlen + 1);

		return *url + baselen + 1;
	}

	path += strlen(CONTENTDIR) + 1;
	pathlen = strlen(path);
	totlen = baselen + pathlen + 2;
	const char *start = path;
	const char *end = strchr(start, '/');
	*url = malloc(totlen);
	strncpy(*url, base_url, baselen);
	(*url)[baselen] = '/';
	char *slug = *url + baselen + 1;
	size_t offset = pathlen;
	while (end != NULL) {
		offset -= end - start;
		memcpy(slug + offset, start, end - start);
		slug[--offset] = '-';
		start = end + 1;
		end = strchr(start, '/');
	}
	memcpy(slug, start, path + pathlen - start);
	for (size_t i = 0; i < pathlen; i++) {
		if (isspace(slug[i])) {
			slug[i] = '-';
			continue;
		}
		slug[i] = tolower(slug[i]);
	}
	(*url)[totlen - 1] = '\0';
	return slug;
}

static void
image_date_from_stat(struct image *image, const struct stat *pstat, 
		struct tm *date)
{
	image->tstamp = pstat->st_ctim.tv_sec;
	localtime_r(&image->tstamp, date);
}

/*
 * If exif data is present and either the tag DateTimeOriginal or
 * CreateDate/DateTimeDigitized is present, then the date and time are taken
 * from either (DateTimeOriginal takes precedence). Otherwise it uses the file's
 * creation time (st_ctim).
 */
static void
image_set_date(struct image *image, const struct stat *pstat)
{
	struct tm date = {0};

	if (image->exif_data == NULL) {
		log_printl(LOG_DEBUG, "No exif data present in %s", image->source);
		log_printl(LOG_DEBUG, "Using date from stat for file %s", image->source);
		image_date_from_stat(image, pstat, &date);
		goto out;
	}

	ExifEntry *entry = exif_content_get_entry(
			image->exif_data->ifd[EXIF_IFD_EXIF], EXIF_TAG_DATE_TIME_ORIGINAL);
	if (entry == NULL) {
		entry = exif_content_get_entry(
			image->exif_data->ifd[EXIF_IFD_EXIF], EXIF_TAG_DATE_TIME_DIGITIZED);
		if (entry == NULL) {
			log_printl(LOG_DEBUG, "No date exif tags present in %s",
					image->source);
			log_printl(LOG_DEBUG, "Using date from stat for file %s",
					image->source);
			image_date_from_stat(image, pstat, &date);
			goto out;
		}
	}

	char buf[32];
	exif_entry_get_value(entry, buf, 32);
	if (strptime(buf, "%Y:%m:%d %H:%M:%S", &date) == NULL) {
		image_date_from_stat(image, pstat, &date);
		goto out;
	}
	image->tstamp = mktime(&date);

out:
	/* TODO: honor user's locale and/or give an option to set the date format */
	strftime(image->datestr, 24, "%Y-%m-%d %H:%M:%S", &date);
}

struct image *
image_new(char *src, const struct stat *pstat, struct album *album)
{
	struct image *image = calloc(1, sizeof *image);
	if (image == NULL) {
		log_printl_errno(LOG_FATAL, "Memory allocation error");
		return NULL;
	}
	char noext[NAME_MAX + 1];

	image->album = album;
	image->source = src;
	image->basename = rbasename(src);

	if ((image->ext = delext(image->basename, noext, NAME_MAX + 1)) == NULL) {
		log_printl(LOG_FATAL, "Can't read %s, file name too long",
				image->basename);
		free(image);
		return NULL;
	}

	size_t relstart = album->slug - album->url;
	image->url = joinpath(album->url, noext);
	image->url_image = joinpath(image->url, image->basename);
	image->url_thumb = malloc(strlen(image->url) + strlen(PHOTO_THUMB_SUFFIX) +
			strlen(image->basename) + 2);
	image->dst = image->url + relstart;
	image->dst_image = image->url_image + relstart;
	image->dst_thumb = image->url_thumb + relstart;
	sprintf(image->url_thumb, "%s/%s" PHOTO_THUMB_SUFFIX "%s", image->url,
			noext, image->ext);

	image->exif_data = exif_data_new_from_file(image->source);
	image->modtime = pstat->st_mtim;
	image_set_date(image, pstat);
	image->map = hashmap_new_with_cap(8);
	image->thumb = hashmap_new_with_cap(4);

	return image;
}

int
image_cmp(const void *a, const void *b)
{
	return TSTAMP_CMP(struct image, a, b);
}

void
image_destroy(void *data)
{
	struct image *image = data;
	free(image->source);
	free(image->url);
	free(image->url_image);
	free(image->url_thumb);
	if (image->exif_data) {
		exif_data_unref(image->exif_data);
	}
	hashmap_free(image->map);
	hashmap_free(image->thumb);
	free(image);
}

struct album *
album_new(struct album_config *conf, struct site_config *sconf, const char *src, 
		const char *rsrc, const struct stat *dstat)
{
	struct album *album = calloc(1, sizeof *album);
	if (album == NULL) {
		log_printl_errno(LOG_FATAL, "Memory allocation error");
		return NULL;
	}
	album->config = conf;
	album->source = strdup(src);
	album->slug = slugify(rsrc, sconf->base_url, &album->url);
	album->images = bstree_new(image_cmp, image_destroy);
	album->tstamp = MAXTIME;
	album->map = hashmap_new_with_cap(16);
	album->thumbs = vector_new(128);
	album->previews = vector_new(sconf->max_previews);

	return album;
}

int
album_cmp(const void *a, const void *b)
{
	return TSTAMP_CMP(struct album, a, b);
}

void
album_add_image(struct album *album, struct image *image)
{
	if (image->tstamp < album->tstamp) {
		album->tstamp = image->tstamp;
		album->datestr = image->datestr;
	}
	bstree_add(album->images, image);
}

void
album_set_year(struct album *album)
{
	char *delim = strchr(album->datestr, '-');
	size_t n = delim - album->datestr;
	strncpy(album->year, album->datestr, n);
	album->year[n] = '\0';
}

void
album_destroy(void *data)
{
	struct album *album = data;
	if (album->config != NULL) {
		album_config_destroy(album->config);
	}
	free(album->source);
	free(album->url);
	bstree_destroy(album->images);
	hashmap_free(album->map);
	vector_free(album->thumbs);
	vector_free(album->previews);
	free(album);
}
