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
	const char *res,
	char * psfile,
	const char * copies,
	char * papertype,
	bool cupsfilter
) {
	if (!cupsfilter) {
		if (!check_printer_status(g))
			return;
	}

	gurich_workaround_pbmgen(g, res, papertype, psfile);

	struct gurich_jbg_st jbg;
	struct gurich_pbm pbm;
	struct gurich_files fs;
	struct gurich_transferdata sendbunker;

	struct tm * tme;
	time_t t;
	char datetime[32 + 1];

	FILE *pbmObj;

	char page[250];

	size_t beginlen;
	size_t fsfiles;
	size_t fsl;

	size_t jbigbegin;
	size_t jbigSize;

	t = time(NULL);
	tme = localtime(&t);
	strftime(datetime, sizeof(datetime), "%Y/%m/%d %H:%M:%S", tme);

	sendbunker.data = malloc((beginlen = (1024 + strlen(psfile))));
	fs.files = malloc(10);

	gurich_alloc_check(sendbunker.data);
	gurich_alloc_set(sendbunker.data);
	gurich_alloc_check(fs.files);
	gurich_alloc_set(fs.files);

	fsfiles = 0;

	jbigbegin = 0;
	jbigSize = 0;

	fsfiles = gurich_dirent_fs(GURICH_TEMP_DIR, &fs, "pbm");
	if (fsfiles == 0) goto cleanup;

	snprintf(
		sendbunker.data,
		beginlen,

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

	sendbunker.begin = strlen(sendbunker.data);

	for (fsl = 0; fsl < fsfiles; ++fsl)
	{
		pbmObj = fopen(fs.files[fsl], "rb");

		#ifdef _DEBUG
		fprintf(stderr, "DEBUG: %s\n", fs.files[fsl]);
		#endif

		fprintf(stderr, "INFO: Preparing page %lu\n", fsl+1);

		if (pbmObj == NULL) {
			fprintf(stderr, "DEBUG: Can't open the pbm file. Quitting.\n");
			return;
		}

		jbg.jbig = malloc(1);
		jbg.jbiglen = 0;

		gurich_alloc_check(jbg.jbig);
		gurich_alloc_set(jbg.jbig);

		gurich_jbg(pbmObj, &pbm, &jbg);

		if (jbg.jbiglen == 0)
		{
			fprintf(stderr, "CRIT: Something did happen with the JBIG image generation which this driver depend upon. Quitting.\n");
			free(jbg.jbig);
			unlink(fs.files[fsl]);
			free(fs.files[fsl]);
			goto cleanup;
		}

		fclose(pbmObj);

		/* Sending preparation... */

		snprintf(
			page,
			250,

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
			(jbg.jbiglen > 65556 ? 65556 : jbg.jbiglen)
		);

		data_modify(&sendbunker, page, strlen(page), 0);

		while (jbg.jbiglen != 0)
		{
			char imglen[50];

			if (jbg.jbiglen > 65556) {
				jbigSize = 65556;
			} else {
				jbigSize = jbg.jbiglen;
			}

			data_modify(&sendbunker, jbg.jbig, jbigSize, jbigbegin);

			jbigbegin += jbigSize;
			jbg.jbiglen -= jbigSize;
			jbigSize = (jbg.jbiglen > 65556 ? 65556 : jbg.jbiglen);

			/* If it's more in the jbig buffer, then do a mark (and later copy the buffer portion to sendbunker).
			 * jbigSize in this context could be called remainingLen.
			*/
			if (jbigSize > 0) {
				snprintf(imglen, 50, "@PJL SET IMAGELEN=%zu\r\n", jbigSize);
				/* Limits to max 30 numbers incl. PJL-text and ending characters. */
				data_modify(&sendbunker, imglen, strlen(imglen), 0);
			}
		}

		data_modify(&sendbunker, PRINTER_PAGE_END, strlen(PRINTER_PAGE_END), 0);

		jbigbegin = 0;

		/* Clean up. */
		unlink(fs.files[fsl]);
		free(fs.files[fsl]);
		free(jbg.jbig);
	}

	if (fsfiles > 0) {
		data_modify(&sendbunker, PRINTER_END, strlen(PRINTER_END), 0);
	}

	#ifdef _DEBUG
	if (!cupsfilter) {
		FILE * f = fopen(GURICH_TEMP_DIR"pjl.bin", "w+");
		fwrite(sendbunker.data, sendbunker.begin, 1, f);
		fclose(f);
	}
	#endif

	/* Send! */
	if (!cupsfilter) {
		do_send_usb(g, &sendbunker);
	} else {
		fwrite(sendbunker.data, sendbunker.begin, 1, stdout);
	}

	cleanup:
		free(sendbunker.data);

		if (fsfiles > 0)
			free(fs.files);
}
