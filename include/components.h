#ifndef REVELA_COMPONENTS_H
#define REVELA_COMPONENTS_H

#include <time.h>
#include <sys/stat.h>
#include <libexif/exif-data.h>

#include "config.h"
#include "hashmap.h"

#define PHOTO_THUMB_SUFFIX "_thumb"

struct image {
	/* The path to the source image file */
	char *source;
	/* Points to the basename in source */
	const char *basename;
	/* Points to the extension in source */
	const char *ext;
	/* The "url" to the dir where index.html for this image will be located */
	char *url;
	/* The "url" to the image file */
	char *url_image;
	/* The "url" to the thumbnail image file */
	char *url_thumb;
	/* Pointers to the relative urls in their respective urls */
	const char *dst;
	const char *dst_image;
	const char *dst_thumb;
	/* The "raw" exif data extracted from the original file */
	ExifData *exif_data;
	/* Last modified time of source file */
	struct timespec modtime;
	/* The datetime this image was taken in human friendly form */
	char datestr[24];
	/* Same as date but in seconds for easier comparison. See image_set_date() */
	time_t tstamp;
	struct album *album;
	/* Hashmap with values to be passed to the image template */
	struct hashmap *map;
	/* Hashmap with values to be passed to thumbs and previews vectors */
	struct hashmap *thumb;
};

struct album {
	struct album_config *config;
	/* The path to the source directory */
	char *source;
	/* The full url that will be used in the template */
	char *url;
	/* Pointer to the slug part in url that will be used for the new dir */
	const char *slug;
	/* Pointer to the human readable date of the earliest image */
	const char *datestr;
	/* The year of this album in human readable form; used to categorize */
	char year[8];
	/* The date of the album is the date of the earliest image */
	time_t tstamp;
	struct bstree *images;
	/* Hashmap with values to be passed to the template */
	struct hashmap *map;
	/* Vector with hashmaps of images to be passed to the templates */
	struct vector *thumbs;
	struct vector *previews;
};

struct image *image_new(char *src, const struct stat *, struct album *);

int image_cmp(const void *a, const void *b);

void image_destroy(void *data);

struct album *album_new(struct album_config *, struct site_config *,
		const char *src, const char *rsrc, const struct stat *);

int album_cmp(const void *a, const void *b);

void album_add_image(struct album *, struct image *);

void album_set_year(struct album *);

void album_destroy(void *data);

#endif
