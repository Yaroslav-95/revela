#include "render.h"

#include <stdio.h>
#include <stdlib.h>

#include "fs.h"
#include "log.h"
#include "site.h"

static bool
images_walk(struct bstnode *node, void *data)
{
	struct image *image = node->value;

	hashmap_insert(image->map, "source", image->url_image);
	hashmap_insert(image->map, "date", image->datestr);
	struct bstnode *prev = bstree_predecessor(node), 
				   *next = bstree_successor(node);
	char *url;
	if (prev) {
		url = ((struct image *)prev->value)->url;
		hashmap_insert(image->map, "prev", url);
	}
	if (next) {
		url = ((struct image *)next->value)->url;
		hashmap_insert(image->map, "next", url);
	}

	hashmap_insert(image->thumb, "link", image->url);
	hashmap_insert(image->thumb, "source", image->url_thumb);

	vector_push(image->album->thumbs, image->thumb);

	return true;
}

static struct hashmap *
years_push_new_year(struct vector *years, char *yearstr)
{
	struct hashmap *year = hashmap_new_with_cap(4);
	struct vector *albums = vector_new(8);
	hashmap_insert(year, "name", yearstr);
	hashmap_insert(year, "albums", albums);
	vector_push(years, year);

	return year;
}

static void
years_push_album(struct vector *years, struct album *album)
{
	struct hashmap *year;
	struct vector *albums;
	if (years->size == 0) {
		year = years_push_new_year(years, album->year);
	} else {
		year = years->values[years->size - 1];
		char *yearstr = hashmap_get(year, "name");
		if (strcmp(yearstr, album->year)) {
			year = years_push_new_year(years, album->year);
		}
	}
	albums = hashmap_get(year, "albums");
	vector_push(albums, album->map);
}

static void
year_walk(const void *k, void *v)
{
	if (!strcmp(k, "albums")) {
		struct vector *albums = v;
		vector_free(albums);
	}
}

static void
years_destroy(struct vector *years)
{
	for (size_t i = 0; i < years->size; i++) {
		struct hashmap *year = years->values[i];
		hashmap_walk(year, year_walk);
		hashmap_free(year);
	}
	vector_free(years);
}

bool
render_set_album_vars(struct render *r, struct album *album)
{

	hashmap_insert(album->map, "title", album->config->title);
	hashmap_insert(album->map, "desc", album->config->desc);
	hashmap_insert(album->map, "link", album->url);
	hashmap_insert(album->map, "date", (char *)album->datestr);
	hashmap_insert(album->map, "year", album->year);

	bstree_inorder_walk(album->images->root, images_walk, NULL);

	for (uint32_t i = 0; 
			i < album->thumbs->size && i < album->previews->cap; i++) {
		vector_push(album->previews, album->thumbs->values[i]);
	}

	hashmap_insert(album->map, "thumbs", album->thumbs);
	hashmap_insert(album->map, "previews", album->previews);

	years_push_album(r->years, album);
	vector_push(r->albums, album->map);

	/*
	 * The common vars for the images in this album; used by both the album
	 * template and the image template.
	 */
	hashmap_insert(r->common_vars, "album", album->map);

	return true;
}

static bool
render(struct env *env, const char *tmpl, const char *opath, 
		struct hashmap *vars)
{
	bool ok = true;
	char *output = template(env, tmpl, vars);
	size_t outlen = strlen(output);
	FILE *f = fopen(opath, "w");
	if (fwrite(output, 1, outlen, f) != outlen) {
		ok = false;
		log_printl_errno(LOG_FATAL, "Can't write %s", opath);
	}
	fclose(f);
	free(output);
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

	hashmap_insert(r->common_vars, "years", r->years);
	hashmap_insert(r->common_vars, "albums", r->years);
	ok = render(r->env, "index.html", path, r->common_vars);
	hashmap_remove(r->common_vars, "years");
	hashmap_remove(r->common_vars, "albums");

	setdatetime(path, &r->modtime);
done:
	return ok;
}

bool
render_make_album(struct render *r, const char *path, const struct album *album)
{
	bool ok = true;
	if (album->images_updated == 0) {
		int isupdate = file_is_uptodate(path, &r->modtime);
		if (isupdate == -1) return false;
		if (isupdate == 1) return true;
	}

	log_printl(LOG_INFO, "Rendering %s", path);

	if (r->dry_run) goto done;

	ok = render(r->env, "album.html", path, r->common_vars);

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

	hashmap_insert(r->common_vars, "image", image->map);
	ok = render(r->env, "image.html", path, r->common_vars);
	hashmap_remove(r->common_vars, "image");

	setdatetime(path, &r->modtime);
done:
	return ok;
}

bool
render_init(struct render *r, const char *root, struct site_config *conf,
		struct bstree *albums)
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

	r->env = env_new(tmplpath);
	if (r->env == NULL) {
		log_printl_errno(LOG_FATAL, "Couldn't initialize template engine");
		return false;
	}
	r->years = vector_new(8);
	r->albums = vector_new(64);

	r->common_vars = hashmap_new_with_cap(16);
	hashmap_insert(r->common_vars, "title", conf->title);
	if (strlen(conf->base_url) == 0) {
		hashmap_insert(r->common_vars, "index", "/");
	} else {
		hashmap_insert(r->common_vars, "index", conf->base_url);
	}

	//bstree_inorder_walk(albums->root, albums_walk, (void *)r);

cleanup:
	free(tmplpath);
	return true;
}

void
render_deinit(struct render *r)
{
	env_free(r->env);
	years_destroy(r->years);
	vector_free(r->albums);
	hashmap_free(r->common_vars);
}
