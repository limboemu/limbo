#include <curses.h>
#include <stdio.h>

/** @file
 *
 * MuCurses alert functions
 *
 */

/**
 * Audible signal
 *
 * @ret rc	return status code
 */
int beep ( void ) {
	printf("\a");
	return OK;
}
