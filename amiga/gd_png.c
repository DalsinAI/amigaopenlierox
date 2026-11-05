/* The part of libgd that OpenLieroX uses (see include/gd.h), on libpng.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#include <stdlib.h>
#include "gd.h"
#include "png_rgba.h"

gdImagePtr gdImageCreateTrueColor(int sx, int sy)
{
	gdImagePtr im;
	int y;

	if (sx <= 0 || sy <= 0)
		return NULL;
	im = (gdImagePtr)calloc(1, sizeof(*im));
	if (!im)
		return NULL;
	im->tpixels = (int **)calloc((size_t)sy, sizeof(int *));
	if (!im->tpixels) {
		free(im);
		return NULL;
	}
	for (y = 0; y < sy; y++) {
		im->tpixels[y] = (int *)calloc((size_t)sx, sizeof(int));
		if (!im->tpixels[y]) {
			im->sy = y;
			gdImageDestroy(im);
			return NULL;
		}
	}
	im->sx = sx;
	im->sy = sy;
	im->trueColor = 1;
	return im;
}

void gdImageDestroy(gdImagePtr im)
{
	int y;
	if (!im)
		return;
	if (im->tpixels) {
		for (y = 0; y < im->sy; y++)
			free(im->tpixels[y]);
		free(im->tpixels);
	}
	free(im);
}

gdImagePtr gdImageCreateFromPngPtr(int size, void *data)
{
	int w, h, alpha, x, y;
	unsigned char *rgba, *p;
	gdImagePtr im;

	if (size <= 0 || !data)
		return NULL;
	rgba = amiga_png_decode(data, (size_t)size, &w, &h, &alpha);
	if (!rgba)
		return NULL;
	im = gdImageCreateTrueColor(w, h);
	if (im) {
		p = rgba;
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++, p += 4)
				/* gd's alpha: 0 opaque, 127 clear */
				im->tpixels[y][x] = gdTrueColorAlpha(p[0], p[1], p[2], (255 - p[3]) >> 1);
	}
	free(rgba);
	return im;
}

void *gdImagePngPtr(gdImagePtr im, int *size)
{
	unsigned char *rgba, *p;
	size_t len = 0;
	void *png;
	int x, y;

	*size = 0;
	if (!im)
		return NULL;
	rgba = (unsigned char *)malloc((size_t)im->sx * im->sy * 4);
	if (!rgba)
		return NULL;
	p = rgba;
	for (y = 0; y < im->sy; y++)
		for (x = 0; x < im->sx; x++) {
			int c = im->tpixels[y][x];
			*p++ = (unsigned char)gdTrueColorGetRed(c);
			*p++ = (unsigned char)gdTrueColorGetGreen(c);
			*p++ = (unsigned char)gdTrueColorGetBlue(c);
			*p++ = 0xFF;
		}
	png = amiga_png_encode(rgba, im->sx, im->sy, 0, &len);
	free(rgba);
	*size = (int)len;
	return png;
}

void *gdImageJpegPtr(gdImagePtr im, int *size, int quality)
{
	(void)im;
	(void)quality;
	*size = 0;
	return NULL;
}

void *gdImageGifPtr(gdImagePtr im, int *size)
{
	(void)im;
	*size = 0;
	return NULL;
}

void gdFree(void *m)
{
	free(m);
}
