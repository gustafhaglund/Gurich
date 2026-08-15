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

#ifndef GURICH_TEMP_DIR
	#define GURICH_TEMP_DIR "/tmp/gurich/"
#endif

#ifndef GURICH_TEMP_DIR_PERMISSION
	#define GURICH_TEMP_DIR_PERMISSION 0777
	//0666
#endif

#ifndef _D_EXACT_NAMLEN
	/* For BSD systems. */
	#define _D_EXACT_NAMLEN(d) strlen((d)->d_name)
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
	bool state;
	int ref;
	char *status;
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

struct gurich_transferdata {
	char *data;
	size_t begin;
};

/* General functions */
#define gurich_alloc_check(a) \
if (a == NULL) { \
	fprintf(stderr, "ERROR: Can't allocate memory (RAM). Quitting.\n"); \
	exit(-1); \
}
#define gurich_alloc_set(a) *a = '\0'

size_t gurich_dirent_fs(
	const char *path,
	struct gurich_files * fs,
	const char * reqfname);

bool gurich_dir_checkup();

char * get_username();

bool gurich_workaround_pbmgen
(
	struct gurich_usb * g,
	const char * res,
	char * papertype,
	const char * psfile
);

void check_printer_usb(struct gurich_usb * g);
void cleanup_usb(struct gurich_usb * g);

void gurich_jbg(FILE *pbmFp,
	struct gurich_pbm * pbm,
	struct gurich_jbg_st *jbg);

/* "Real" printer functions */
struct gurich_status gurich_status(struct gurich_usb * g);
unsigned short int gurich_toner(struct gurich_usb * g);
size_t gurich_printed(struct gurich_usb * g);
void gurich_testpage(struct gurich_usb * g);

void gurich_prnt
(
	struct gurich_usb * g,
	const char * username,
	const char * res,
	char * psfile,
	const char * copies,
	char * papertype,
	bool cupsfilter
);

#include <prntcommon.h>

#endif
