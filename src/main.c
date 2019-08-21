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
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
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

/**
 * PRINT MODE
 * print out direcotry contents mode.
 */
static enum
{
	/* Default. Ignore files whose names start with "." */
	PRINT_DEFAULT,
	/* "-A" option. print out include "." AND ".." */
	PRINT_ALMOST,
	/* "-a" option. print out include "." AND ".." AND ".FILENAME" */
	PRINT_ALL
} print_mode;

/**
 * PRINT FORMAT MODE
 * print out direcotry contents format mode.
 */
static enum
{
	/* Default. */
	PRINT_DEFAULT_FORMAT,
	/* "-l" option. a lot of info, one per line */
	PRINT_LONG_FORMAT,
} print_format;

/**
 * PRINT TIME MODE
 */
static enum
{
	/* Default, modify time */
	PRINT_MODIFY_TIME,
	/* "-c" option, change time */
	PRINT_CHANGE_TIME,
	/* "-u" option, access time */
	PRINT_ACCESS_TIME
} print_time;

/* File information slots */
static struct fileinfo *files;
static struct fileinfo **sorted;
/* allocate `fileinfo` count in slots, index of first unused */
static size_t alloc_count;
static size_t unused_index;
/* the number of columns to use for columns */
static int nlink_width;
static int user_width;
static int group_width;
static int file_size_width;
static int time_width;
/* time information */
static struct timespec current;
static struct timespec year_ago;

/* strftime formats for non-recent and recent files */
static char const *long_time_format[2] =
{
	/* strftime format for non-recent files (older than 6 months). */
	("%b %e  %Y"),
	/* strftime format for recent files (younger than 6 months), in -l */
	("%b %e %H:%M")
};

/* option data {"long name", needs argument, flags, "short name"} */
static struct option const longopts[] =
{
	{"all", no_argument, NULL, 'a'},
	{"almost-all", no_argument, NULL, 'A'},
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
	print_mode = PRINT_DEFAULT;
	print_format = PRINT_DEFAULT_FORMAT;
	print_time = PRINT_MODIFY_TIME;
	int longindex = 0;
	int opt = 0;

	while ((opt = getopt_long(argc, argv,
		"alA",
		longopts, &longindex)) != -1) {
		switch (opt) {
		case 'a':
			print_mode = PRINT_ALL;
			break;
		case 'l':
			print_format = PRINT_LONG_FORMAT;
			break;
		case 'A':
			print_mode = PRINT_ALMOST;
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

	if (S_ISDIR(ai->status.st_mode) && !S_ISDIR(bi->status.st_mode))
		return COMPARE_EARLIER;

	if (!S_ISDIR(ai->status.st_mode) && S_ISDIR(bi->status.st_mode))
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
	bool ret = false;

	switch (print_mode) {
	case PRINT_DEFAULT:
		ret = (name[0] == '.') ? true : false;
	case PRINT_ALMOST:
		ret |= dot_or_ddot(name);
	case PRINT_ALL:
		break;
	}
	return ret;
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
 * set_useralign - set user-id from uid with aligned
 * @uid:   user-id
 * @u_buf: output buffer. (username OR user-id)
 * @width: output width
 */
static void set_useralign(uid_t uid, char *u_buf, int width)
{
	struct passwd *passwd = getpwuid(uid);
	if (passwd)
		sprintf(u_buf, "%-*s", width, passwd->pw_name);
	else
		sprintf(u_buf, "%-*d", width, uid);
}

/**
 * set_groupalign - set group-id from gid with aligned
 * @gid:   group-id
 * @g_buf: output buffer. (groupname OR group-id)
 * @width: output width
 */
static void set_groupalign(gid_t gid, char *g_buf, int width)
{
	struct group *group = getgrgid(gid);
	if (group)
		sprintf(g_buf, "%-*s", width, group->gr_name);
	else
		sprintf(g_buf, "%-*d", width, gid);
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

	if (print_format == PRINT_LONG_FORMAT) {
		char buf[FILETYPE_SIZE +
				FILELINK_SIZE +
				FILEUSERGROUP_SIZE +
				FILESIZE_SIZE +
				FILETIME_SIZE] = {0};
		size_t len = 0;

		uid_t uid = finfo->status.st_uid;
		set_useralign(uid, buf, 0);
		len = strlen(buf);
		if (user_width < len)
			user_width = len;

		gid_t gid = finfo->status.st_gid;
		set_groupalign(gid, buf, 0);
		len = strlen(buf);
		if (group_width < len)
			group_width = len;

		size_t size = finfo->status.st_size;
		sprintf(buf, "%lu", size);
		len = strlen(buf);
		if (file_size_width < len)
			file_size_width = len;

		size_t nlink = finfo->status.st_nlink;
		sprintf(buf, "%lu", nlink);
		len = strlen(buf);
		if (nlink_width < len)
			nlink_width = len;
	}

	unused_index++;
errout:
	return err;
}

/**
 * ftypelet - Display letters and indicators for each filetype.
 * @bits:  File mode
 *
 * Return: File type bit
 */
static char ftypelet (mode_t bits)
{
  /* These are the most common, so test for them first.  */
  if (S_ISREG (bits))
    return '-';
  if (S_ISDIR (bits))
    return 'd';

  /* Other letters standardized by POSIX 1003.1-2004.  */
  if (S_ISBLK (bits))
    return 'b';
  if (S_ISCHR (bits))
    return 'c';
  if (S_ISLNK (bits))
    return 'l';
  if (S_ISFIFO (bits))
    return 'p';

  /* Other file types (though not letters) standardized by POSIX.  */
  if (S_ISSOCK (bits))
    return 's';

  return '?';
}

/**
 * get_filemode - fill in string STR with an ls-style.
 * @mode: File mode
 * @dest: Output buffer. Representative of the st_mode field.
 */
static void get_filemode(mode_t mode, char *dest)
{
	dest[0] = ftypelet(mode);
	dest[1] = mode & S_IRUSR ? 'r' : '-';
	dest[2] = mode & S_IWUSR ? 'w' : '-';
	dest[3] = mode & S_ISUID ? (mode & S_IXUSR ? 's' : 'S')
		: (mode & S_IXUSR ? 'x' : '-');
	dest[4] = mode & S_IRGRP ? 'r' : '-';
	dest[5] = mode & S_IWGRP ? 'w' : '-';
	dest[6] = mode & S_ISGID ? (mode & S_IXGRP ? 's' : 'S')
		: (mode & S_IXGRP ? 'x' : '-');
	dest[7] = mode & S_IROTH ? 'r' : '-';
	dest[8] = mode & S_IWOTH ? 'w' : '-';
	dest[9] = mode & S_ISVTX ? (mode & S_IXOTH ? 't' : 'T')
		: (mode & S_IXOTH ? 'x' : '-');
	dest[10] = '\0';
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
 * timecmp - compare time *a to *b.
 * @a:  compared timespec
 * @b:  compared timespec
 *
 * Return: Positive - A > B
 *         Negative - A < B
 *         zero -     A = B
 */
static inline int
timecmp (struct timespec a, struct timespec b)
{
	return (a.tv_sec < b.tv_sec ? COMPARE_EARLIER
			: a.tv_sec > b.tv_sec ? COMPARE_LATER
			: (int) (a.tv_nsec - b.tv_nsec));
}

/**
 * __printfiles_slots_long - Print the file name in long format
 * @out:    Output streams
 * @f:      File information.
 *
 * Return:  Number of items write
 */
static size_t __printfiles_slots_long(FILE *out, const struct fileinfo *f)
{
	char mode[FILETYPE_SIZE] = {0};
	char n_links[FILELINK_SIZE] = {0};
	char user[FILEUSERGROUP_SIZE] = {0};
	char group[FILEUSERGROUP_SIZE] = {0};
	char size[FILESIZE_SIZE] = {0};
	char time[FILETIME_SIZE];
	struct timespec ts;
	bool recent;
	size_t len = 0;
	const char *name = f->name;

	get_filemode(f->status.st_mode, mode);
	sprintf(n_links, "%*lu", nlink_width, f->status.st_nlink);
	set_useralign(f->status.st_uid, user, user_width);
	set_groupalign(f->status.st_gid, group, group_width);
	sprintf(size, "%*lu", file_size_width, f->status.st_size);

	switch (print_time)
	{
		case PRINT_MODIFY_TIME:
			ts = f->status.st_mtim;
			break;
		case PRINT_CHANGE_TIME:
			ts = f->status.st_ctim;
			break;
		case PRINT_ACCESS_TIME:
			ts = f->status.st_atim;
			break;
	}
	recent = (timecmp(year_ago, ts) < 0);
	strftime(time,
			FILETIME_SIZE,
			long_time_format[recent],
			localtime(&ts.tv_sec));

	if (out != NULL)
		len = fprintf(out, "%s %s %s %s %s %s %s",
				mode, n_links, user, group, size, time, name);
	return len;
}

/**
 * printfiles_slots - List all the files in slots
 */
static void printfiles_slots(void)
{
	int i;

	switch (print_format) {
	case PRINT_DEFAULT_FORMAT:
		for (i = 0; i < unused_index; i++) {
			__printfiles_slots(stdout, sorted[i]);
			putchar('\n');
		}
		break;
	case PRINT_LONG_FORMAT:
		for (i = 0; i < unused_index; i++) {
			__printfiles_slots_long(stdout, sorted[i]);
			putchar('\n');
		}
		break;
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

	nlink_width = 0;
	user_width = 0;
	group_width = 0;
	file_size_width = 0;
	time_width = 0;

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

		if (S_ISDIR(f->status.st_mode))
			add_list(f->name, strlen(f->name) + 1);
	}

	for (i = 0, j = 0; i < unused_index; i++)
	{
		bool is_command_arg_direcory;
		struct fileinfo *f = sorted[i];
		sorted[j] = f;
		is_command_arg_direcory = f->is_command_arg &&
							S_ISDIR(f->status.st_mode);
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

	clock_gettime(CLOCK_MONOTONIC, &current);
	year_ago.tv_sec = current.tv_sec - (365.2425 * 24 * 60 * 60);
	year_ago.tv_nsec = current.tv_nsec;

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
