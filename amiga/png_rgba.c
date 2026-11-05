/* PNG to 8-bit RGBA rows and back, on libpng (see png_rgba.h).
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <png.h>
#include "png_rgba.h"

struct mem_in { const unsigned char *p; size_t left; };

/* libpng's warnings (iCCP profiles and the like, common in the game's
 * pictures) would go to the Shell; the pictures load all the same. */
static void quiet(png_structp png, png_const_charp msg)
{
	(void)png;
	(void)msg;
}

static void read_mem(png_structp png, png_bytep out, png_size_t n)
{
	struct mem_in *m = (struct mem_in *)png_get_io_ptr(png);
	if (n > m->left)
		png_error(png, "PNG data ends early");
	memcpy(out, m->p, n);
	m->p += n;
	m->left -= n;
}

unsigned char *amiga_png_decode(const void *data, size_t size, int *w, int *h, int *has_alpha)
{
	png_structp png;
	png_infop info;
	struct mem_in in;
	unsigned char *volatile pixels = NULL;
	png_bytep *volatile rows = NULL;
	png_uint_32 pw, ph, y;
	int depth, ctype, alpha_seen;
	size_t i;

	if (size < 8 || png_sig_cmp((png_const_bytep)data, 0, 8))
		return NULL;
	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, quiet);
	if (!png)
		return NULL;
	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, NULL, NULL);
		return NULL;
	}
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		free(rows);
		free(pixels);
		return NULL;
	}
	in.p = (const unsigned char *)data;
	in.left = size;
	png_set_read_fn(png, &in, read_mem);
	png_read_info(png, info);
	png_get_IHDR(png, info, &pw, &ph, &depth, &ctype, NULL, NULL, NULL);
	if (pw == 0 || ph == 0 || pw > 16384 || ph > 16384)
		png_error(png, "bad PNG size");

	/* everything to 8-bit RGBA */
	if (depth == 16)
		png_set_strip_16(png);
	if (ctype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);
	if (ctype == PNG_COLOR_TYPE_GRAY && depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);
	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);
	if (ctype == PNG_COLOR_TYPE_GRAY || ctype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);
	png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	png_set_interlace_handling(png);
	png_read_update_info(png, info);

	pixels = (unsigned char *)malloc((size_t)pw * ph * 4);
	rows = (png_bytep *)malloc(sizeof(png_bytep) * ph);
	if (!pixels || !rows)
		png_error(png, "out of memory");
	for (y = 0; y < ph; y++)
		rows[y] = pixels + (size_t)y * pw * 4;
	png_read_image(png, rows);
	png_read_end(png, NULL);
	png_destroy_read_struct(&png, &info, NULL);
	free(rows);

	alpha_seen = 0;
	for (i = 3; i < (size_t)pw * ph * 4; i += 4)
		if (pixels[i] != 0xFF) {
			alpha_seen = 1;
			break;
		}
	*w = (int)pw;
	*h = (int)ph;
	*has_alpha = alpha_seen;
	return pixels;
}

struct mem_out { unsigned char *p; size_t len, cap; int failed; };

static void write_mem(png_structp png, png_bytep d, png_size_t n)
{
	struct mem_out *m = (struct mem_out *)png_get_io_ptr(png);
	if (m->len + n > m->cap) {
		size_t cap = m->cap ? m->cap * 2 : 65536;
		unsigned char *np;
		while (cap < m->len + n)
			cap *= 2;
		np = (unsigned char *)realloc(m->p, cap);
		if (!np) {
			m->failed = 1;
			png_error(png, "out of memory");
		}
		m->p = np;
		m->cap = cap;
	}
	memcpy(m->p + m->len, d, n);
	m->len += n;
}

static void flush_mem(png_structp png) { (void)png; }

void *amiga_png_encode(const unsigned char *rgba, int w, int h, int alpha, size_t *size)
{
	png_structp png;
	png_infop info;
	struct mem_out out = { NULL, 0, 0, 0 };
	unsigned char *volatile row = NULL;
	int x, y;

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, quiet);
	if (!png)
		return NULL;
	info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, NULL);
		return NULL;
	}
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_write_struct(&png, &info);
		free(row);
		free(out.p);
		return NULL;
	}
	png_set_write_fn(png, &out, write_mem, flush_mem);
	png_set_IHDR(png, info, (png_uint_32)w, (png_uint_32)h, 8,
	             alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);
	row = (unsigned char *)malloc((size_t)w * 4);
	if (!row)
		png_error(png, "out of memory");
	for (y = 0; y < h; y++) {
		const unsigned char *s = rgba + (size_t)y * w * 4;
		unsigned char *d = row;
		for (x = 0; x < w; x++, s += 4) {
			*d++ = s[0];
			*d++ = s[1];
			*d++ = s[2];
			if (alpha)
				*d++ = s[3];
		}
		png_write_row(png, row);
	}
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);
	free(row);
	*size = out.len;
	return out.p;
}
