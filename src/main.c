/*
 * main.c
 *
 * Print Directory entry (like 'ls').
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <config.h>
#include <getopt.h>
#include <limits.h>
#include "gettext.h"
#include "list.h"
#define _(String) gettext (String)
#define PROGRAM_NAME	"pdir"

#define ALLOCATION_FAILURE	1
#define CMDLINE_FAILURE	2

#define GETOPT_HELP_CHAR	(CHAR_MIN - 2)

static bool print_all;

static struct option const longopts[] =
{
	{"all", no_argument, NULL, 'a'},
	{"help",no_argument, NULL, GETOPT_HELP_CHAR},
	{0,0,0,0}
};

void usage(int status)
{
	FILE *out;
	switch(status) {
		case CMDLINE_FAILURE:
			out = stderr;
			break;
		default:
			out = stdout;
	}
	fprintf(out, _("Usage: %s [OPTION]... [FILE]...\n"),
			PROGRAM_NAME);

	exit(status);
}

static int decode_cmdline(int argc, char **argv)
{
	int longindex = 0;
	int opt = 0;
	int i;

	while((opt = getopt_long(argc, argv,
		"a",
		longopts, &longindex)) != -1) {
		switch(opt) {
			case 'a':
				print_all = true;
				break;
			case GETOPT_HELP_CHAR:
				usage(EXIT_SUCCESS);
				break;
			default:
				usage(CMDLINE_FAILURE);
		}
	}

	init_list();
	for(i = optind; i < argc; i++) {
		add_list(argv[i], strlen(argv[i]));
	}

	return optind;
}

int main(int argc, char *argv[])
{
	int optind;
	int n_files;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	optind = decode_cmdline(argc, argv);
	n_files = argc - optind;

	if(n_files <= 0) {

	} else {

	}

	return 0;
}
