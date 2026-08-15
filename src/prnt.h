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

/* Printer details only. */

#ifndef __GURICH_PRINT__H
#define __GURICH_PRINT__H

#define PRINTER_VENDOR_ID         0x05ca
/* The USB vendor ID for Ricoh. */
#define PRINTER_LIBUSB_DEBUG      3

#define PRINTER_STANDARD_MEDIATYPE		"PLAINRECYCLE"
/* AUTO / TRAY1 */
#define PRINTER_STANDARD_COVER			"OFF"
#define PRINTER_STANDARD_HOLD			"OFF"

#define PRINTER_START	0x1b, 0x25, 0x2d, 0x31, 0x32, 0x33, 0x34, 0x35, 0x58, 0x40, 0x50, 0x4a, 0x4c, 0x0d, 0x0a
#define PRINTER_START_FORMAT "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define PRINTER_PAGE_END "@PJL SET DOTCOUNT=10\r\n@PJL SET PAGESTATUS=END\r\n"
#define PRINTER_END	"@PJL EOJ\r\n%-12345X\r\n"

/* Status byte reported at offset 10 of the status control transfer.
 * Note that PRINTING and WARMING UP share a code, so it cannot be used
 * to tell a busy printer from one that has just woken up. */
#define PRINTER_STATUS_ENERGY_SAVING  0x30
#define PRINTER_STATUS_IDLE           0x31
#define PRINTER_STATUS_RETURNING_IDLE 0x32
#define PRINTER_STATUS_PRINTING       0x33
#define PRINTER_STATUS_BAD            0x35
#define PRINTER_STATUS_PREPARING      0x37

/* Largest JBIG payload the printer accepts under one IMAGELEN header.
 * Taken from USB captures of the vendor driver. */
#define PRINTER_MAX_IMAGELEN 65556

/* Chunk size for bulk transfers to the printer. */
#define PRINTER_USB_CHUNK 4096

#endif
