/**
 * @file main.c
 * @brief Print Directory entry (like 'ls').
 * @author LeavaTail
 * @date 2019/08/18
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <config.h>
#include <getopt.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include "pdir.h"
#include "gettext.h"
#include "error.h"
#include "list.h"

/**
 * Be written to support message catalogs
 * HOW TO USE
 *   printf(_(MESSAGE));
 *   perror(_(MESSAGE));
 */
#define _(String) gettext (String)

/**
 * Special Option(no short option)
 */
enum
{
	GETOPT_HELP_CHAR = (CHAR_MIN - 2),
	GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

/**
 * usage - print out usage.
 * @status: Status code
 */
void usage(int status)
{
	FILE *out;
	switch (status) {
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

/**
 * file_failure - MESSAGE to report the failure to access a file named FILE
 * @status: Status code
 * @name:   File name
 */
static void file_failure (int status, char const *name)
{
	switch (status) {
	case ALLOCATION_FAILURE:
		error(status, _("%s: cannot allocate memory"), PROGRAM_NAME);
		break;
	case ACCESS_FAILURE:
		error(status, _("%s: cannot access '%s'"), PROGRAM_NAME, name);
		break;
	case OPENDIRECTRY_FAILURE:
		error(status, _("%s: cannot open directory '%s'"), PROGRAM_NAME, name);
		break;
	}
}

/**
 * version - print out program version.
 * @command_name: command name
 * @version:      program version
 * @author:       program authoer
 */
void version(const char *command_name, const char *version,
			const char *author)
{
	FILE *out = stdout;

	fprintf(out, "%s %s\n", command_name, version);
	fprintf(out, "Copyright (C) %s Free Software Foundation, Inc.",
													COPYRIGHT_YEAR);
	fputs("\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n", out);
	fprintf (out, _("Written by %s.\n"), PROGRAM_AUTHOR);
}

/* "-a" option. print out include "." AND ".." AND ".FILENAME" */
static bool print_all;
/* File information slots */
static struct fileinfo *files;
static struct fileinfo **sorted;
/* allocate `fileinfo` count in slots, index of first unused */
static size_t alloc_count;
static size_t unused_index;

/* option data {"long name", needs argument, flags, "short name"} */
static struct option const longopts[] =
{
	{"all", no_argument, NULL, 'a'},
	{"help",no_argument, NULL, GETOPT_HELP_CHAR},
	{"version",no_argument, NULL, GETOPT_VERSION_CHAR},
	{0,0,0,0}
};

/**
 * decode_cmdline - analyze command-line arguments.
 * @argc: command-line argument count
 * @argv: commend-line arfument vector
 *
 * Return: index of the first non-option argument
 */
static int decode_cmdline(int argc, char **argv)
{
	int longindex = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv,
		"a",
		longopts, &longindex)) != -1) {
		switch (opt) {
		case 'a':
			print_all = true;
			break;
		case GETOPT_HELP_CHAR:
			usage(EXIT_SUCCESS);
			break;
		case GETOPT_VERSION_CHAR:
			version(PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_AUTHOR);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage(CMDLINE_FAILURE);
		}
	}

	return optind;
}

/**
 * joinpath - put DIRNAME/NAME into DEST, handling "." and "/" properly.
 * @dest   : Retult pathname
 * @dirname: Base direcotry name
 * @name   : File name
 */
static void joinpath(char *dest, const char *dirname, const char *name)
{
	if (dirname[0] != '.' || dirname[1] != '\0') {
		while (*dirname)
			*dest++ = *dirname++;
		if (dest[-1] != '/')
			*dest++ = '/';
	}

	while (*name)
		*dest++ = *name++;
	*dest = '\0';
}

/**
 * dot_or_ddot - Check whether File name is "." OR ".."
 * @name:   File name
 *
 * Return: true  - File name is "." OR ".."
 *         false - File name is Regular file
 */
static inline bool dot_or_ddot(char const *name)
{
	bool ret = false;
	if (name != NULL && name[0] == '.') {
		char sep = name[(name[1] == '.') + 1];
		ret = (!sep || sep == '/');
	}
	return ret;
}

/**
 * COMPARE RESULT compare(a, b);
 *  -1: *a is earlier than *b
 *   1: *a is later than *b
 */
enum
{
	COMPARE_EARLIER = -1,
	COMPARE_LATER = 1
};
/**
 * compare_name - Compare filename (Dirname > Filename)
 * @a:   fileinfo pointer
 * @b:   fileinfo pointer
 *
 * Return:  1 - earlier than
 *         -1 - later than
 *          0 - equal to
 */
static int compare_name(const void *a, const void *b)
{
	struct fileinfo *ai = *(struct fileinfo**)a;
	struct fileinfo *bi = *(struct fileinfo**)b;

	if (((ai->status.st_mode & S_IFMT) == S_IFDIR)
	&& ((bi->status.st_mode & S_IFMT) != S_IFDIR))
		return COMPARE_EARLIER;

	if (((ai->status.st_mode & S_IFMT) != S_IFDIR)
	&& ((bi->status.st_mode & S_IFMT) == S_IFDIR))
		return COMPARE_LATER;

	return strcmp(ai->name, bi->name);
}

/**
 * file_ignored - "." or ".." is normally ignored.
 * @name:   File name
 *
 * Return: true  - File should be ignore ("." OR ".." OR ".FILENAME")
 *         false - File shoule be print  ("FILENAME" OR '-a' option)
 */
static bool file_ignored(char const *name)
{
	return (!print_all && (dot_or_ddot(name) || name[0] == '.'));
}

/**
 * init_slots - Initialize File information slots
 *
 * File information slots initialize.(allocation count, index,...)
 */
static void init_slots(void)
{
	alloc_count = 0;
	unused_index = 0;
}

/**
 * addfiles_slots - Add a File information to slots
 * @name:    File name
 * @dirname: Base direcotry name
 * @command_line_arg: Command line argument
 *
 * Return: 0 - success
 *         otherwise - error(show ERROR STATUS CODE)
 */
static int addfiles_slots(char const *name, char const *dirname,
						bool command_arg)
{
	int err = 0;
	struct fileinfo *finfo;

	if (alloc_count <= unused_index) {
		alloc_count += ALLOCATE_COUNT;
		files = realloc(files, alloc_count);
		if (!files) {
			file_failure(ALLOCATION_FAILURE, NULL);
			free(files);
			exit(ALLOCATION_FAILURE);
		}
		sorted = realloc(sorted, alloc_count);
		if (!sorted) {
			file_failure(ALLOCATION_FAILURE, NULL);
			free(sorted);
			exit(ALLOCATION_FAILURE);
		}
	}
	finfo = &files[unused_index];
	memset(finfo, '\0', sizeof(*finfo));

	char *path;
	if (name[0] == '/' || dirname[0] == '\0') {
		path = (char *)name;
	} else {
		path = alloca(strlen(name) + strlen(dirname) + 2);
		joinpath(path, dirname, name);
	}

	err = lstat(path, &finfo->status);
	if (err) {
		file_failure(ACCESS_FAILURE, path);
		goto errout;
	}

	finfo->name = malloc((strlen(name) + 1) * sizeof(*name));
	if (!finfo->name) {
		file_failure(ALLOCATION_FAILURE, NULL);
		err = ALLOCATION_FAILURE;
		goto errout;
	}
	strncpy(finfo->name, name, strlen(name) + 1);
	finfo->is_command_arg = command_arg;

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
static size_t __printfiles_slots(FILE *out, const struct fileinfo *f)
{
	size_t len = 0;
	const char *name = f->name;

	if (out != NULL)
		len = fwrite(name, sizeof(char), strlen(name) + 1, out);
	return len;
}

/**
 * printfiles_slots - List all the files in slots
 */
static void printfiles_slots(void)
{
	int i;

	for (i = 0; i < unused_index; i++) {
		__printfiles_slots(stdout, sorted[i]);
		putchar('\n');
	}
}

/**
 * clear_slots - clean up file information slots
 *
 * WARN: files slots will not release.
 */
static void clear_slots(void)
{
	int i;
	for (i = 0; i < unused_index; i++)
		free(files[i].name);
	unused_index = 0;
}

/**
 * clean_slots - clean up all file information slots
 *
 * WARN: Be sure clean up list when use slots.
 */
static void clean_slots(void)
{
	clear_slots();
	free(sorted);
	free(files);
}

/**
 * sortfiles_slots - sort files now in the file information slots
 */
static void sortfiles_slots(void)
{
	int i;
	for (i = 0 ; i < unused_index; i++)
		sorted[i] = &files[i];
	qsort((void const **)sorted, unused_index, sizeof(struct fileinfo*),
															compare_name);
}

/**
 * extractfiles_fromdir - Remove directory and set directory entries
 * @dirname: Base direcotry name
 */
static void extractfiles_fromdir(char const *dirname)
{
	int i, j;
	for (i = 0; i < unused_index; i++) {
		struct fileinfo *f = sorted[i];

		if (((f->status.st_mode & S_IFMT) == S_IFDIR))
			add_list(f->name, strlen(f->name) + 1);
	}

	for (i = 0, j = 0; i < unused_index; i++)
	{
		bool is_command_arg_direcory;
		struct fileinfo *f = sorted[i];
		sorted[j] = f;
		is_command_arg_direcory = f->is_command_arg &&
							((f->status.st_mode & S_IFMT) == S_IFDIR);
		j += !(is_command_arg_direcory);
		if (is_command_arg_direcory)
			free(f->name);
	}
	unused_index = j;
}

/**
 * print_dir - Read directory name, and list the files in it.
 * @name: Base direcotry name
 */
static void print_dir(char const *name)
{
	DIR *dirp;
	struct dirent *next;
	static bool first = true;

	dirp = opendir(name);
	if (!dirp) {
		file_failure(OPENDIRECTRY_FAILURE, name);
		return;
	}

	if (!first)
		putchar('\n');
	first = false;
	fprintf(stdout, "%s:\n", name);

	clear_slots();
	while ((next = readdir(dirp)) != NULL) {
		if (!file_ignored(next->d_name))
			addfiles_slots(next->d_name, name, false);
	}

	sortfiles_slots();
	closedir(dirp);
	printfiles_slots();
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
	init_list();
	alloc_count = ALLOCATE_COUNT;

	files = malloc (alloc_count * (sizeof(*files)));
	if (!files) {
		file_failure(ALLOCATION_FAILURE, NULL);
		exit(ALLOCATION_FAILURE);
	}
	sorted = malloc(alloc_count * (sizeof(*sorted)));
	if (!sorted) {
		file_failure(ALLOCATION_FAILURE, NULL);
		exit(ALLOCATION_FAILURE);
	}

	if (n_files <= 0) {
		addfiles_slots(".", "", true);
	} else {
		for (i = optind; i < argc; i++)
			addfiles_slots(argv[i], "", true);
	}

	if (unused_index) {
		sortfiles_slots();
		extractfiles_fromdir(NULL);
	}
	printfiles_slots();

	while (get_listcount()) {
		size_t len = get_length();
		char *dirname = malloc(len * sizeof(char));
		get_list(dirname, len);
		print_dir(dirname);
		free(dirname);
	}

	clean_list();
	clean_slots();
	return 0;
}
