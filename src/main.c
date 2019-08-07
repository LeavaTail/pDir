/*
 * main.c
 *
 * Print Directory entry (like 'ls').
 */
#include <stdio.h>
#include <config.h>
#include "gettext.h"
#define _(String) gettext (String)

int main(int argc, char *argv[])
{
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	printf(gettext("hello, world\n"));
	return 0;
}
