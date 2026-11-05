/* OpenLieroX for AmigaOS: the SDL_mixer 1.2 calls it makes, on SDL 1.2's
 * own audio (amiga/sdl_mixer_lite.c): WAV samples read and converted once to the
 * output format at load, a few channels mixed in the audio callback.
 * The kit's SDL 1.2 (sdl12-compat) comes without an SDL_mixer 1.2.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_SDL_MIXER_LITE_H
#define AMIGA_SDL_MIXER_LITE_H
#include <SDL.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SDL_MIXER_MAJOR_VERSION 1
#define SDL_MIXER_MINOR_VERSION 2
#define SDL_MIXER_PATCHLEVEL 12
#define MIX_MAJOR_VERSION SDL_MIXER_MAJOR_VERSION
#define MIX_MINOR_VERSION SDL_MIXER_MINOR_VERSION
#define MIX_PATCHLEVEL SDL_MIXER_PATCHLEVEL
#define MIX_MAX_VOLUME 128
#define MIX_DEFAULT_FORMAT AUDIO_S16SYS

typedef struct _Mix_Music Mix_Music; /* no music: Music.cpp has none either */

typedef struct Mix_Chunk {
	int allocated;
	Uint8 *abuf;
	Uint32 alen;
	Uint8 volume;
} Mix_Chunk;

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
void Mix_CloseAudio(void);
int Mix_AllocateChannels(int numchans);
/* from a file only (stdio): see sdl_mixer_lite.c */
Mix_Chunk *Mix_LoadWAV_file(const char *file);
#define Mix_LoadWAV(file) Mix_LoadWAV_file(file)
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
int Mix_Volume(int channel, int volume);
int Mix_HaltChannel(int channel);
#define Mix_GetError SDL_GetError
#define Mix_SetError SDL_SetError
#ifdef __cplusplus
}
#endif
#endif
