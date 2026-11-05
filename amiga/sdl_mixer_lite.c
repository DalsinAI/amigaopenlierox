/* The SDL_mixer 1.2 calls OpenLieroX makes, on SDL 1.2's audio (see
 * include/SDL_mixer.h). Output is 16-bit signed, native order, at 22050 Hz
 * whatever the game asks for (OpenLieroX asks 44100 Hz, mono): samples
 * are converted to that once, when they load, so the 68k only adds them up.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#include <stdlib.h>
#include <string.h>
#include "SDL_mixer.h"

#define AMIGA_MIX_RATE 22050
#define AMIGA_MIX_CHANNELS 16

struct channel {
	Mix_Chunk *chunk;
	Uint32 pos;
	int volume;
};

#include <stdio.h>

static struct channel chans[AMIGA_MIX_CHANNELS];
static int nchans = 8;
static int opened;
static SDL_AudioSpec have;

static void mix_callback(void *user, Uint8 *stream, int len)
{
	Sint16 *out = (Sint16 *)stream;
	int frames = len / 2, i, c;
	static Sint32 acc[4096];
	(void)user;

	while (frames > 0) {
		int n = frames > 4096 ? 4096 : frames;
		int any = 0;
		memset(acc, 0, sizeof(Sint32) * n);
		for (c = 0; c < nchans; c++) {
			struct channel *ch = &chans[c];
			const Sint16 *src;
			int left, todo, vol;
			if (!ch->chunk)
				continue;
			left = (int)((ch->chunk->alen - ch->pos) / 2);
			todo = left < n ? left : n;
			src = (const Sint16 *)(ch->chunk->abuf + ch->pos);
			vol = ch->volume * ch->chunk->volume;	/* 0..128*128 */
			for (i = 0; i < todo; i++)
				acc[i] += (src[i] * vol) >> 14;
			ch->pos += (Uint32)todo * 2;
			if (ch->pos >= ch->chunk->alen)
				ch->chunk = NULL;
			any = 1;
		}
		if (any) {
			for (i = 0; i < n; i++) {
				Sint32 v = acc[i];
				out[i] = (Sint16)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
			}
		} else
			memset(out, 0, (size_t)n * 2);
		out += n;
		frames -= n;
	}
}

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize)
{
	SDL_AudioSpec want;
	int c;
	(void)frequency;
	(void)format;
	(void)channels;

	if (opened)
		return 0;
	memset(&want, 0, sizeof(want));
	want.freq = AMIGA_MIX_RATE;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = (Uint16)(chunksize >= 512 ? 1024 : 512);
	want.callback = mix_callback;
	if (SDL_OpenAudio(&want, &have) < 0)
		return -1;
	if (have.format != AUDIO_S16SYS || have.channels != 1) {
		SDL_CloseAudio();
		SDL_SetError("Mix_OpenAudio: no 16-bit mono output");
		return -1;
	}
	for (c = 0; c < AMIGA_MIX_CHANNELS; c++) {
		chans[c].chunk = NULL;
		chans[c].volume = MIX_MAX_VOLUME;
	}
	opened = 1;
	SDL_PauseAudio(0);
	return 0;
}

void Mix_CloseAudio(void)
{
	if (!opened)
		return;
	SDL_CloseAudio();
	opened = 0;
}

int Mix_AllocateChannels(int numchans)
{
	if (numchans < 0)
		return nchans;
	SDL_LockAudio();
	nchans = numchans > AMIGA_MIX_CHANNELS ? AMIGA_MIX_CHANNELS : numchans;
	SDL_UnlockAudio();
	return nchans;
}

/* sdl12-compat's SDL_LoadWAV_RW never returned in the lab (OpenGPU 0.6):
 * the port reads the WAV itself, with stdio, and converts it here.
 * PCM only (8-bit unsigned or 16-bit signed little-endian, mono or stereo,
 * any rate), to 16-bit native mono at the output rate (linear resampling). */
static Uint32 le32(const Uint8 *p) { return p[0] | (p[1] << 8) | ((Uint32)p[2] << 16) | ((Uint32)p[3] << 24); }
static Uint16 le16(const Uint8 *p) { return (Uint16)(p[0] | (p[1] << 8)); }

static Mix_Chunk *chunk_from_wav(const Uint8 *d, Uint32 size)
{
	const Uint8 *fmt = NULL, *data = NULL;
	Uint32 datalen = 0, pos = 12, frames, outframes, i;
	int channels, bits, rate, step;
	Sint16 *out;
	Mix_Chunk *chunk;

	if (size < 12 || memcmp(d, "RIFF", 4) || memcmp(d + 8, "WAVE", 4)) {
		SDL_SetError("Mix_LoadWAV: not a WAV file");
		return NULL;
	}
	while (pos + 8 <= size) {
		Uint32 len = le32(d + pos + 4);
		if (len > size - pos - 8)
			len = size - pos - 8;	/* some files claim more than they have */
		if (!memcmp(d + pos, "fmt ", 4) && len >= 16)
			fmt = d + pos + 8;
		else if (!memcmp(d + pos, "data", 4)) {
			data = d + pos + 8;
			datalen = len;
		}
		pos += 8 + len + (len & 1);
	}
	if (!fmt || !data) {
		SDL_SetError("Mix_LoadWAV: no fmt or data chunk");
		return NULL;
	}
	channels = le16(fmt + 2);
	rate = (int)le32(fmt + 4);
	bits = le16(fmt + 14);
	if (le16(fmt) != 1 || (bits != 8 && bits != 16) || channels < 1 || channels > 2 || rate <= 0) {
		SDL_SetError("Mix_LoadWAV: only 8/16-bit PCM, mono or stereo");
		return NULL;
	}
	step = channels * (bits / 8);
	frames = datalen / (Uint32)step;
	if (frames == 0) {
		SDL_SetError("Mix_LoadWAV: empty");
		return NULL;
	}
	outframes = (Uint32)(((unsigned long long)frames * (Uint32)have.freq) / (Uint32)rate);
	if (outframes == 0)
		outframes = 1;
	out = (Sint16 *)malloc((size_t)outframes * 2);
	chunk = (Mix_Chunk *)malloc(sizeof(*chunk));
	if (!out || !chunk) {
		free(out);
		free(chunk);
		SDL_SetError("Mix_LoadWAV: out of memory");
		return NULL;
	}
	for (i = 0; i < outframes; i++) {
		/* source position in 16.16 fixed point */
		unsigned long long sp = ((unsigned long long)i * (Uint32)rate << 16) / (Uint32)have.freq;
		Uint32 s0 = (Uint32)(sp >> 16), s1 = s0 + 1 < frames ? s0 + 1 : s0;
		int f = (int)(sp & 0xFFFF), c;
		Sint32 a = 0, b = 0;
		for (c = 0; c < channels; c++) {
			if (bits == 8) {
				a += ((int)data[s0 * step + c] - 128) << 8;
				b += ((int)data[s1 * step + c] - 128) << 8;
			} else {
				a += (Sint16)le16(data + s0 * step + c * 2);
				b += (Sint16)le16(data + s1 * step + c * 2);
			}
		}
		a /= channels;
		b /= channels;
		out[i] = (Sint16)(a + (((b - a) * f) >> 16));
	}
	chunk->allocated = 1;
	chunk->abuf = (Uint8 *)out;
	chunk->alen = outframes * 2;
	chunk->volume = MIX_MAX_VOLUME;
	return chunk;
}

Mix_Chunk *Mix_LoadWAV_file(const char *file)
{
	FILE *f;
	long len;
	Uint8 *d;
	Mix_Chunk *chunk = NULL;

	if (!opened) {
		SDL_SetError("Mix_LoadWAV: audio not open");
		return NULL;
	}
	f = fopen(file, "rb");
	if (!f) {
		SDL_SetError("Mix_LoadWAV: cannot open %s", file);
		return NULL;
	}
	if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		SDL_SetError("Mix_LoadWAV: cannot read %s", file);
		return NULL;
	}
	d = (Uint8 *)malloc((size_t)len);
	if (d && fread(d, 1, (size_t)len, f) == (size_t)len)
		chunk = chunk_from_wav(d, (Uint32)len);
	else
		SDL_SetError("Mix_LoadWAV: cannot read %s", file);
	fclose(f);
	free(d);
	return chunk;
}

void Mix_FreeChunk(Mix_Chunk *chunk)
{
	int c;
	if (!chunk)
		return;
	if (opened) {
		SDL_LockAudio();
		for (c = 0; c < AMIGA_MIX_CHANNELS; c++)
			if (chans[c].chunk == chunk)
				chans[c].chunk = NULL;
		SDL_UnlockAudio();
	}
	free(chunk->abuf);
	free(chunk);
}

int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops)
{
	int c, found = -1;
	(void)loops;	/* OpenLieroX plays every sample once */

	if (!opened || !chunk || chunk->alen == 0)
		return -1;
	SDL_LockAudio();
	if (channel >= 0 && channel < nchans)
		found = channel;
	else
		for (c = 0; c < nchans; c++)
			if (!chans[c].chunk) {
				found = c;
				break;
			}
	if (found >= 0) {
		chans[found].chunk = chunk;
		chans[found].pos = 0;
	}
	SDL_UnlockAudio();
	if (found < 0)
		SDL_SetError("Mix_PlayChannel: no free channel");
	return found;
}

int Mix_Volume(int channel, int volume)
{
	int c, old;
	if (channel >= AMIGA_MIX_CHANNELS)
		return 0;
	old = chans[channel < 0 ? 0 : channel].volume;
	if (volume < 0)
		return old;
	if (volume > MIX_MAX_VOLUME)
		volume = MIX_MAX_VOLUME;
	SDL_LockAudio();
	if (channel < 0)
		for (c = 0; c < AMIGA_MIX_CHANNELS; c++)
			chans[c].volume = volume;
	else
		chans[channel].volume = volume;
	SDL_UnlockAudio();
	return old;
}

int Mix_HaltChannel(int channel)
{
	int c;
	if (!opened)
		return 0;
	SDL_LockAudio();
	for (c = 0; c < AMIGA_MIX_CHANNELS; c++)
		if (channel < 0 || c == channel)
			chans[c].chunk = NULL;
	SDL_UnlockAudio();
	return 0;
}
