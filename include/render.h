#ifndef REVELA_RENDER_H
#define REVELA_RENDER_H

#include <sys/stat.h>

#include "bstree.h"
#include "config.h"
#include "template.h"
#include "components.h"

struct render {
	/* Unja environment */
	struct env *env;
	/* List of years with albums in each year */
	struct vector *years;
	/* List of albums */
	struct vector *albums;
	/* Hashmap used for all templates */
	struct hashmap *common_vars;
	/* Modification time for the templates dir */
	struct timespec modtime;
	size_t albums_updated;
	bool dry_run;
};

bool render_make_index(struct render *, const char *path);

bool render_make_album(struct render *r, const char *path,
		const struct album *album);

bool render_make_image(struct render *r, const char *path,
		const struct image *image);

bool render_set_album_vars(struct render *, struct album *);

bool render_init(struct render *, const char *path, struct site_config *,
		struct bstree *albums);

void render_deinit(struct render *);

#endif
