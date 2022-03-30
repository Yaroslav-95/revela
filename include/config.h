#ifndef REVELA_CONFIG_H
#define REVELA_CONFIG_H
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define SITE_CONF "site.ini"
#define ALBUM_CONF "album.ini"

struct image_config {
	bool strip;
	uint8_t quality;
	size_t max_width;
	size_t max_height;
	bool smart_resize;
	double blur;
};

struct site_config {
	char *title;
	char *base_url;
	struct image_config images;
	struct image_config thumbnails;
};

struct album_config {
	char *title;
	char *desc;
};

bool site_config_read_ini(const char *wdir, struct site_config *);

bool album_config_read_ini(const char *adir, struct album_config *);

struct site_config *site_config_init(void);

struct album_config *album_config_init(void);

void site_config_destroy(struct site_config *config);

void album_config_destroy(struct album_config *config);

#endif
