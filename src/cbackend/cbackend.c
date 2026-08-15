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

/* Cups backend. */

#include <gurich.h>

/* TODO
static bool check_cmd(struct gurich_transferdata *data)
{
	bool ret = false;

	//fprintf(stderr, "DEBUG: gurich ");
	//fwrite(data->data, data->begin, 1, stderr);
	//fprintf(stderr, "\n");

	if (strstr(data->data, "ReportLevels") != NULL) {
		fputs("ATTR: marker-colors=#000000\n", stderr);
		fputs("ATTR: marker-low-levels=5\n", stderr);
		fputs("ATTR: marker-names=Black\n", stderr);
		fputs("ATTR: marker-types=toner\n", stderr);

		ret = true;
	}

	return ret;
}*/

int main(int argc, char ** argv)
{
	struct gurich_usb g;
	/* Must start zeroed: data_append() reallocs data->data, so a stack
	 * struct left uninitialised hands realloc() a garbage pointer. */
	struct gurich_transferdata bin = { NULL, 0, 0 };
	g.initialized = false;

	check_printer_usb(&g);
	/* TODO: Use the serial number (cupsBackendDeviceURI()).
	 * For now, it's not possible to have more than one Ricoh printer in function. */

	if (!g.initialized) {
		fprintf(stderr, "DEBUG: Could not find the printer. Quitting.\n\n");
		return -1;
	}

	if (!check_printer_status(&g)) {
		cleanup_usb(&g);
		return -1;
	}

	if (argc == 1) {
		//printf("direct gurich \"gurich\" \"SP100/SP110 series\"\n");
		//cupsBackendReport("usb", cupsBackendDeviceURI(argv), "Ricoh SP100/SP110 series", "", "", "");
		printf("direct gurich:/usb/%0lx?serial=%0lx \"gurich\" \"SP100/SP110 series\"", g.idProduct, g.iSerialNumber);
		return 0;
	}

	char binbuf[8192];
	ssize_t readlen;
	int fp = 0;

	if (argc < 6) {
		fprintf(stderr, "ERROR: Not enough arguments\n");
		cleanup_usb(&g);
		return -1;
	} else if (argc > 6) {
		fp = open(argv[6], O_RDONLY);

		if (fp < 0) {
			fprintf(stderr, "ERROR: Could not open %s: %s\n", argv[6], strerror(errno));
			cleanup_usb(&g);
			return -1;
		}
	}

	/* ssize_t, not size_t: a read() error returns -1, which as an unsigned
	 * value would pass "> 0" and be appended as a huge length. */
	while ((readlen = read(fp, binbuf, sizeof(binbuf))) > 0) {
		data_append(&bin, binbuf, (size_t)readlen);
	}

	if (readlen < 0) {
		fprintf(stderr, "ERROR: Could not read the job data: %s\n", strerror(errno));
		free(bin.data);
		if (fp != 0) close(fp);
		cleanup_usb(&g);
		return -1;
	}

	if (fp != 0) close(fp);

	/*if (check_cmd(&bin)) {
		goto exit;
	}*/

	if (!do_send_usb(&g, &bin)) {
		free(bin.data);
		cleanup_usb(&g);
		return -1;
	}

	//exit:
		free(bin.data);
		cleanup_usb(&g);
		return 0;
}
