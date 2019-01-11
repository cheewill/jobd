/*
 * Copyright (c) 2018 Mark Heily <mark@heily.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "job.h"
#include "logger.h"
#include "parser.h"
#include "database.h"

static char *progname;

static void
usage(void)
{
    fprintf(stderr, "usage: %s [-v] [-f path] import|init\n", progname);
    exit(EXIT_FAILURE);
}

static int
import_from_file(const char *path)
{
	struct job_parser *jpr;
	
	if (!(jpr = job_parser_new()))
		return (-1);

	printlog(LOG_DEBUG, "importing job from manifest at %s", path);
	if (parse_job_file(jpr, path) != 0) {
		printlog(LOG_ERR, "error parsing %s", path);
		return (-1);
	}

	if (job_db_insert(jpr) < 0) abort();
	
	job_parser_free(jpr);
	return (0);
}

static int
import_from_directory(const char *configdir)
{
	DIR	*dirp;
	struct dirent *entry;
	char *path;
	int rv = 0;

	printlog(LOG_DEBUG, "importing all jobs in directory: %s", configdir);
	if ((dirp = opendir(configdir)) == NULL)
		err(1, "opendir(3) of %s", configdir);

	while (dirp) {
        errno = 0;
        entry = readdir(dirp);
        if (errno != 0)
            err(1, "readdir(3)");
		if (!entry)
            break;
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		char *extension = strrchr(entry->d_name, '.');
		if (!extension || strcmp(extension, ".toml"))
			continue;
		if (asprintf(&path, "%s/%s", configdir, entry->d_name) < 0)
			err(1, "asprintf");
		printlog(LOG_DEBUG, "parsing %s", path);
		if (import_from_file(path) < 0) {
			printlog(LOG_ERR, "error parsing %s", path);
			free(path);
			rv = -1;
			continue;
		}
		free(path);
	}
	if (closedir(dirp) < 0) {
		err(1, "closedir(3)");
	}

	return (rv);
}

int
import_action(const char *path)
{
	int rv;
	struct stat sb;

	rv = stat(path, &sb);
	if (rv < 0)
		err(1, "stat of %s", path);

	if (db_exec(dbh, "BEGIN TRANSACTION") < 0)
		return (-1);

	if (S_ISDIR(sb.st_mode)) 
		rv = import_from_directory(path);
	else
		rv = import_from_file(path);

	if (rv == 0) {
		if (db_exec(dbh, "COMMIT") < 0)
			return (-1);
		else
			return (0);
	} else {
		(void) db_exec(dbh, "ROLLBACK");
		return (-1);
	}
}

int
main(int argc, char *argv[])
{
	int c;
	char *command = NULL;
	char *f_flag = NULL;

	if (logger_init(NULL) < 0)
		errx(1, "unable to initialize logging");
	if (db_init() < 0)
		errx(1, "unable to initialize database functions");

    progname = basename(argv[0]);
  	while ((c = getopt (argc, argv, "f:hv")) != -1) {
		switch (c) {
			case 'f':
				f_flag = strdup(optarg);
				if (!f_flag)
					err(1, "strdup");
				break;
            case 'h':
                usage();
                break;
			case 'v': 
				logger_set_verbose(1);
				break;
			default:
			    usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	command = argv[0];
	if (!strcmp(command, "init")) {
		if (db_create(NULL, f_flag) < 0)
			errx(1, "unable to create the database");
		exit(EXIT_SUCCESS);
	}

	if (db_open(NULL, DB_OPEN_NO_VOLATILE) < 0)
		errx(1, "unable to open the database");

	if (!strcmp(command, "import")) {
		if (import_action(f_flag ? f_flag : "/dev/stdin") < 0)
			exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
