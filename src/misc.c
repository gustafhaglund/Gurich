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
 * Builds the name Ghostscript was told to write page n to.
 *
 * The driver passes gs -sOutputFile=<dir>%03d-page.pbm, so the names are
 * already known and pages can simply be walked 1, 2, 3 ... until one is
 * missing. Listing the directory instead was what put pages in the wrong
 * order, because readdir() returns hash order, not name order.
 */
void gurich_page_path(char * buf, size_t size, const char * dir, size_t page)
{
	snprintf(buf, size, "%s%03zu-page.pbm", dir, page);
}

/*
 * Creates a private directory for one job and returns it with a trailing
 * slash. mkdtemp() makes it 0700 and unique, which is what stops two
 * concurrent jobs from consuming each other's pages.
 */
bool gurich_tempdir(char * out, size_t outlen)
{
	char template[] = GURICH_TEMP_TEMPLATE;

	if (mkdtemp(template) == NULL) {
		fprintf(stderr, "ERROR: Could not create a temporary directory: %s\n", strerror(errno));
		return false;
	}

	if ((size_t)snprintf(out, outlen, "%s/", template) >= outlen) {
		fprintf(stderr, "ERROR: Temporary directory path is too long.\n");
		rmdir(template);
		return false;
	}

	return true;
}

char * get_username()
{
	struct passwd *pw;
	uid_t uid;

	uid = geteuid();
	pw = getpwuid(uid);

	if (pw) {
		return pw->pw_name;
	}
	else {
		return "Gurich";
	}
}
