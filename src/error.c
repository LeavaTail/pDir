/**
 * @file error.c
 * @brief Error handler
 * @author LeavaTail
 * @date 2019/08/16
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * ERROR STATUS CODE
 *  1: allocation failed(malloc)
 */
enum
{
	ALLOCATION_FAILURE = 1
};

/**
 * error - Output error message
 * @status: status code
 * @message: output message (variable length)
 */
void error(int status, const char *message, ...)
{
	va_list args, copy;
	size_t length;
	char *buffer;

	va_start (args, message);
	/* calculate variable message length */
	va_copy(copy, args);
	length = vsnprintf(NULL, 0, message, copy);

	/* store variable message to string */
	buffer = malloc((length + 1) * sizeof(char));
	if (buffer) {
		vsnprintf(buffer, length + 1, message, args);
		perror(buffer);

		free(buffer);
	}

	va_end(args);
}
