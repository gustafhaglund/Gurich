/*

				Gurich
		**	Ricoh SP110 series driver **

	Copyright (C) 2016, 2017 Gustaf Haglund <kontakt@ghaglund.se>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __GURICH_H__
#define __GURICH_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>

#include <jbig.h>
#include <libusb.h>
#include <cups/cups.h>
#include <cups/backend.h>

#include <prnt.h>

/*
 * Each job gets its own directory, created by mkdtemp() with 0700
 * permissions. A shared fixed path would let concurrent jobs consume each
 * other's pages, and would leave a killed job's pages behind for the next
 * job to pick up.
 */
#ifndef GURICH_TEMP_TEMPLATE
	#define GURICH_TEMP_TEMPLATE "/tmp/gurich-XXXXXX"
#endif

struct gurich_usb {
	libusb_context *ctx;
	libusb_device_handle *device_handle;
	libusb_device *device;
	unsigned int interface;
	bool initialized;
	uint64_t idProduct;
	uint64_t iSerialNumber;
};

struct gurich_status {
	int ref;
	const char *status;
};

struct gurich_jbg_st {
	char *jbig;
	size_t jbiglen;
};

struct gurich_pbm {
	unsigned long width;
	unsigned long height;
};

struct gurich_files {
	char **files;
};

/*
 * A growable byte buffer. cap tracks the allocation so appends amortise to
 * O(1); without it, every append reallocated to the exact new size, which
 * made building a job quadratic in its own size.
 */
struct gurich_transferdata {
	char *data;
	size_t len;
	size_t cap;
};

/* General functions */
#define gurich_alloc_check(a) \
if (a == NULL) { \
	fprintf(stderr, "ERROR: Can't allocate memory (RAM). Quitting.\n"); \
	exit(-1); \
}

size_t gurich_pbm_pages(const char *path, struct gurich_files * fs);

bool gurich_tempdir(char * out, size_t outlen);

char * get_username();

bool gurich_workaround_pbmgen
(
	const char * tempdir,
	const char * res,
	const char * papertype,
	const char * psfile
);

void check_printer_usb(struct gurich_usb * g);
void cleanup_usb(struct gurich_usb * g);

void gurich_jbg(FILE *pbmFp,
	struct gurich_pbm * pbm,
	struct gurich_jbg_st *jbg);

/* "Real" printer functions */
struct gurich_status gurich_status(struct gurich_usb * g);
/* Toner percentage, or -1 when the printer could not be queried. */
int gurich_toner(struct gurich_usb * g);
size_t gurich_printed(struct gurich_usb * g);
void gurich_testpage(struct gurich_usb * g);

void gurich_prnt
(
	struct gurich_usb * g,
	const char * username,
	const char * res,
	const char * psfile,
	const char * copies,
	const char * papertype,
	bool cupsfilter
);

#include <prntcommon.h>

#endif
