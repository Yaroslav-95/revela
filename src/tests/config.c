#include <string.h>

#include "tests/tests.h"
#include "config.h"
#include "log.h"

#define TESTS_DIR "tests"
#define TEST_ALBUM "tests/album.ini"

static void
test_site_config_read_ini(void)
{
	struct site_config *config = site_config_init();
	asserteq(site_config_read_ini(TESTS_DIR, config), true);
	asserteq(strcmp(config->title, "An example gallery"), 0);
	asserteq(strcmp(config->base_url, "http://www.example.com/photos"), 0);
	asserteq(config->max_previews, 20);
	asserteq(config->images.strip, false);
	asserteq(config->images.quality, 80);
	asserteq(config->images.max_width, 3000);
	asserteq(config->images.max_height, 2000);
	asserteq(config->images.smart_resize, true);
	asserteq(config->thumbnails.strip, true);
	asserteq(config->thumbnails.quality, 75);
	asserteq(config->thumbnails.max_width, 400);
	asserteq(config->thumbnails.max_height, 270);
	asserteq(config->thumbnails.smart_resize, true);
	site_config_destroy(config);
}

static void
test_album_config_read_ini(void)
{
	struct album_config *config = album_config_init();
	asserteq(album_config_read_ini(TEST_ALBUM, config), true);
	asserteq(strcmp(config->title, "An example album"), 0);
	asserteq(strcmp(config->desc, "Example description"), 0);
	album_config_destroy(config);
}

static void
init(void)
{
	log_set_verbosity(LOG_SILENT);
}

int
main(void)
{
	INIT_TESTS();
	init();
	RUN_TEST(test_site_config_read_ini);
	RUN_TEST(test_album_config_read_ini);
}
