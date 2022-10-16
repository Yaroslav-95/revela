#include "render.h"

#include <stdio.h>
#include <stdlib.h>

#include "fs.h"
#include "log.h"
#include "site.h"

static void
images_walk(struct vector *images)
{
	size_t i;
	struct image *image;
	size_t last = images->len - 1;

	vector_foreach(images, i, image) {
		roscha_hmap_set_new(image->map, "source", (slice_whole(image->url_image)));
		roscha_hmap_set_new(image->map, "date", (slice_whole(image->datestr)));
		char *url;
		if (i > 0) {
			struct image *prev = images->values[i - 1];
			url = prev->url;
			roscha_hmap_set_new(image->map, "prev", (slice_whole(url)));
		}
		if (i < last) {
			struct image *next = images->values[i + 1];
			url = next->url;
			roscha_hmap_set_new(image->map, "next", (slice_whole(url)));
		}

		roscha_hmap_set_new(image->thumb, "link", (slice_whole(image->url)));
		roscha_hmap_set_new(image->thumb, "source", (slice_whole(image->url_thumb)));

		roscha_vector_push(image->album->thumbs, image->thumb);
	}
}

static inline void
years_push_new_year(struct roscha_object *years, char *yearstr,
		struct roscha_object **year, struct roscha_object **albums)
{
	*year = roscha_object_new(hmap_new_with_cap(8));
	*albums = roscha_object_new(vector_new_with_cap(8));
	roscha_hmap_set_new((*year), "name", (slice_whole(yearstr)));
	roscha_hmap_set(*year, "albums", *albums);
	roscha_vector_push(years, *year);
	roscha_object_unref(*year);
	roscha_object_unref(*albums);
}

static void
years_push_album(struct roscha_object *years, struct album *album)
{
	struct roscha_object *year;
	struct roscha_object *albums;
	if (years->vector->len == 0) {
		years_push_new_year(years, album->year, &years, &albums);
	} else {
		year = years->vector->values[years->vector->len - 1];
		struct roscha_object *yearval = roscha_hmap_get(year, "name");
		if (strcmp(yearval->slice.str, album->year)) {
			years_push_new_year(years, album->year, &year, &albums);
		} else {
			albums = roscha_hmap_get(year, "albums");
		}
	}
	roscha_vector_push(albums, album->map);
}

bool
render_set_album_vars(struct render *r, struct album *album)
{

	if (album->config->title) {
		roscha_hmap_set_new(album->map, "title",
				(slice_whole(album->config->title)));
	}
	if (album->config->desc) {
		roscha_hmap_set_new(album->map, "desc",
				(slice_whole(album->config->desc)));
	}
	roscha_hmap_set_new(album->map, "link", (slice_whole(album->url)));
	roscha_hmap_set_new(album->map, "date", (slice_whole(album->datestr)));
	roscha_hmap_set_new(album->map, "year", (slice_whole(album->year)));

	images_walk(album->images);

	roscha_hmap_set(album->map, "thumbs", album->thumbs);

	years_push_album(r->years, album);
	roscha_vector_push(r->albums, album->map);

	/*
	 * The common vars for the images in this album; used by both the album
	 * template and the image template.
	 */
	roscha_object_unref(roscha_hmap_set(r->env->vars, "album", album->map));

	return true;
}

static bool
render(struct roscha_env *env, const char *tmpl, const char *opath)
{
	bool ok = true;
	sds output = roscha_env_render(env, tmpl);
	size_t outlen = strlen(output);
	FILE *f = fopen(opath, "w");
	if (fwrite(output, 1, outlen, f) != outlen) {
		ok = false;
		log_printl_errno(LOG_FATAL, "Can't write %s", opath);
	}
	fclose(f);
	sdsfree(output);
	return ok;
}

bool
render_make_index(struct render *r, const char *path)
{
	bool ok = true;
	if (r->albums_updated == 0) {
		int isupdate = file_is_uptodate(path, &r->modtime);
		if (isupdate == -1) return false;
		if (isupdate == 1) return true;
	}

	log_printl(LOG_INFO, "Rendering %s", path);

	if (r->dry_run) goto done;

	roscha_hmap_set(r->env->vars, "years", r->years);
	roscha_hmap_set(r->env->vars, "albums", r->years);
	ok = render(r->env, "index.html", path);
	roscha_hmap_unset(r->env->vars, "years");
	roscha_hmap_unset(r->env->vars, "albums");

	setdatetime(path, &r->modtime);
done:
	return ok;
}

bool
render_make_album(struct render *r, const char *path, const struct album *album)
{
	bool ok = true;
	if (album->images_updated == 0 && !album->config_updated) {
		int isupdate = file_is_uptodate(path, &r->modtime);
		if (isupdate == -1) return false;
		if (isupdate == 1) return true;
	}

	log_printl(LOG_INFO, "Rendering %s", path);

	if (r->dry_run) goto done;

	ok = render(r->env, "album.html", path);

	setdatetime(path, &r->modtime);
done:
	return ok;
}

bool
render_make_image(struct render *r, const char *path, const struct image *image)
{
	bool ok = true;

	log_printl(LOG_INFO, "Rendering %s", path);

	if (r->dry_run) goto done;

	roscha_hmap_set(r->env->vars, "image", image->map);
	ok = render(r->env, "image.html", path);
	roscha_hmap_unset(r->env->vars, "image");

	setdatetime(path, &r->modtime);
done:
	return ok;
}

bool
render_init(struct render *r, const char *root, struct site_config *conf,
		struct vector *albums)
{
	char *tmplpath = joinpath(root, TEMPLATESDIR);
	struct stat tstat;
	if (stat(tmplpath, &tstat) == -1) {
		if (errno == ENOENT) {
			free(tmplpath);
			tmplpath = joinpath(SITE_DEFAULT_RESOURCES, TEMPLATESDIR);
		} else {
			log_printl_errno(LOG_FATAL, "Unable to read templates dir");
			return false;
		}
	}

	r->modtime = tstat.st_mtim;

	if (r->dry_run) goto cleanup;

	r->env = roscha_env_new();
	bool ok =  roscha_env_load_dir(r->env, tmplpath);
	if (!ok) {
		struct vector *errors = roscha_env_check_errors(r->env);
		log_printl(LOG_FATAL, "Couldn't initialize template engine: %s",
				errors->values[0]);
		return false;
	}
	r->years = roscha_object_new(vector_new_with_cap(8));
	r->albums = roscha_object_new(vector_new_with_cap(64));

	roscha_hmap_set_new(r->env->vars, "title", (slice_whole(conf->title)));
	roscha_hmap_set_new(r->env->vars, "index", (slice_whole(conf->base_url)));

	//bstree_inorder_walk(albums->root, albums_walk, (void *)r);

cleanup:
	free(tmplpath);
	return true;
}

void
render_deinit(struct render *r)
{
	roscha_env_destroy(r->env);
	roscha_object_unref(r->years);
	roscha_object_unref(r->albums);
}
