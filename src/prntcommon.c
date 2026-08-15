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

/* Makes room for extra bytes past data->len, doubling rather than fitting
 * exactly so that a job costs O(size) to assemble instead of O(size^2). */
static void data_reserve(struct gurich_transferdata *data, size_t extra)
{
	char * resized;
	size_t want = data->len + extra;

	if (want <= data->cap) {
		return;
	}

	if (data->cap == 0) {
		data->cap = 8192;
	}
	while (data->cap < want) {
		data->cap *= 2;
	}

	resized = realloc(data->data, data->cap);
	gurich_alloc_check(resized);
	data->data = resized;
}

void data_append(
	struct gurich_transferdata *data,
	const char *src,
	size_t len)
{
	if (len == 0) {
		return;
	}

	data_reserve(data, len);
	memcpy(data->data + data->len, src, len);
	data->len += len;
}

void data_printf(struct gurich_transferdata *data, const char *fmt, ...)
{
	va_list ap, apc;
	int need;

	va_start(ap, fmt);
	va_copy(apc, ap);

	need = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);

	if (need > 0) {
		/* +1 for the NUL vsnprintf insists on writing; len excludes it, so
		 * the next append overwrites it. */
		data_reserve(data, (size_t)need + 1);
		vsnprintf(data->data + data->len, (size_t)need + 1, fmt, apc);
		data->len += (size_t)need;
	}

	va_end(apc);
}

bool check_printer_status(struct gurich_usb * g)
{
	#ifndef _NO_USB
	struct gurich_status data = gurich_status(g);
	int toner;

	/*
	 * There is deliberately no "already printing" check. gurich_status()
	 * reports 0x33 as "PRINTING / WARMING UP" -- the two are the same code,
	 * so refusing on it would reject jobs sent to a printer that is merely
	 * warming up, which is the normal state after idle.
	 */
	if (data.ref == PRINTER_STATUS_BAD) {
		fprintf(stderr, "ERROR: Please read the manual and check out the printer (bad state reported). Quitting.\n");
		return false;
	}

	toner = gurich_toner(g);
	if (toner == 0) {
		fprintf(stderr, "ERROR: Not sufficient level of toner (0%% left of toner). Quitting.\n");
		return false;
	}
	/* toner < 0 means the query failed; that is not a reason to refuse a job. */
	#else
	(void)g;
	#endif

	return true;
}

bool do_send_usb(
	struct gurich_usb * g,
	struct gurich_transferdata * usbdata)
{
	#ifndef _NO_USB
	size_t sent;

	if (g->initialized == false) {
		return false;
	}

	for (sent = 0; sent < usbdata->len; )
	{
		size_t remaining = usbdata->len - sent;
		int chunk = (int)(remaining > PRINTER_USB_CHUNK ? PRINTER_USB_CHUNK : remaining);
		int transferred = 0;
		int err;

		/* libusb reads straight from our buffer, so there is nothing to be
		 * gained by staging the chunk in a scratch array first. */
		err = libusb_bulk_transfer(
			g->device_handle, 0x01,
			(unsigned char *)usbdata->data + sent,
			chunk, &transferred, 5000);

		if (err != 0) {
			fprintf(
				stderr,
				"ERROR: Sending to the printer failed after %zu of %zu bytes (%s). Quitting.\n",
				sent, usbdata->len, libusb_error_name(err)
			);
			return false;
		}

		#ifdef _DEBUG
			printf("transferred: %d, sent: %zu, total: %zu\n", transferred, sent, usbdata->len);
		#endif

		sent += (size_t)transferred;

		if (transferred == 0) {
			fprintf(stderr, "ERROR: The printer stopped accepting data. Quitting.\n");
			return false;
		}
	}
	#else
	(void)g; (void)usbdata;
	#endif

	return true;
}
