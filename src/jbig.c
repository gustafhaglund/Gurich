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

/* This file is largely based on source code from jbigkit, by Markus Kuhn. */

static void jbg_out(unsigned char *jbgenc, size_t len, void *jbg_tmp)
{ 
	struct gurich_jbg_st *jbg = (struct gurich_jbg_st *)jbg_tmp;	size_t i, c;

	jbg->jbig = realloc(jbg->jbig, (jbg->jbiglen+len));
	gurich_alloc_check(jbg->jbig);

	for (i = jbg->jbiglen, c = 0; c < len; ++i, ++c) {
		jbg->jbig[i] = jbgenc[c];
	}

	jbg->jbiglen += len;
}

static unsigned long getint(FILE *f)
{
	int c;
	unsigned long i;

	while ((c = getc(f)) != EOF && !isdigit(c))
	{
		if (c == '#') {
			while ((c = getc(f)) != EOF && !(c == 13 || c == 10)) ;
		}
	}

	if (c != EOF) {
		ungetc(c, f);
		if (fscanf(f, "%lu", &i) != 1) {
			/* should never fail, since c must be a digit */
			return 0;
		}
	}

	return i;
}

void gurich_jbg(FILE *pbmFp, struct gurich_pbm * pbm, struct gurich_jbg_st *jbg)
{
	struct jbg_enc_state s;

	unsigned short int type;
	unsigned long width, height;

	int options = 72; /* -p */
	int order = 3; /* -o */
	int mx = 0; /* -m */
	int d = 0; /* -q */

	/** P4 **/
	int encode_planes = 1;
	unsigned int l0 = 0;
	int dl = -1, dh = -1;

	unsigned char **bitmap;
	size_t bitmap_size;

	/* read, get width, height and type */
	type = getint(pbmFp);
	width = getint(pbmFp);
	height = getint(pbmFp);
	fgetc(pbmFp);

	if (type != 4 && type != 0) {
		/* Not intended to support other formats than P4 (binary PBM). */
		fprintf(stderr, "ERROR: Ghostscript, a util this driver uses, provided an unsupported format. Quitting.\n");
		goto failure;
	} else if (width < 1 || height < 1 || type == 0) {
		fprintf(stderr, "ERROR: Something did happen with getint(), which is used for capturing of height, width and format spec of a PBM file. Quitting.\n");
		goto failure;
	}

	pbm->height = height;
	pbm->width = width;

	/* action */
	bitmap_size = ((width+7) / 8) * (size_t)height;

	bitmap = malloc(sizeof(unsigned char*) * encode_planes);
	bitmap[0] = malloc(bitmap_size);

	fread(bitmap[0], bitmap_size, 1, pbmFp);

	/* jbg lib */
	jbg_enc_init(&s, width, height, encode_planes, bitmap, jbg_out, jbg);

	jbg_enc_layers(&s, d);
	jbg_enc_lrange(&s, dl, dh);
	jbg_enc_options(&s, order, options, l0, mx, -1);

	jbg_enc_out(&s);
	jbg_enc_free(&s);

	/* free */
	free(bitmap[0]);
	free(bitmap);

	return;

	failure:
		jbg->jbiglen = 0;
		return;
}
