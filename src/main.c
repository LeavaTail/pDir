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
#include "gettext.h"
#include "list.h"
#define _(String) gettext (String)

#define CMDLINE_FAILURE	1

static bool print_all;

static struct option const longopts[] =
{
	{"all", no_argument, NULL, 'a'},
	{0,0,0,0}
};

void usage(int status)
{
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

	printf(gettext("hello, world\n"));
	return 0;
}
