#ifndef REVELA_RENDER_H
#define REVELA_RENDER_H

#include "config.h"
#include "components.h"

#include "roscha.h"

#include <sys/stat.h>

/* Variables that are common to all templates */
struct base_template {
	/* Title of the gallery */
	struct roscha_object *title;
	/* The base url or index url of the gallery */
	struct roscha_object *index;
};

/* Variables of the index template */
struct index_template {
	/* vector of all albums */
	struct roscha_object *albums;
	/* hmap with the year "name" and a vector of albums (see album template) */
	struct roscha_object *years;
};

/* 
 * Variables for the album template and album elements in the albums vector of
 * the index template.
 */
struct album_template {
	/* The title of the album as stored in album.ini */
	struct roscha_object *title;
	/* The description of the album as stored in album.ini */
	struct roscha_object *desc;
	/* The url to the album */
	struct roscha_object *link;
	/* The date of the oldest photo in the album */
	struct roscha_object *date;
	/* The year of the oldest photo in the album */
	struct roscha_object *year;
	/* vector of n first thumbs; deprecated */
	struct roscha_object *previews;
	/* vector of all thumbs hmaps */
	struct roscha_object *thumbs;
};

/* Variables for the image template and thumb hashmap */
struct image_template {
	/* The date the image was taken/created */
	struct roscha_object *date;
	/* The url to the image file */
	struct roscha_object *source;
	/* The url to the previous image page */
	struct roscha_object *prev;
	/* The url to the next image page */
	struct roscha_object *next;
};

struct render {
	/* Roscha environment */
	struct roscha_env *env;
	struct base_template base;
	struct index_template index;
	/* Modification time for the templates dir */
	struct timespec modtime;
	/* Refcounted vector of years with album hmaps */
	struct roscha_object *years;
	/* Refcounted vector album hmaps */
	struct roscha_object *albums;
	/* Count of the albums that were updated */
	size_t albums_updated;
	/* Whether we should simulate rendering or actually render templates */
	bool dry_run;
};

bool render_make_index(struct render *, const char *path);

bool render_make_album(struct render *r, const char *path,
		const struct album *album);

bool render_make_image(struct render *r, const char *path,
		const struct image *image);

bool render_set_album_vars(struct render *, struct album *);

bool render_init(struct render *, const char *path, struct site_config *,
		struct vector *albums);

void render_deinit(struct render *);

#endif
