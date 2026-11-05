/* PNG to 8-bit RGBA rows and back, on libpng: shared by the SDL_image
 * and libgd shims of the Amiga port.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_PNG_RGBA_H
#define AMIGA_PNG_RGBA_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Decodes a whole PNG held in memory. Returns malloc()ed RGBA (w*h*4
 * bytes, R first), or NULL; *has_alpha says whether any pixel is not
 * opaque (tRNS or an alpha channel). */
unsigned char *amiga_png_decode(const void *data, size_t size, int *w, int *h, int *has_alpha);
/* Encodes RGBA rows (R first) as a PNG in malloc()ed memory. */
void *amiga_png_encode(const unsigned char *rgba, int w, int h, int alpha, size_t *size);
#ifdef __cplusplus
}
#endif
#endif
