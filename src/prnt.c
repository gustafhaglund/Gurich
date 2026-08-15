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

void gurich_prnt
(
	struct gurich_usb * g,
	const char * username,
	const char * res,
	const char * psfile,
	const char * copies,
	const char * papertype,
	bool cupsfilter
) {
	struct gurich_jbg_st jbg;
	struct gurich_pbm pbm;
	struct gurich_transferdata sendbunker = { NULL, 0 };

	struct tm * tme;
	time_t t;
	char datetime[32 + 1];
	char tempdir[256];
	char pagepath[320];

	FILE *pbmObj;

	size_t page = 1;

	if (!cupsfilter) {
		if (!check_printer_status(g))
			return;
	}

	if (!gurich_tempdir(tempdir, sizeof(tempdir))) {
		return;
	}

	if (!gurich_workaround_pbmgen(tempdir, res, papertype, psfile)) {
		fprintf(stderr, "ERROR: Ghostscript did not produce printable PBM data. Quitting.\n");
		goto cleanup;
	}

	t = time(NULL);
	tme = localtime(&t);
	strftime(datetime, sizeof(datetime), "%Y/%m/%d %H:%M:%S", tme);

	data_printf(
		&sendbunker,

		PRINTER_START_FORMAT \
		"@PJL SET TIMESTAMP=%s\r\n" \
		"@PJL SET FILENAME=%s\r\n" \
		"@PJL SET COMPRESS=JBIG\r\n" \
		"@PJL SET USERNAME=%s\r\n" \
		"@PJL SET COVER=%s\r\n" \
		"@PJL SET HOLD=%s\r\n",

		PRINTER_START,
		datetime,
		psfile,
		username,
		PRINTER_STANDARD_COVER,
		PRINTER_STANDARD_HOLD
	);

	/* Ghostscript numbers pages consecutively from 1, so walking the names
	 * until one is missing both counts the pages and orders them. */
	for (page = 1; ; ++page)
	{
		size_t sent;

		gurich_page_path(pagepath, sizeof(pagepath), tempdir, page);

		pbmObj = fopen(pagepath, "rb");
		if (pbmObj == NULL) {
			break;
		}

		#ifdef _DEBUG
		fprintf(stderr, "DEBUG: %s\n", pagepath);
		#endif

		fprintf(stderr, "INFO: Preparing page %zu\n", page);

		jbg.jbig = NULL;
		jbg.jbiglen = 0;

		gurich_jbg(pbmObj, &pbm, &jbg);
		fclose(pbmObj);

		if (jbg.jbiglen == 0)
		{
			fprintf(stderr, "CRIT: Something did happen with the JBIG image generation which this driver depend upon. Quitting.\n");
			free(jbg.jbig);
			goto cleanup;
		}

		data_printf(
			&sendbunker,

			"@PJL SET PAGESTATUS=START\r\n" \
			"@PJL SET COPIES=%s\r\n" \
			"@PJL SET MEDIASOURCE=AUTO\r\n" \
			"@PJL SET MEDIATYPE=%s\r\n" \
			"@PJL SET PAPER=%s\r\n" \
			"@PJL SET PAPERWIDTH=%lu\r\n" \
			"@PJL SET PAPERLENGTH=%lu\r\n" \
			"@PJL SET RESOLUTION=%s\r\n" \
			"@PJL SET IMAGELEN=%zu\r\n",

			copies,
			PRINTER_STANDARD_MEDIATYPE,
			papertype,
			pbm.width,
			pbm.height,
			res,
			(jbg.jbiglen > PRINTER_MAX_IMAGELEN ? (size_t)PRINTER_MAX_IMAGELEN : jbg.jbiglen)
		);

		/* The payload goes out in IMAGELEN-sized runs, each one but the
		 * first preceded by its own IMAGELEN header. */
		for (sent = 0; sent < jbg.jbiglen; )
		{
			size_t remaining = jbg.jbiglen - sent;
			size_t chunk = remaining > PRINTER_MAX_IMAGELEN ? (size_t)PRINTER_MAX_IMAGELEN : remaining;

			data_append(&sendbunker, jbg.jbig + sent, chunk);
			sent += chunk;

			remaining = jbg.jbiglen - sent;
			if (remaining > 0) {
				data_printf(
					&sendbunker, "@PJL SET IMAGELEN=%zu\r\n",
					remaining > PRINTER_MAX_IMAGELEN ? (size_t)PRINTER_MAX_IMAGELEN : remaining
				);
			}
		}

		data_append(&sendbunker, PRINTER_PAGE_END, strlen(PRINTER_PAGE_END));

		free(jbg.jbig);
		unlink(pagepath);
	}

	if (page == 1) {
		fprintf(stderr, "ERROR: Ghostscript produced no pages. Quitting.\n");
		goto cleanup;
	}

	data_append(&sendbunker, PRINTER_END, strlen(PRINTER_END));

	#ifdef _DEBUG
	if (!cupsfilter) {
		FILE * f = fopen("/tmp/gurich-pjl.bin", "w+");
		if (f != NULL) {
			fwrite(sendbunker.data, sendbunker.len, 1, f);
			fclose(f);
		}
	}
	#endif

	/* Send! */
	if (!cupsfilter) {
		do_send_usb(g, &sendbunker);
	} else {
		fwrite(sendbunker.data, sendbunker.len, 1, stdout);
	}

	cleanup:
		/* Pages are unlinked as they are consumed, so only the current one
		 * and any that follow it can still be here. On the normal path the
		 * first unlink fails at once and the loop ends. */
		for (;; ++page) {
			gurich_page_path(pagepath, sizeof(pagepath), tempdir, page);

			if (unlink(pagepath) != 0) {
				break;
			}
		}

		free(sendbunker.data);
		rmdir(tempdir);
}
