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

#include <gurich.h>

/*
 * Appends len bytes from src (starting at srcbegin) to data,
 * growing data's backing buffer as needed.
 */
void data_modify(
	struct gurich_transferdata *data,
	const char *src,
	size_t len,
	size_t srcbegin)
{
	char * resized;

	if (len == 0) {
		return;
	}

	resized = realloc(data->data, data->begin + len);
	gurich_alloc_check(resized);
	data->data = resized;

	memcpy(data->data + data->begin, src + srcbegin, len);
	data->begin += len;
}

bool check_printer_status(struct gurich_usb * g)
{
	#ifndef _NO_USB
	struct gurich_status data = gurich_status(g);

	if (strcmp((char*)data.status, "BAD") == 0) {
		fprintf(stderr, "ERROR: Please read the manual and check out the printer (bad state reported). Quitting.\n");
		return false;
	} else if (strcmp((char*)data.status, "PRINTING") == 0) {
		fprintf(stderr, "ERROR: Your printer is already printing. Quitting.\n");
		return false;
	} else if (gurich_toner(g) == 0) {
		fprintf(stderr, "ERROR: Not sufficient level of toner (0%% left of toner). Quitting.\n");
		return false;
	}
	#endif

	return true;
}

void do_send_usb(
	struct gurich_usb * g,
	struct gurich_transferdata * usbdata)
{
	#ifndef _NO_USB
	if (g->initialized == false) {
		return;
	}

	size_t totSize;
	size_t tsSize;
	size_t tsStart;
	int receivedLength;

	totSize = usbdata->begin, tsSize = 0, tsStart = 0;
	receivedLength = 0;

	//fprintf(stderr, "DEBUG: %lu totSize\n", totSize);

	while (totSize != 0)
	{
		unsigned char tBuf[4096];

		if (totSize > 4096)
			tsSize = 4096;
		else
			tsSize = totSize;

		memcpy(tBuf, usbdata->data + tsStart, tsSize);

		libusb_bulk_transfer (g->device_handle, 0x01, tBuf, tsSize, &receivedLength, 5000);

		#ifdef _DEBUG
			printf ("receivedLength: %d, tsStart: %zu, totSize: %zu, tsSize %zu\n", receivedLength, tsStart, totSize, tsSize);
		#endif

		tsStart += tsSize;
		totSize -= tsSize;
	}
	#endif
}
