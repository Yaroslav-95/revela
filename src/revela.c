#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "fs.h"
#include "log.h"
#include "site.h"
#include "config.h"
#include "bstree.h"

static const char *usage =
	"Usage: %s [options] [-i <input dir>] -o <output dir>\n";

static struct site site = {0};
static enum log_level loglvl = LOG_DETAIL;

static void
bad_arguments(const char *cmd)
{
	fprintf(stderr, usage, cmd);
	exit(1);
}

static void
parse_arguments(int argc, char *argv[])
{
	int opt;
	char *cmd = argv[0];
	while ((opt = getopt(argc, argv, "i:o:n")) != -1) {
		switch (opt) {
		case 'i':
			site.root_dir = strdup(optarg);
			break;
		case 'o':
			site.output_dir = optarg;
			break;
		case 'n':
			site.dry_run = true;
			break;
		default:
			bad_arguments(cmd);
		}
	}
	if (site.output_dir == NULL) {
		bad_arguments(cmd);
	}
}

int
main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	parse_arguments(argc, argv);

#ifdef DEBUG
	log_set_verbosity(LOG_DEBUG);
#else
	log_set_verbosity(loglvl);
#endif

	ret = site_init(&site) && site_load(&site) && site_build(&site) 
		? EXIT_SUCCESS : EXIT_FAILURE;

	if (site.dry_run) {
		log_printl(LOG_INFO, "==== [DRY RUN] ====");
	}

	site_deinit(&site);
	return ret;
}
