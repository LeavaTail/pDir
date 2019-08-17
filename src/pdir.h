#ifndef _PDIR_H
#define _PDIR_H

/**
 * Program Name.
 * displayed when 'usage'
 */
#define PROGRAM_NAME	"pdir"

/**
 * ERROR STATUS CODE
 *  1: allocation failed(malloc)
 *  2: invalid command-line option
 *  3: file cannot open
 *  4: directory cannot open
 */
#define ALLOCATION_FAILURE	1
#define CMDLINE_FAILURE	2
#define ACCESS_FAILURE	3
#define OPENDIRECTRY_FAILURE	4

/**
 * Count of allocation memory in slots.
 * Default is 100, expand as needed.
 */
#define ALLOCATE_COUNT	100

/**
 * struct fileinfo - File information.
 * @name:   File name
 * @status: File status
 */
struct fileinfo {
	char *name;
	struct stat status;
};

#endif
