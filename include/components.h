#ifndef REVELA_COMPONENTS_H
#define REVELA_COMPONENTS_H

#include "config.h"

#include "hmap.h"
#include "vector.h"
#include "object.h"

#include <time.h>
#include <sys/stat.h>
#include <libexif/exif-data.h>

#define PHOTO_THUMB_SUFFIX "_thumb"

/* All data related to a single image's files, templates, and pages */
struct image {
	/* The albums this image belongs to */
	struct album *album;
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
	/* Reference counted hashmap with values to be passed to the template */
	struct roscha_object *map;
	/* hashmap with values to be passed to the thumbs vector */
	struct roscha_object *thumb;
	/* 
	 * Whether this image was modified since the last time the gallery was
	 * generated.
	 */
	bool modified;
};

/* All data related to an album's images, templates, and pages */
struct album {
	struct site *site;
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
	/*
	 * List/vector with the images of this album to be sorted by from
	 * older to newer.
	 */
	struct vector *images;
	/* Files/dirs that belong to images and which shouldn't be deleted */
	struct hmap *preserved;
	/* Reference counted hashmap with values to be passed to the template */
	struct roscha_object *map;
	/*
	 * Reference counted vector with refcounted hashmaps of image thumbnails to
	 * be passed to the template.
	 */
	struct roscha_object *thumbs;
	/* Count of images that were actually updated */
	size_t images_updated;
};

struct image *image_new(char *src, const struct stat *, struct album *);

struct image *image_old(struct stat *istat);

int image_cmp(const void *a, const void *b);

void image_destroy(struct image *);

struct album *album_new(struct album_config *, struct site *,
		const char *src, const char *rsrc, const struct stat *);

int album_cmp(const void *a, const void *b);

void album_add_image(struct album *, struct image *);

void album_set_year(struct album *);

void album_destroy(struct album *);

#endif
