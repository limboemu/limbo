/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <stdbool.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

static const unsigned long long convert[] = {
	0x0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFFFFULL, 0xFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL
};

static int
print_str_fill(char **buffer, size_t bufsize, char *sizec,
					const char *str, char c)
{
	int i, sizei, len;
	char *bstart = *buffer;

	sizei = strtoul(sizec, NULL, 10);
	len = strlen(str);
	if (sizei > len) {
		for (i = 0;
			(i < (sizei - len)) && ((*buffer - bstart) < bufsize);
									i++) {
			**buffer = c;
			*buffer += 1;
		}
	}
	return 1;
}

static int
print_str(char **buffer, size_t bufsize, const char *str)
{
	char *bstart = *buffer;
	size_t i;

	for (i = 0; (i < strlen(str)) && ((*buffer - bstart) < bufsize); i++) {
		**buffer = str[i];
		*buffer += 1;
	}
	return 1;
}

static unsigned int
print_intlen(unsigned long value, unsigned short int base)
{
	int i = 0;

	while (value > 0) {
		value /= base;
		i++;
	}
	if (i == 0)
		i = 1;
	return i;
}

static int
print_itoa(char **buffer, size_t bufsize, unsigned long value,
					unsigned short base, bool upper)
{
	const char zeichen[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	char c;
	size_t i, len;

	if(base <= 2 || base > 16)
		return 0;

	len = i = print_intlen(value, base);

	/* Don't print to buffer if bufsize is not enough. */
	if (len > bufsize)
		return 0;

	do {
		c = zeichen[value % base];
		if (upper)
			c = toupper(c);

		(*buffer)[--i] = c;
		value /= base;
	} while(value);

	*buffer += len;

	return 1;
}



static int
print_fill(char **buffer, size_t bufsize, char *sizec, unsigned long size,
				unsigned short int base, char c, int optlen)
{
	int i, sizei, len;
	char *bstart = *buffer;

	sizei = strtoul(sizec, NULL, 10);
 	len = print_intlen(size, base) + optlen;
	if (sizei > len) {
		for (i = 0;
			(i < (sizei - len)) && ((*buffer - bstart) < bufsize);
									i++) {
			**buffer = c;
			*buffer += 1;
		}
	}

	return 0;
}


static int
print_format(char **buffer, size_t bufsize, const char *format, void *var)
{
	char *start;
	unsigned int i = 0, length_mod = sizeof(int);
	unsigned long value = 0;
	unsigned long signBit;
	char *form, sizec[32];
	char sign = ' ';
	bool upper = false;

	form  = (char *) format;
	start = *buffer;

	form++;
	if(*form == '0' || *form == '.') {
		sign = '0';
		form++;
	}

	while ((*form != '\0') && ((*buffer - start) < bufsize)) {
		switch(*form) {
			case 'u':
			case 'd':
			case 'i':
				sizec[i] = '\0';
				value = (unsigned long) var;
				signBit = 0x1ULL << (length_mod * 8 - 1);
				if ((*form != 'u') && (signBit & value)) {
					**buffer = '-';
					*buffer += 1;
					value = (-(unsigned long)value) & convert[length_mod];
				}
				print_fill(buffer, bufsize - (*buffer - start),
						sizec, value, 10, sign, 0);
				print_itoa(buffer, bufsize - (*buffer - start),
							value, 10, upper);
				break;
			case 'X':
				upper = true;
			case 'x':
				sizec[i] = '\0';
				value = (unsigned long) var & convert[length_mod];
				print_fill(buffer, bufsize - (*buffer - start),
						sizec, value, 16, sign, 0);
				print_itoa(buffer, bufsize - (*buffer - start),
							value, 16, upper);
				break;
			case 'O':
			case 'o':
				sizec[i] = '\0';
				value = (long int) var & convert[length_mod];
				print_fill(buffer, bufsize - (*buffer - start),
						sizec, value, 8, sign, 0);
				print_itoa(buffer, bufsize - (*buffer - start),
							value, 8, upper);
				break;
			case 'p':
				sizec[i] = '\0';
				print_fill(buffer, bufsize - (*buffer - start),
					sizec, (unsigned long) var, 16, ' ', 2);
				print_str(buffer, bufsize - (*buffer - start),
									"0x");
				print_itoa(buffer, bufsize - (*buffer - start),
						(unsigned long) var, 16, upper);
				break;
			case 'c':
				sizec[i] = '\0';
				print_fill(buffer, bufsize - (*buffer - start),
							sizec, 1, 10, ' ', 0);
				**buffer = (unsigned long) var;
				*buffer += 1;
				break;
			case 's':
				sizec[i] = '\0';
				print_str_fill(buffer,
					bufsize - (*buffer - start), sizec,
							(char *) var, ' ');

				print_str(buffer, bufsize - (*buffer - start),
								(char *) var);
				break;
			case 'l':
				form++;
				if(*form == 'l') {
					length_mod = sizeof(long long int);
				} else {
					form--;
					length_mod = sizeof(long int);
				}
				break;
			case 'h':
				form++;
				if(*form == 'h') {
					length_mod = sizeof(signed char);
				} else {
					form--;
					length_mod = sizeof(short int);
				}
				break;
			case 'z':
				length_mod = sizeof(size_t);
				break;
			default:
				if(*form >= '0' && *form <= '9')
					sizec[i++] = *form;
		}
		form++;
	}

	
	return (long int) (*buffer - start);
}


/*
 * The vsnprintf function prints a formated strings into a buffer.
 * BUG: buffer size checking does not fully work yet
 */
int
vsnprintf(char *buffer, size_t bufsize, const char *format, va_list arg)
{
	char *ptr, *bstart;

	bstart = buffer;
	ptr = (char *) format;

	/*
	 * Return from here if size passed is zero, otherwise we would
	 * overrun buffer while setting NULL character at the end.
	 */
	if (!buffer || !bufsize)
		return 0;

	/* Leave one space for NULL character */
	bufsize--;

	while(*ptr != '\0' && (buffer - bstart) < bufsize)
	{
		if(*ptr == '%') {
			char formstr[20];
			int i=0;
			
			do {
				formstr[i] = *ptr;
				ptr++;
				i++;
			} while(!(*ptr == 'd' || *ptr == 'i' || *ptr == 'u' || *ptr == 'x' || *ptr == 'X'
						|| *ptr == 'p' || *ptr == 'c' || *ptr == 's' || *ptr == '%'
						|| *ptr == 'O' || *ptr == 'o' )); 
			formstr[i++] = *ptr;
			formstr[i] = '\0';
			if(*ptr == '%') {
				*buffer++ = '%';
			} else {
				print_format(&buffer,
					bufsize - (buffer - bstart),
					formstr, va_arg(arg, void *));
			}
			ptr++;
		} else {

			*buffer = *ptr;

			buffer++;
			ptr++;
		}
	}
	
	*buffer = '\0';

	return (buffer - bstart);
}
