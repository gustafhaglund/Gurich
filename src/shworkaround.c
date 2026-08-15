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
#include <sys/wait.h>

bool gurich_workaround_pbmgen(
	const char * tempdir, const char * res,
	const char * papertype, const char * psfile)
{
	char gs_cmd[512];
	char * gs_papertype;
	int status;

	if (strcmp(res, "1200") == 0) {
		res = "1200x600";
	}

	/*
	 * Lowercase a private copy for Ghostscript's -sPAPERSIZE. papertype may
	 * point at a string literal (the CUPS filter path passes "A4"), so
	 * lowercasing it in place would be a write into read-only memory, and
	 * the caller still needs the original case for the PJL PAPER field.
	 */
	gs_papertype = malloc(strlen(papertype) + 1);
	gurich_alloc_check(gs_papertype);
	strcpy(gs_papertype, papertype);

	for (size_t i = 0; gs_papertype[i] != '\0'; i++){
		gs_papertype[i] = tolower((unsigned char)gs_papertype[i]);
	}

	if ((size_t)snprintf(
		gs_cmd,
		sizeof(gs_cmd),
		"/usr/bin/env gs -sDEVICE=pbmraw -sOutputFile=%s%%03d-page.pbm "
		"-r%s -dQUIET -dBATCH -dNOPAUSE -sPAPERSIZE=%s '%s'",
		tempdir,
		res,
		gs_papertype,
		psfile
	) >= sizeof(gs_cmd)) {
		fprintf(stderr, "ERROR: The Ghostscript command line is too long.\n");
		free(gs_papertype);
		return false;
	}

	#ifdef _DEBUG
	fprintf(stderr, "DEBUG: gs_cmd %s\n", gs_cmd);
	#endif

	status = system(gs_cmd);

	free(gs_papertype);

	if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		fprintf(stderr, "ERROR: Ghostscript (%s) failed to convert the input to PBM.\n", psfile);
		return false;
	}

	return true;
}
