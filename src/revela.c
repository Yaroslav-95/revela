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

#ifndef VERSION
#define VERSION "0.1.0"
#endif

static const char *usage =
	"Usage: %s [OPTIONS] -o <output dir>\n"
	"For more information consult `man 1 revela`\n";

static struct site site = {0};
static enum log_level loglvl = LOG_DETAIL;

static inline void
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
	while ((opt = getopt(argc, argv, "i:o:nhV")) != -1) {
		switch (opt) {
		case 'i':
			site.root_dir = strdup(optarg);
			break;
		case 'o':
			site.output_dir = realpath(optarg, NULL);
			break;
		case 'n':
			site.dry_run = true;
			break;
		case 'h':
			printf(usage, cmd);
			exit(0);
		case 'V':
			printf("revela "VERSION"\n");
			exit(0);
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
	parse_arguments(argc, argv);

#ifdef DEBUG
	log_set_verbosity(LOG_DEBUG);
#else
	log_set_verbosity(loglvl);
#endif

	int ret = site_init(&site) && site_load(&site) && site_build(&site) 
		? EXIT_SUCCESS : EXIT_FAILURE;

	if (site.dry_run) {
		log_printl(LOG_INFO, "==== [DRY RUN] ====");
	}

	site_deinit(&site);

	return ret;
}
