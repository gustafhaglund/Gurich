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

static void init_msg()
{
	unsigned short int i;

	puts("\t* Gurich - printer driver *\n");
	puts("Copyright (c) 2016, 2017 Gustaf Haglund");
	puts(
		"\n"
		"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
		"This is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n"
	);

	for (i = 0; i < 45; ++i) {
		printf("_");
	}

	puts("\n");
}

static void display_usage(const char * exec, bool cpr)
{
	printf("print: %s -p [ps/pdf file, resolution (600|1200), copies, paper type (A4 is tested)]\n", exec);
	#ifndef _NO_USB
	printf("status: %s -s\n", exec);
	printf("testpage: %s -t\n", exec);
	#endif

	if (cpr) {
		puts("\nCopyright (c) 2016, 2017 Gustaf Haglund\n");
	}
}

#ifndef _NO_USB
static void display_status(struct gurich_usb * g)
{
	struct gurich_status data = gurich_status(g);
	int ink = gurich_toner(g);

	printf("Printer status: %s, reference: 0x%x\n", data.status, data.ref);

	if (ink < 0) {
		puts("Printer toner: unknown (the printer did not answer)");
	} else {
		printf(
			"Printer toner: %d %% left %s\n",
			ink,
			(ink <= 10 ? "- please consider buying more toner" : "")
		);
	}

	printf("Printed out pages (stats): %zu\n", gurich_printed(g));
}
#endif

static void nofilter_print(struct gurich_usb * g, int argc, char ** argv)
{
	if (argc < 6) {
		fprintf(stderr, "Not enough arguments. Quitting.\n");
		return;
	}

	gurich_prnt(g, get_username(), argv[3], argv[2], argv[4], argv[5], false);
}

static void cups_filter_print(struct gurich_usb * g, int argc, char ** argv)
{
	int psf;
	char * res;
	FILE * psfp;
	ssize_t psread;
	char * copies;
	char * username;
	char * papertype;
	char psbuf[8192];
	char psfn[BUFSIZ];

	/* CUPS calls a filter as: job user title copies options [file] */
	if (argc < 6) {
		fprintf(stderr, "ERROR: Not enough arguments for a CUPS filter. Quitting.\n");
		return;
	}

	psf = cupsTempFd(psfn, BUFSIZ);
	if (psf < 0) {
		fprintf(stderr, "ERROR: Could not create a CUPS temp file.\n");
		return;
	}

	psfp = fdopen(psf, "wb+");
	if (psfp == NULL) {
		fprintf(stderr, "ERROR: Could not open the CUPS temp file: %s\n", strerror(errno));
		close(psf);
		return;
	}

	username = argv[2];
	copies = argv[4];
	res = "600";
	papertype = "A4";

	/*
	 * TODO:
	 * -Full CUPS filter support with user-supplied options
	 *
	 */

	while ((psread = read(0, psbuf, sizeof(psbuf))) > 0)
	{
		if (fwrite(psbuf, (size_t)psread, 1, psfp) != 1) {
			fprintf(stderr, "ERROR: Could not write PostScript input to the CUPS temp file.\n");
			fclose(psfp);
			unlink(psfn);
			return;
		}
	}

	if (psread < 0) {
		fprintf(stderr, "ERROR: Could not read PostScript input: %s\n", strerror(errno));
		fclose(psfp);
		unlink(psfn);
		return;
	}

	fclose(psfp);

	gurich_prnt(g, username, res, psfn, copies, papertype, true);
	unlink(psfn);
}

int main(int argc, char ** argv)
{
	const char *arg;
	struct gurich_usb g;
	g.initialized = false;

	if (argc < 2) {
		display_usage(argv[0], true);
		return 0;
	}

	arg = argv[1];

	if (arg[0] == '-')
	{
		init_msg();

		/* Usage text is answered before probing USB: needing the printer
		 * plugged in just to read the help was never useful. */
		if (arg[1] == 'h' || arg[1] == '\0') {
			display_usage(argv[0], false);
			puts("");
			return 0;
		}

		#ifndef _NO_USB
		check_printer_usb(&g);

		if (!g.initialized) {
			fprintf(stderr, "Could not find the printer. Quitting.\n\n");
			exit(-1);
		}
		#endif

		switch(arg[1])
		{
			case 'p':
				nofilter_print(&g, argc, argv);
				break;
			#ifndef _NO_USB
			case 's':
				display_status(&g);
				break;
			case 't':
				gurich_testpage(&g);
				break;
			#endif
			default:
				display_usage(argv[0], false);
		}

		puts("");
	} else {
		cups_filter_print(&g, argc, argv);
	}

	cleanup_usb(&g);
	return 0;
}
