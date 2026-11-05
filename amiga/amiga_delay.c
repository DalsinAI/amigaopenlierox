/* SDL_Delay for OpenLieroX's other threads.
 * In the lab (OpenGPU 0.6's SDL 2 under sdl12-compat) a thread that waited
 * through SDL never came back: the thread pool then hung on quit, "waiting
 * for 1 threads to finish". The program is linked with --wrap=SDL_Delay:
 * the main task sleeps through SDL as before, every other task through
 * dos.library's Delay() (whole ticks, 20 ms, at least one).
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
#include <exec/types.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <SDL.h>

static struct Task *main_task;

__attribute__((constructor)) static void delay_init(void)
{
	main_task = FindTask(NULL);
}

void __real_SDL_Delay(Uint32 ms);
void __wrap_SDL_Delay(Uint32 ms)
{
	if (FindTask(NULL) == main_task) {
		__real_SDL_Delay(ms);
		return;
	}
	Delay((ms + 19) / 20 ? (LONG)((ms + 19) / 20) : 1);
}

/* The thread pool's waits (amiga/include/amiga_prelude.h) sleep here, on
 * dos.library alone: one tick. */
void amiga_wait_tick(void)
{
	Delay(1);
}
