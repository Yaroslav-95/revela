#ifndef REVELA_SITE_H
#define REVELA_SITE_H

#include "config.h"
#include "bstree.h"
#include "render.h"
#include "components.h"

#include <wand/magick_wand.h>

#ifndef SITE_DEFAULT_RESOURCES
#define SITE_DEFAULT_RESOURCES "/usr/share/revela"
#endif

#define CSSDIR "css"
#define STATICDIR "static"
#define CONTENTDIR "content"
#define TEMPLATESDIR "templates"
#define DEFAULTALBUM "unorganized"

struct site {
	struct site_config *config;
	MagickWand *wand;
	char *root_dir;
	const char *output_dir;
	char *content_dir;
	/* 
	 * Indicates how many characters after the full root dir path of the input
	 * directory the relative path of the albums + the content dir start. See
	 * site_init()
	 */
	size_t rel_content_dir;
	struct bstree *albums;
	struct render render;
	bool dry_run;
};

bool site_build(struct site *);

bool site_load(struct site *);

bool site_init(struct site *);

void site_deinit(struct site *);

#endif
