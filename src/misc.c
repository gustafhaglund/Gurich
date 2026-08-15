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

static int gurich_is_pbm(const struct dirent *d)
{
	const char *dot = strrchr(d->d_name, '.');
	return dot != NULL && strcmp(dot, ".pbm") == 0;
}

/*
 * Collects the job's pages, in page order.
 *
 * Ghostscript names them 001-page.pbm, 002-page.pbm and so on, but readdir()
 * hands them back in directory order, which on ext4/btrfs is hash order --
 * a twelve page document came out as 6,10,11,9,7,1,2,4,5,8,12,3. alphasort
 * both fixes that and replaces the hand-rolled growable array this used to
 * carry.
 */
size_t gurich_pbm_pages(const char *path, struct gurich_files * fs)
{
	struct dirent **names;
	int n;

	fs->files = NULL;

	n = scandir(path, &names, gurich_is_pbm, alphasort);
	if (n <= 0) {
		return 0;
	}

	fs->files = malloc((size_t)n * sizeof(char *));
	gurich_alloc_check(fs->files);

	for (int i = 0; i < n; ++i) {
		size_t len = strlen(path) + strlen(names[i]->d_name) + 1;

		fs->files[i] = malloc(len);
		gurich_alloc_check(fs->files[i]);
		snprintf(fs->files[i], len, "%s%s", path, names[i]->d_name);

		free(names[i]);
	}
	free(names);

	return (size_t)n;
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
