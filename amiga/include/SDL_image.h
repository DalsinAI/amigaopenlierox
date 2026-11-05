/* OpenLieroX for AmigaOS: the one SDL_image 1.2 call it makes, IMG_Load,
 * on libpng (PNG) and SDL's own SDL_LoadBMP (BMP): amiga/sdl_image_lite.c.
 * The kit's SDL 1.2 (sdl12-compat) comes without an SDL_image 1.2.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_SDL_IMAGE_LITE_H
#define AMIGA_SDL_IMAGE_LITE_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
SDL_Surface *IMG_Load(const char *file);
#define IMG_GetError SDL_GetError
#ifdef __cplusplus
}
#endif
#endif
