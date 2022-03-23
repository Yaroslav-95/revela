#include "site.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "fs.h"
#include "log.h"
#include "hmap.h"

/* TODO: handle error cases for paths that are too long */

#define THUMB_SUFFIX "_thumb"

static const char *index_html = "index.html";

/*
 * Checks where the removed image used to be based on the modification time of
 * the image dir and updates the html of the images between which it used to be,
 * if they haven't been updated yet.
 */
static bool
prerm_imagedir(const char *path, void *data)
{
	struct stat st;
	struct album *album = data;
	char htmlpath[PATH_MAX];

	if (stat(path, &st)) {
		log_printl_errno(LOG_ERROR, "Couldn't stat %s", path);
		return false;
	}
	if (S_ISDIR(st.st_mode)) {
		struct image *old = image_old(&st);
		struct bstnode *imnode = bstree_add(album->images, old),
					   *prev = bstree_predecessor(imnode),
					   *next = bstree_successor(imnode);
		if (prev) {
			struct image *imprev = (struct image *)prev->value;
			if (!imprev->modified) {
				joinpathb(htmlpath, imprev->dst, index_html);
				if (!render_make_image(&album->site->render, htmlpath, imprev)) {
					goto fail;
				}
			}
		}
		if (next) {
			struct image *imnext = (struct image *)next->value;
			if (!imnext->modified) {
				joinpathb(htmlpath, imnext->dst, index_html);
				if (!render_make_image(&album->site->render, htmlpath, imnext)) {
					goto fail;
				}
			}
		}
		bstree_remove(album->images, imnode);
		return true;
fail:
		bstree_remove(album->images, imnode);
		return false;
	}
	return true;
}

static bool
wand_passfail(MagickWand *wand, MagickPassFail status)
{
	if (status != MagickPass) {
		char *desc;
		ExceptionType severity;
		desc = MagickGetException(wand, &severity);
		log_printl(LOG_FATAL, "GraphicsMagick error: %.1024s severity: %d\n",
				desc, severity);
		return false;
	}

	return true;
}

#define TRYWAND(w, f) if (!wand_passfail(w, f)) goto magick_fail

static bool
optimize_image(MagickWand *wand, const char *dst,
		const struct image_config *conf, 
		const struct timespec *srcmtim, bool dry)
{
	int update = file_is_uptodate(dst, srcmtim);
	if (update == -1) return false;
	if (update == 1) return true;

	log_printl(LOG_DETAIL, "Converting %s", dst);
	if (dry) goto out;

	unsigned long nx = conf->max_width, ny = conf->max_height;
	if (conf->strip) {
		TRYWAND(wand, MagickStripImage(wand));
	}
	TRYWAND(wand, MagickSetCompressionQuality(wand, conf->quality));
	unsigned long x = MagickGetImageWidth(wand),
				  y = MagickGetImageHeight(wand);
	/* Resize only if the image is actually bigger. No point in making small
	 * images bigger. */
	if (x > nx || y > ny) {
		if (conf->smart_resize) {
			double ratio = (double)x / y;
			if (x > y) {
				ny = nx / ratio;
			} else {
				nx = ny * ratio;
			}
		}
		TRYWAND(wand, MagickResizeImage(wand, nx, ny, GaussianFilter,
					conf->blur));
	}
	TRYWAND(wand, MagickWriteImage(wand, dst));
	setdatetime(dst, srcmtim);

out:
	return true;
magick_fail:
	return false;
}

static bool
images_walk(struct bstnode *node, void *data)
{
	struct site *site = data;
	struct image *image = node->value;
	struct stat dstat;
	struct timespec ddate = { .tv_sec = image->tstamp, .tv_nsec = 0 };
	char htmlpath[PATH_MAX];
	const char *base = rbasename(image->dst);

	log_printl(LOG_DEBUG, "Image: %s, datetime %s", image->basename, 
			image->datestr);

	if (!nmkdir(image->dst, &dstat, site->dry_run)) return false;

	if (!site->dry_run) {
		TRYWAND(site->wand, MagickReadImage(site->wand, image->source));
	}
	if (!optimize_image(site->wand, image->dst_image, &site->config->images,
			&image->modtime, site->dry_run)) {
		goto magick_fail;
	}
	if (!optimize_image(site->wand, image->dst_thumb, &site->config->thumbnails,
			&image->modtime, site->dry_run)) {
		goto magick_fail;
	}
	if (!site->dry_run) {
		MagickRemoveImage(site->wand);
	}

	joinpathb(htmlpath, image->dst, index_html);
	hmap_set(image->album->preserved, base, (char *)base);

	int isupdate = file_is_uptodate(htmlpath, &site->render.modtime);
	if (isupdate == -1) return false;
	if (isupdate == 0) {
		if (!render_make_image(&site->render, htmlpath, image)) {
			return false;
		}
		image->modified = true;
		image->album->images_updated++;
		/* Check if previous image wasn't updated, if so, render it */
		struct bstnode *prev = bstree_predecessor(node);
		if (prev) {
			struct image *iprev = prev->value;
			if (!iprev->modified) {
				joinpathb(htmlpath, iprev->dst, index_html);
				if (!render_make_image(&site->render, htmlpath, iprev)) {
					return false;
				}
				goto success;
			}
		}

		goto success;
	}

	/*
	 * Render anyway if next image doesn't exist yet in directory or if previous
	 * image was updated.
	 */
	struct bstnode *next = bstree_successor(node), *prev = NULL;
	if (next) {
		struct image *inext = next->value;
		if (access(inext->dst, F_OK) != 0) {
			image->album->images_updated++;
			return render_make_image(&site->render, htmlpath, image);
		}
	}
	if ((prev = bstree_predecessor(node)) != NULL) {
		struct image *iprev = prev->value;
		if (iprev->modified) {
			image->album->images_updated++;
			return render_make_image(&site->render, htmlpath, image);
		}
	}

success:
	setdatetime(image->dst, &ddate);
	return true;
magick_fail:
	return false;
}

static bool
albums_walk(struct bstnode *node, void *data)
{
	struct site *site = data;
	struct album *album = node->value;
	struct stat dstat;
	if (!nmkdir(album->slug, &dstat, site->dry_run)) return false;

	hmap_set(site->album_dirs, album->slug, (char *)album->slug);
	if (!site->dry_run) {
		if (!render_set_album_vars(&site->render, album)) return false;

	}

	log_printl(LOG_DEBUG, "Album: %s, datetime %s", album->slug, album->datestr);
	if (!bstree_inorder_walk(album->images->root, images_walk, site)) {
		return false;
	}

	hmap_set(album->preserved, index_html, (char *)index_html);
	ssize_t deleted = rmextra(album->slug, album->preserved, prerm_imagedir,
			album, site->dry_run);
	if (deleted < 0) {
		log_printl_errno(LOG_ERROR, 
					"Something happened while deleting extraneous files");
	} else {
		album->images_updated += deleted;
	}
	if (album->images_updated > 0) {
		site->render.albums_updated++;
	}

	char htmlpath[PATH_MAX];
	joinpathb(htmlpath, album->slug, index_html);
	if (!render_make_album(&site->render, htmlpath, album)) return false;


	return true;
}

/*
 * Recursively traverse the content directory. If there are images in the
 * directory, "create" an album. If an album.ini was found, then the title and
 * description in that file are used. Otherwise, the date of the album is used
 * as its title. If the images are in the root of the content directory, then a
 * special "unorganized" album will be created. The title and description will
 * be used, but the slug will always be "unorganized".
 */
static bool
traverse(struct site *site, const char *path, struct stat *dstat)
{
	bool ok = true;
	DIR *dir = opendir(path);
	if (!dir) {
		log_printl_errno(LOG_FATAL, "Can't open directory %s", path);
		return false;
	}
	struct dirent *ent;
	struct album_config *album_conf = calloc(1, sizeof *album_conf);
	struct album *album = album_new(album_conf, site, path,
			path + site->rel_content_dir, dstat);
	if (album == NULL) {
		closedir(dir);
		return false;
	}
	while ((ent = readdir(dir))) {
		if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}

		struct stat fstats;
		char *subpath = joinpath(path, ent->d_name);
		if (stat(subpath, &fstats)) {
			log_printl_errno(LOG_FATAL, "Can't read %s", subpath);
			ok = false;
			goto fail;
		}

		if (S_ISDIR(fstats.st_mode)) {
			ok = traverse(site, subpath, &fstats);
			if (!ok) goto fail;
		} else if (!strcmp(ent->d_name, ALBUM_CONF)) {
			ok = album_config_read_ini(subpath, album_conf);
			if (!ok) goto fail;
		} else if (isimage(subpath)) {
			struct image *image = image_new(subpath, &fstats, album);
			if (image == NULL) {
				free(subpath);
				goto fail;
			}
			album_add_image(album, image);
			continue;
		}
		free(subpath);
	}
	
	if (album->images->root != NULL) {
		album_set_year(album);
		bstree_add(site->albums, album);
		closedir(dir);
		return true;
	}

fail:
	album_destroy(album);
	closedir(dir);
	return ok;
}

bool
site_build(struct site *site)
{
	struct stat dstat;
	char staticp[PATH_MAX];
	char startwd[PATH_MAX];
	getcwd(startwd, PATH_MAX);

	if (!nmkdir(site->output_dir, &dstat, false)) return false;

	if (chdir(site->output_dir)) {
		log_printl_errno(LOG_FATAL, "Can't change to directory %s",
				site->output_dir);
		return false;
	}

	if (!bstree_inorder_walk(site->albums->root, albums_walk, (void *)site)) {
		return false;
	}

	if (!render_make_index(&site->render, index_html)) return false;
	hmap_set(site->album_dirs, index_html, (char *)index_html);

	joinpathb(staticp, site->root_dir, STATICDIR);
	if (stat(staticp, &dstat)) {
		if (errno != ENOENT) {
			log_printl_errno(LOG_FATAL, "Couldn't read static dir");
			return false;
		}
		if (rmextra(site->output_dir, site->album_dirs, NULL, NULL,
					site->dry_run) < 0) {
			log_printl_errno(LOG_ERROR,
					"Something happened while deleting extraneous files");
		}
	} else if (!filesync(staticp, site->output_dir, site->album_dirs,
				site->dry_run)) {
		log_printl(LOG_FATAL, "Can't copy static files");
		return false;
	}

	chdir(startwd);
	return true;
}

bool
site_load(struct site *site)
{
	struct stat cstat;
	if (stat(site->content_dir, &cstat)) {
		log_printl_errno(LOG_FATAL, "Can't read %s", site->content_dir);
		return false;
	}

	if (!traverse(site, site->content_dir, &cstat)) return false;

	return render_init(&site->render, site->root_dir, site->config, site->albums);
}

bool
site_init(struct site *site)
{
	site->config = site_config_init();
	if (!site_config_read_ini(site->root_dir, site->config)) return false;
	site->albums = bstree_new(album_cmp, album_destroy);

	if (site->root_dir == NULL) {
		site->root_dir = malloc(PATH_MAX);
		if (getcwd(site->root_dir, PATH_MAX) == NULL) {
			log_printl_errno(LOG_FATAL, "Couldn't get working directory");
			return false;
		}
	}

	site->content_dir = joinpath(site->root_dir, CONTENTDIR); 
	site->rel_content_dir = strlen(site->root_dir) + 1;
	InitializeMagick(NULL);
	site->wand = NewMagickWand();
	site->album_dirs = hmap_new();
	site->render.dry_run = site->dry_run;

	roscha_init();

	return true;
}

void
site_deinit(struct site *site)
{
	if (site->albums) bstree_destroy(site->albums);
	site_config_destroy(site->config);
	free(site->content_dir);
	free(site->root_dir);
	if (site->wand != NULL) {
		DestroyMagickWand(site->wand);
		DestroyMagick();
	}
	if (!site->dry_run) {
		hmap_free(site->album_dirs);
		render_deinit(&site->render);
	}

	roscha_deinit();
}
