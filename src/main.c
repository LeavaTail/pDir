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
#include <sys/stat.h>
#include "gettext.h"
#include "error.h"
#include "list.h"
#define _(String) gettext (String)
#define PROGRAM_NAME	"pdir"

#define ALLOCATION_FAILURE	1
#define CMDLINE_FAILURE	2
#define ACCESS_FAILURE	3
#define ALLOCATE_COUNT	100

#define GETOPT_HELP_CHAR	(CHAR_MIN - 2)

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

static void file_failure (int status, char const *name)
{
	switch(status){
		case ALLOCATION_FAILURE:
			error(status, _("%s: cannot allocate memory"), PROGRAM_NAME);
			break;
		case ACCESS_FAILURE:
			error(status, _("%s: cannot access '%s'"), PROGRAM_NAME, name);
			break;
	}
}

static bool print_all;

static struct option const longopts[] =
{
	{"all", no_argument, NULL, 'a'},
	{"help",no_argument, NULL, GETOPT_HELP_CHAR},
	{0,0,0,0}
};

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

static void joinpath(char *dest, const char *dirname, const char *name)
{
	if(dirname[0] != '.' || dirname[1] != '\0') {
		while(*dirname){
			*dest++ = *dirname++;
		}
		printf("\n");
		if(dest[-1] != '/')
			*dest++ = '/';
	}
	while(*name){
		*dest++ = *name++;
	}
}

/**
 * struct fileinfo - File information.
 * @name:   File name
 * @status: File status
 */
struct fileinfo {
	char *name;
	struct stat status;
};

//! File information slots
static struct fileinfo *files;
//! allocate `fileinfo` count in slots
static size_t alloc_count;
//! index of first unused `fileinfo` count in slots
static size_t unused_index;


/**
 * init_slots - Initialize File information slots
 *
 * File information slots initialize.(allocation count, index,...)
 */
static void init_slots()
{
	alloc_count = 0;
	unused_index = 0;
}

/**
 * addfiles_slots - Add a File information to slots
 * @name:   File name
 *
 * Return: 0 - success
 *         otherwise - error(show ERROR STATUS CODE)
 */
static int addfiles_slots(char const *name, char const *dirname)
{
	int err = 0;
	struct fileinfo *finfo;

	if(alloc_count <= unused_index) {
		files = realloc(files, alloc_count+=ALLOCATE_COUNT);
		if(!files) {
			file_failure(ALLOCATION_FAILURE, NULL);
			free(files);
			exit(ALLOCATION_FAILURE);
		}
		alloc_count += ALLOCATE_COUNT;
	}
	finfo = &files[unused_index];
	memset(finfo, '\0', sizeof *finfo);

	char *path;
	if(name[0] == '/' || dirname[0] == '\0') {
		path = (char *)name;
	} else {
		path = alloca(strlen(name) + strlen(dirname) + 2);
		joinpath(path, dirname, name);
	}

	err = lstat(path, &finfo->status);
	if(err) {
		file_failure(ACCESS_FAILURE, path);
		goto errout;
	}

	finfo->name = malloc((strlen(path)+1) * sizeof *path);
	if(!finfo->name) {
		file_failure(ALLOCATION_FAILURE, NULL);
		err = ALLOCATION_FAILURE;
		goto errout;
	}
	strncpy(finfo->name, path, strlen(path)+1);

	unused_index++;
errout:
	return err;
}

/**
 * __printfiles_slots - Print the file name
 * @out:    Output streams
 * @f:      File information.
 *
 * Return:  Number of items write
 */
static size_t __printfiles_slots(FILE *out, const struct fileinfo f)
{
	size_t len = 0;
	const char* name = f.name;

	if(out != NULL)
		len = fwrite(name, sizeof(char), strlen(name), out);
	return len;
}

/**
 * printfiles_slots - List all the files in slots
 */
static void printfiles_slots()
{
	int i;

	for(i = 0; i < unused_index; i++) {
		__printfiles_slots(stdout, files[i]);
		putchar('\n');
	}
}

/**
 * clean_slots - clean up file information slots
 *
 * WARN: Be sure clean up list when use slots.
 */
static void clean_slots()
{
	int i;
	for(i = unused_index; i >= 0;  i--, unused_index--) {
		free(files[i].name);
	}

	free(files);
}

int main(int argc, char *argv[])
{
	int i;
	int optind;
	int n_files;

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	optind = decode_cmdline(argc, argv);
	n_files = argc - optind;

	init_slots();
	alloc_count = ALLOCATE_COUNT;
	files = malloc (alloc_count * (sizeof *files));
	if(!files) {
		file_failure(ALLOCATION_FAILURE, NULL);
		exit(ALLOCATION_FAILURE);
	}

	if(n_files <= 0) {
		addfiles_slots(".", "");
	} else {
		for(i = optind; i < argc; i++)
			addfiles_slots(argv[i], "");
	}

	printfiles_slots();
	clean_slots();
	return 0;
}
