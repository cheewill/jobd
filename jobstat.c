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

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "database.h"
#include "job.h"
#include "logger.h"

static void
usage(void) 
{
	printf("todo\n");
}

static int
renderer(void *unused, int cols, char **values, char **names)
{
	int i;

	(void)unused; (void)names;

	for (i = 0; i < cols; i++) {
		printf("%s\n", values[i] ? values[i] : "NULL");
	}
	return (0);
}

int
print_all_jobs(void)
{
	int rv;
	char *sql = "SELECT job_id FROM jobs ORDER BY job_id";
	char *err_msg = NULL;

	printf("\033[1m\033[4m%s\033[0m\n", "JobID");

	rv = sqlite3_exec(dbh, sql, renderer, "some stuff", &err_msg);
	if (rv != SQLITE_OK) {
		printlog(LOG_ERR, "Database error: %s", err_msg);
		free(err_msg);
		return (-1);
	}

	return (0);
}

int
main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "fv")) != -1) {
		switch (c) {
		case 'f':
				break;
		case 'v':
				break;
		default:
				fputs("unrecognized command option", stderr);
				usage();
				exit(EXIT_FAILURE);
				break;
		}
	}

	if (logger_init() < 0)
		errx(1, "logger_init");
	
	if (db_init() < 0)
		errx(1, "logger_init");

	if (db_open(NULL, true))
		errx(1, "db_open");

	if (print_all_jobs() < 0)
		exit(EXIT_FAILURE);
		
	exit(EXIT_SUCCESS);
}