/* OpenLieroX for AmigaOS: included first in every upstream file
 * (-include). libnix has setjmp/longjmp but not the sig* forms that
 * main.cpp's "recover from a crash" loop names; signals are not saved
 * on AmigaOS anyway. And SDL_CondWait polls (below).
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#ifndef AMIGA_PRELUDE_H
#define AMIGA_PRELUDE_H
#include <setjmp.h>
#ifndef sigsetjmp
typedef jmp_buf sigjmp_buf;
#define sigsetjmp(env, savesigs) setjmp(env)
#define siglongjmp(env, val) longjmp(env, val)
#endif

/* Waiting on a condition in the lab (OpenGPU 0.6's SDL 2 under
 * sdl12-compat) sometimes never came back, even with a timeout: the thread
 * pool then hung on quit, "waiting for 1 threads to finish". Every wait in
 * OpenLieroX re-checks its condition in a loop, so here a wait does not use
 * the condition at all: it lets go of the mutex, sleeps a tick (20 ms,
 * through dos.library, not SDL) and takes it again. Signals stay as they are (no one needs them now). */
#ifdef __cplusplus
#include <SDL_mutex.h>
extern "C" void amiga_wait_tick(void); /* amiga/amiga_delay.c: dos.library Delay(1) */
static inline int amiga_cond_wait(SDL_cond *, SDL_mutex *m)
{
	SDL_mutexV(m);
	amiga_wait_tick();
	SDL_mutexP(m);
	return 0;
}
#define SDL_CondWait(cond, mutex) amiga_cond_wait((cond), (mutex))
#endif
#endif
