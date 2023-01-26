#include "config.h"

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fs.h"
#include "log.h"
#include "parcini.h"

typedef enum kv_handler_result (*ini_keyvalue_handler_fn)(struct parcini_line *, void *dst);

enum config_key_result {
	CONFIG_KEY_NONE,
	CONFIG_KEY_OK,
	CONFIG_KEY_BADKEY,
	CONFIG_KEY_BADVALUE,
};

enum kv_handler_result {
	KV_HANDLER_OK,
	KV_HANDLER_NOMATCH,
	KV_HANDLER_BADVALUE,
};

static int
site_config_images_keyvalue_handler(struct parcini_line *parsed,
                                    struct image_config *iconfig)
{
	int res = CONFIG_KEY_BADKEY;
	if (!strcmp(parsed->key, "strip")) {
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_BOOLEAN,
		                           &iconfig->strip)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
	}
	if (!strcmp(parsed->key, "quality")) {
		long int temp;
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_INTEGER,
		                           &temp)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
		if (res == CONFIG_KEY_OK) {
			if (temp > 100 || temp < 0) {
				res = CONFIG_KEY_BADVALUE;
			} else {
				iconfig->quality = (uint8_t)temp;
			}
		}
	}
	if (!strcmp(parsed->key, "max_width")) {
		long int temp;
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_INTEGER,
		                           &temp)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
		if (res == CONFIG_KEY_OK) {
			if (temp < 1) {
				res = CONFIG_KEY_BADVALUE;
			} else {
				iconfig->max_width = (size_t)temp;
			}
		}
	}
	if (!strcmp(parsed->key, "max_height")) {
		long int temp;
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_INTEGER,
		                           &temp)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
		if (res == CONFIG_KEY_OK) {
			if (temp < 1) {
				res = CONFIG_KEY_BADVALUE;
			} else {
				iconfig->max_height = (size_t)temp;
			}
		}
	}
	if (!strcmp(parsed->key, "smart_resize")) {
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_BOOLEAN,
		                           &iconfig->smart_resize)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
	}
	if (!strcmp(parsed->key, "blur")) {
		long int temp;
		res = parcini_value_handle(&parsed->value, PARCINI_VALUE_INTEGER,
		                           &temp)
		        ? CONFIG_KEY_OK
		        : CONFIG_KEY_BADVALUE;
		if (res == CONFIG_KEY_OK) {
			if (temp < -100 || temp > 100) {
				res = CONFIG_KEY_BADVALUE;
			} else {
				iconfig->blur = (double)temp / 100;
			}
		}
	}

	return res;
}

#define MATCHSK(s, k, p) !strcmp(s, p->section) && !strcmp(k, p->key)

static enum kv_handler_result
site_config_keyvalue_handler(struct parcini_line *parsed, void *dst)
{
	struct site_config *config = dst;
	enum config_key_result subconf = CONFIG_KEY_NONE;
	if (!strcmp(parsed->section, "images")) {
		subconf = site_config_images_keyvalue_handler(parsed, &config->images);
	} else if (!strcmp(parsed->section, "thumbnails")) {
		subconf = site_config_images_keyvalue_handler(parsed,
		                                              &config->thumbnails);
	}
	switch (subconf) {
	case CONFIG_KEY_OK:
		return KV_HANDLER_OK;
	case CONFIG_KEY_BADKEY:
		goto out;
	case CONFIG_KEY_BADVALUE:
		return KV_HANDLER_BADVALUE;
	default:
		break;
	}
	if (MATCHSK("", "title", parsed)) {
		free(config->title);
		return parcini_value_handle(&parsed->value, PARCINI_VALUE_STRING,
		                            &config->title)
		         ? KV_HANDLER_OK
		         : KV_HANDLER_BADVALUE;
	}
	if (MATCHSK("", "base_url", parsed)) {
		free(config->base_url);
		return parcini_value_handle(&parsed->value, PARCINI_VALUE_STRING,
		                            &config->base_url)
		         ? KV_HANDLER_OK
		         : KV_HANDLER_BADVALUE;
	}

out:
	return KV_HANDLER_NOMATCH;
}

static enum kv_handler_result
album_config_keyvalue_handler(struct parcini_line *parsed, void *dst)
{
	struct album_config *config = dst;
	if (MATCHSK("", "title", parsed)) {
		free(config->title);
		return parcini_value_handle(&parsed->value, PARCINI_VALUE_STRING,
		                            &config->title)
		         ? KV_HANDLER_OK
		         : KV_HANDLER_BADVALUE;
	}
	if (MATCHSK("", "desc", parsed)) {
		free(config->desc);
		return parcini_value_handle(&parsed->value, PARCINI_VALUE_STRING,
		                            &config->desc)
		         ? KV_HANDLER_OK
		         : KV_HANDLER_BADVALUE;
	}

	return KV_HANDLER_NOMATCH;
}

static bool
ini_handler(const char *fpath, ini_keyvalue_handler_fn kvhandler,
            void *dst)
{
	struct parcini_line parsed;
	enum parcini_result res;
	parcini_t *parser = parcini_from_file(fpath);
	if (parser == NULL) {
		log_printl_errno(LOG_FATAL, "Couldn't open %s", fpath);
		return false;
	}

	bool ok = true;
	enum kv_handler_result hres;
	while ((res = parcini_parse_next_line(parser, &parsed)) != PARCINI_EOF
	       && ok) {
		switch (res) {
		case PARCINI_KEYVALUE:
			if ((hres = kvhandler(&parsed, dst)) == KV_HANDLER_BADVALUE) {
				log_printl(LOG_ERROR, "Error parsing %s:%ld, bad value for key"
				                      "%s",
				           fpath, parsed.lineno, parsed.key);
				ok = false;
			} else if (hres == KV_HANDLER_NOMATCH) {
				log_printl(LOG_ERROR,
				           "Warning: key '%s' in section '%s' is not a valid "
				           "section-key combination for %s, ignoring.",
				           parsed.key, parsed.section, fpath);
			}
			continue;

		case PARCINI_EMPTY_LINE:
		case PARCINI_SECTION:
			continue;

		case PARCINI_STREAM_ERROR:
		case PARCINI_MEMORY_ERROR:
			log_printl_errno(LOG_FATAL, "Error reading %s", fpath);
			ok = false;
			break;

		default:
			log_printl(LOG_FATAL, "Error parsing %s:%ld", fpath, parsed.lineno);
			ok = false;
			break;
		}
	}

	parcini_destroy(parser);
	return ok;
}

bool
site_config_read_ini(const char *wdir, struct site_config *config)
{
	if (wdir == NULL) {
		return ini_handler(SITE_CONF, site_config_keyvalue_handler,
		                   config);
	}
	char *fpath = joinpath(wdir, SITE_CONF);
	bool ok = ini_handler(fpath, site_config_keyvalue_handler, config);
	free(fpath);
	return ok;
}

bool
album_config_read_ini(const char *fpath, struct album_config *config)
{
	bool ok = ini_handler(fpath, album_config_keyvalue_handler, config);
	return ok;
}

struct site_config *
site_config_init(void)
{
	struct site_config *config = calloc(1, sizeof *config);
	if (config != NULL) {
		config->images = (struct image_config){
			.strip = true,
			.quality = 80,
			.max_width = 3000,
			.max_height = 2000,
			.smart_resize = true,
			.blur = 0,
		};
		config->thumbnails = (struct image_config){
			.strip = true,
			.quality = 60,
			.max_width = 400,
			.max_height = 270,
			.smart_resize = true,
			.blur = 0.25,
		};
	}

	return config;
}

struct album_config *
album_config_init(void)
{
	struct album_config *config = calloc(1, sizeof *config);
	return config;
}

void
site_config_destroy(struct site_config *config)
{
	free(config->title);
	free(config->base_url);
	free(config);
}

void
album_config_destroy(struct album_config *config)
{
	free(config->title);
	free(config->desc);
	free(config);
}
