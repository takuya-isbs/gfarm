/*
 * $Id$
 */


#include "gfperf-lib.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <string.h>

#include "gfperf-metadata.h"

int posix_flag = 0;
char *testdir = "gfarm:///tmp";
int loop_number = 250;
char *unit = UNIT_OPS;
int unit_flag = UNIT_FLAG_OPS;

static
void
usage(char *argv[])
{
	fprintf(stderr, "%s\n", argv[0]);
#ifdef HAVE_GETOPT_LONG
	fprintf(stderr, "\t [-n, --number <loop number>] \n");
	fprintf(stderr, "\t [-t, --testdir <gfarm url>] \n");
	fprintf(stderr, "\t [-u, --unit  <ops or usec>] \n");
#else
	fprintf(stderr, "\t [-n <loop number>] \n");
	fprintf(stderr, "\t [-t <gfarm url>] \n");
	fprintf(stderr, "\t [-u <ops or usec>] \n");
#endif
}

static
int
parse_opt(int argc, char *argv[])
{
	int r;
	gfarm_error_t e;
	static char *optstr = "hn:t:u:";
#ifdef HAVE_GETOPT_LONG
	int option_index = 0;
	static struct option long_options[] = {
		{"number", 1, 0, 'n'},
		{"testdir", 1, 0, 't'},
		{"unit", 1, 0, 'u'},
		{0, 0, 0, 0}
	};
#endif
	while (1) {
#ifdef HAVE_GETOPT_LONG
		r = getopt_long(argc, argv, optstr,
				long_options, &option_index);
#else
		r = getopt(argc, argv, optstr);
#endif
		if (r < 0)
			break;
		switch (r) {
		case 'h':
			return (GFARM_ERR_INVALID_ARGUMENT);
		case 'n':
			loop_number = atoi(optarg);
			if (loop_number >= 10000000) {
				fprintf(stderr, "%s: too many loop number\n",
					argv[0]);
				exit(1);
			}
			break;
		case 't':
			testdir = optarg;
			break;
		case 'u':
			unit = optarg;
			if (strcmp(unit, UNIT_OPS) == 0)
				unit_flag = UNIT_FLAG_OPS;
			else if (strcmp(unit, UNIT_USEC) == 0)
				unit_flag = UNIT_FLAG_USEC;
			else {
				unit_flag = UNIT_FLAG_UNDEF;
				return (GFARM_ERR_INVALID_ARGUMENT);
			}
			break;
		default:
			return (GFARM_ERR_INVALID_ARGUMENT);
		}
	}

	if (is_file_url(testdir)) {
		posix_flag = 1;
		testdir = &testdir[FILE_URL_PREFIX_LEN];
	} else
		posix_flag = 0;

	if (posix_flag)
		e = is_dir_posix(testdir);
	else
		e = is_dir_gfarm(testdir);
	if (e != GFARM_ERR_NO_ERROR) {
		fprintf(stderr, "%s is not a directory.\n",
			testdir);
		return (e);
	}

	return (GFARM_ERR_NO_ERROR);
}

void
free_directory_names(struct directory_names *p)
{
	int i;
	int n = p->n;

	for (i = 0; i <= n ; i++) {
		if (p->names[i] != NULL)
			free(p->names[i]);
	}
	free(p);
}

struct directory_names *
create_directory_names(int n, char *postfix)
{
	int i, ret, size;
	struct directory_names *p;

	size = sizeof(struct directory_names)+(n+1)*(sizeof(char *));
	p = (struct directory_names *)malloc(size);
	if (p == NULL)
		return (NULL);

	memset(p, 0, size);
	p->n = n;
	for (i = 0; i <= n ; i++)
		p->names[i] = NULL;

	for (i = 0; i <= n ; i++) {
		ret = asprintf(&p->names[i],
			       "%s/test%07d%s",
			       testdir, i, postfix);

		if (ret == -1)
			goto err_return;
	}

	return (p);

err_return:
	free_directory_names(p);
	return (NULL);
}

void
set_number(struct test_results *r, int n)
{
	r->number = n - 1;
}

void
set_start(struct test_results *r)
{
	gettimeofday(&r->start, NULL);
}

void
set_middle(struct test_results *r)
{
	gettimeofday(&r->middle, NULL);
}

void
set_end(struct test_results *r)
{
	gettimeofday(&r->end, NULL);
}

void
calc_result(struct test_results *r)
{
	struct timeval d;

	sub_timeval(&r->middle, &r->start, &d);
	r->startup = (float)(d.tv_sec*1000000 + d.tv_usec);
	sub_timeval(&r->end, &r->middle, &d);
	r->average = ((float)(d.tv_sec*1000000 + d.tv_usec)) / r->number;
}

void
adjust_result(struct test_results *r)
{
	if (unit_flag == UNIT_FLAG_OPS) {
		r->startup = (float)1000000/r->startup;
		r->average = (float)1000000/r->average;
	}
}

int
main(int argc, char *argv[])
{
	gfarm_error_t e;
	struct directory_names *dirs;
	struct directory_names *files;

	e = gfarm_initialize(&argc, &argv);
	if (e != GFARM_ERR_NO_ERROR) {
		fprintf(stderr, "%s: %s\n", argv[0],
			gfarm_error_string(e));
		return (GFARM_ERR_INVALID_ARGUMENT);
	}

	e = parse_opt(argc, argv);
	if (e != GFARM_ERR_NO_ERROR) {
		usage(argv);
		gfarm_terminate();
		return (GFARM_ERR_INVALID_ARGUMENT);
	}

	dirs = create_directory_names(loop_number, ".dir");
	if (dirs == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		gfarm_terminate();
		return (GFARM_ERR_NO_MEMORY);
	}

	files = create_directory_names(loop_number, ".file");
	if (files == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		gfarm_terminate();
		return (GFARM_ERR_NO_MEMORY);
	}

	if (posix_flag)
		do_posix_test(dirs, files);
	else
		do_libgfarm_test(dirs, files);

	free_directory_names(files);
	free_directory_names(dirs);


	e = gfarm_terminate();
	if (e != GFARM_ERR_NO_ERROR) {
		fprintf(stderr, "%s: %s\n", argv[0],
			gfarm_error_string(e));
		return (1);
	}

	return (GFARM_ERR_NO_ERROR);
}
