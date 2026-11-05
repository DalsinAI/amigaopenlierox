/* OpenLieroX for AmigaOS: the part of libgd that OpenLieroX uses, on
 * libpng (amiga/gd_png.c): true-colour images, reading a PNG from memory
 * (MapLoader's hi-res .lxl levels) and writing one (screenshots).
 * JPEG and GIF screenshots are not written (NULL).
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_GD_SHIM_H
#define AMIGA_GD_SHIM_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gdImageStruct {
	int sx, sy;
	int **tpixels; /* [y][x], 0xAARRGGBB with gd's 7-bit alpha (0 opaque) */
	int trueColor;
	int saveAlphaFlag;
} gdImage;
typedef gdImage *gdImagePtr;

#define gdImageSX(im) ((im)->sx)
#define gdImageSY(im) ((im)->sy)
#define gdTrueColor(r, g, b) (((r) << 16) + ((g) << 8) + (b))
#define gdTrueColorAlpha(r, g, b, a) (((a) << 24) + ((r) << 16) + ((g) << 8) + (b))
#define gdTrueColorGetAlpha(c) (((c) & 0x7F000000) >> 24)
#define gdTrueColorGetRed(c) (((c) & 0xFF0000) >> 16)
#define gdTrueColorGetGreen(c) (((c) & 0x00FF00) >> 8)
#define gdTrueColorGetBlue(c) ((c) & 0x0000FF)
#define gdImageGetTrueColorPixel(im, x, y) ((im)->tpixels[(y)][(x)])
#define gdImageSetPixel(im, x, y, c) ((im)->tpixels[(y)][(x)] = (c))
#define gdImageSaveAlpha(im, f) ((im)->saveAlphaFlag = (f))
#define gdImageAlphaBlending(im, f) ((void)(im), (void)(f))

gdImagePtr gdImageCreateTrueColor(int sx, int sy);
gdImagePtr gdImageCreateFromPngPtr(int size, void *data);
void gdImageDestroy(gdImagePtr im);
void *gdImagePngPtr(gdImagePtr im, int *size);
void *gdImageJpegPtr(gdImagePtr im, int *size, int quality);
void *gdImageGifPtr(gdImagePtr im, int *size);
void gdFree(void *m);
#ifdef __cplusplus
}
#endif
#endif
