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

size_t gurich_dirent_fs(
	const char *path,
	struct gurich_files * fs,
	const char *reqfname)
{
	DIR *r_dir;
	struct dirent *r_read;
	size_t files, len;

	r_dir = NULL, r_read = NULL;
	files = 0, len = 0;

	if ((r_dir = opendir(path)) == NULL) {
		return 0;
	}

	while ((r_read = readdir(r_dir)) != NULL)
	{
		if (!strcmp (r_read->d_name, "."))
			continue;
		if (!strcmp (r_read->d_name, ".."))
			continue;
		if (strstr(r_read->d_name, reqfname) == NULL)
			continue;

		char **resized;

		resized = realloc(fs->files, (files+1) * sizeof(char *));
		if (resized == NULL) {
			goto dirent_mem_fail;
		}
		fs->files = resized;

		len = (strlen(path)+_D_EXACT_NAMLEN(r_read))+1;
		fs->files[files] = malloc(len);

		if (fs->files[files] == NULL) {
			goto dirent_mem_fail;
		}

		snprintf(fs->files[files], len, "%s%s", path, r_read->d_name);

		files++;
	}

	closedir(r_dir);

	return files;

	dirent_mem_fail:
		if (r_dir != NULL) {
			closedir(r_dir);
		}
		for (size_t i = 0; i < files; ++i) {
			free(fs->files[i]);
		}
		free(fs->files);
		fs->files = NULL;
		puts("Can't allocate memory (RAM). Quitting.");
		return 0;
		/* Expecting the function calling gurich_dirent_fs to then exit gracefully. */
}

bool gurich_dir_checkup()
{
	struct stat s;
	short int err;

	err = stat(GURICH_TEMP_DIR, &s);

	if (err == -1) {
		switch (errno) {
			case ENOENT:
				mkdir(GURICH_TEMP_DIR, GURICH_TEMP_DIR_PERMISSION);
				break;
			case EEXIST:
				break;
			default:
				fprintf(stderr, "Directory management error: %s\n", strerror(errno));
				return false;
		}
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
