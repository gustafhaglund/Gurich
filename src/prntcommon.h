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


#ifndef __GURICH_CBACKEND__
#define __GURICH_CBACKEND__

void data_append(
	struct gurich_transferdata *data,
	const char *src,
	size_t len);

/* Appends printf-formatted text; sizes the buffer itself, so no call site
 * has to guess a maximum length. */
void data_printf(struct gurich_transferdata *data, const char *fmt, ...);

bool check_printer_status(struct gurich_usb * g);

bool do_send_usb(
	struct gurich_usb * g,
	struct gurich_transferdata * usbdata);

#endif
