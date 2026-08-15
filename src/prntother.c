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
 * These queries all read a fixed offset out of the reply. The buffers are
 * zeroed and the transfer length checked, because a failed control transfer
 * otherwise leaves us reading uninitialised stack -- which for the toner
 * query could produce a bogus "0%% toner" and refuse a perfectly good job.
 */
static bool gurich_query(struct gurich_usb * g, uint8_t request,
	unsigned char * buf, int len, int need)
{
	int got;

	memset(buf, 0, (size_t)len);
	got = libusb_control_transfer(g->device_handle, 0xc1, request, 0x00, 0, buf, (uint16_t)len, 5000);

	return got >= need;
}

struct gurich_status gurich_status(struct gurich_usb * g)
{
	struct gurich_status stat;
	unsigned char statusbuf[1024];

	if (!gurich_query(g, 9, statusbuf, sizeof(statusbuf), 11)) {
		stat.ref = -1;
		stat.status = "UNKNOWN (could not query the printer)";
		return stat;
	}

	stat.ref = statusbuf[10];

	switch (statusbuf[10])
	{
		case PRINTER_STATUS_BAD:
			stat.status = "BAD";
			break;
		case PRINTER_STATUS_ENERGY_SAVING:
			stat.status = "ENERGY SAVING MODE 1";
			break;
		case PRINTER_STATUS_IDLE:
			stat.status = "GOOD / ENERGY SAVING MODE 2 / IDLE";
			break;
		case PRINTER_STATUS_PREPARING:
			stat.status = "PREPARING / RELAXING (UNKNOWN?)";
			break;
		case PRINTER_STATUS_PRINTING:
			stat.status = "PRINTING / WARMING UP";
			break;
		case PRINTER_STATUS_RETURNING_IDLE:
			stat.status = "GOING BACK TO IDLE (UNKNOWN?)";
			break;
		default:
			stat.status = "UNKNOWN";
	}

	return stat;
}

int gurich_toner(struct gurich_usb * g)
{
	unsigned char tonerbuf[1024];

	if (!gurich_query(g, 149, tonerbuf, sizeof(tonerbuf), 7)) {
		return -1;
	}

	return tonerbuf[6] * 10;
}

size_t gurich_printed(struct gurich_usb * g)
{
	unsigned char prbuf[1024];

	if (!gurich_query(g, 193, prbuf, sizeof(prbuf), 62)) {
		return 0;
	}

	if (prbuf[25] == 0x00) {
		return prbuf[24] + 15;
	} else if (prbuf[1] == 0x00 && prbuf[61] == 0x00) {
		return prbuf[0];
	} else {
		return (prbuf[0] + prbuf[24])/2 + 264;
	}
	/* prbuf[56] -> failures */
}

void gurich_testpage(struct gurich_usb * g)
{
	unsigned char testpagebuf[8];
	libusb_control_transfer(g->device_handle, 0x41, 162, 0x0000, 0, testpagebuf, 8, 5000);
}
