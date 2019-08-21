#ifndef _PDIR_H
#define _PDIR_H

/**
 * Program Name, version, author.
 * displayed when 'usage' and 'version'
 */
#define PROGRAM_NAME	"pdir"
#define PROGRAM_VERSION	"0.1"
#define PROGRAM_AUTHOR	"LeavaTail"
#define COPYRIGHT_YEAR	"2019"
/**
 * Debug code
 */
#ifdef PDIR_DEBUG
#define pdir_debug(fmt, ...)						\
	do {								\
		fprintf( stderr, "(%s: %u): %s:" fmt, \
				__FILE__, __LINE__, __func__, ##__VA_ARGS__); \
	} while (0)
#else
#define pdir_debug(fmt, ...)	do { } while (0)
#endif

/**
 * ERROR STATUS CODE
 *  1: allocation failed(malloc)
 *  2: invalid command-line option
 *  3: file cannot open
 *  4: directory cannot open
 */
enum
{
	ALLOCATION_FAILURE = 1,
	CMDLINE_FAILURE = 2,
	ACCESS_FAILURE = 3,
	OPENDIRECTRY_FAILURE = 4
};

/**
 * Count of allocation memory in slots.
 * Default is 100, expand as needed.
 */
#define ALLOCATE_COUNT	100

/**
 * FOR LONG FORMAT, BUFFER SIZE
 */
enum
{
	/* FILETYPE "drwxrwxrwx" */
	FILETYPE_SIZE = 11,
	/* FILELINK COUNT (FIXME)*/
	FILELINK_SIZE = 11,
	/* USERNAME/GROUPNAME SIZE "#define UT_NAMESIZE    32" */
	FILEUSERGROUP_SIZE = 33,
	/* FILESIZE "4(GB) = 4,294,967,296(B)" */
	FILESIZE_SIZE = 11,
	/* TIMESIZE (FIXME) */
	FILETIME_SIZE = 81
};

/**
 * struct fileinfo - File information.
 * @name:   File name
 * @status: File status
 * @is_command_arg: specified that command_line argument
 */
struct fileinfo {
	char *name;
	struct stat status;
	bool is_command_arg;
};

#endif
