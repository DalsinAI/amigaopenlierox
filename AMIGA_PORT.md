OpenLieroX for AmigaOS 3.x
==========================

OpenLieroX 0.58_rc5 (upstream https://github.com/albertz/openlierox, tag
0.58_rc5, commit 44943640075f8c9fb7edefdea900d9deccd0eab9, LGPL 2 or
later), the real-time worms shooter after Liero and Liero Xtreme, for
AmigaOS 3.2 on a 68040 or 68060 with an FPU and an RTG screen, on
OpenGPU's SDL 1.2 (sdl12-compat on SDL 2).

Status: runs in the lab (AmigaChrome, OpenGPU 0.6, 640x480x32 RTG):
loading, the main menu, Local Play with one human against two bots on
Dirt Level with the Classic mod (about 24 frames a second, shown with
Advanced.ShowFPS), back to the menu, and quitting to the Shell with no
screen left behind. Sound: SDL's audio opens (22050 Hz, 16-bit mono) and
every sample loads; the lab is muted, so it has not been heard.


What you need
-------------

- AmigaOS 3.2, a 68040/68060 with an FPU, an RTG screen that SDL 2 can
  open at 640x480 in 16 or 32-bit colour.
- OpenGPU's runtime (OpenUp installs it): opengpu.library and
  LIBS:OpenGPU/SDL2.module (picture, keyboard, mouse, sound through AHI).
- Fast RAM: 32 MB or more; the hi-res levels are big.
- About 40 MB of disk.


Installing
----------

Unpack the archive anywhere (on AmiCradle: SYS:Utilities/OpenCompress x
openlierox-...lha RAM:), then double-click Install-OpenLieroX beside the
OpenLieroX drawer: it checks for a 68040 with FPU and for SDL2.module (in
LIBS:GFOpenGPU/, or copied there from LIBS:OpenGPU/), asks where to put the
game (Work:Games by default) and updates an earlier install in place,
keeping its cfg drawer (options, profiles, logs).


Using it
--------

Open the OpenLieroX icon, or run it from a Shell (it finds its data
through PROGDIR:, so from any directory; give it a big stack, 1 MB, as the
icon does). Options, profiles, logs and screenshots go in the drawer
beside the program (cfg/).

Local play: Local Play in the main menu, one human against bots (the CPU
profiles in cfg/players.dat), on the bundled levels with the Classic mod.
Network play, the server list, IRC chat, the news and the downloads of
maps and mods are off: there is no network here.


How it is built: the route
--------------------------

OpenLieroX's releases (0.58) are SDL 1.2 programs; upstream's master has
moved to SDL 2 and brought in OpenAL, yaml-cpp, Lua, libgd and more
Boost, with twice the source files. 0.58_rc5 needs far less: SDL 1.2,
SDL_image and SDL_mixer (one call each, almost), libxml2, and libcurl,
libzip, libgd and HawkNL for things that are cut here. It uses no Boost in
the game (only Boost.Process for the dedicated server's scripts, cut).

libxml2 is not linked. The game uses it only for HTML: the chat and news
box (CBrowser) and stripping tags from text. amiga/libxml_oxp.c gives it
those calls on OXP, the Team's own parser (DalsinAI/gfopenxml), whose HTML
reader (oxp_html.h) reads the markup; the tree is copied into nodes of the
shape the game walks, so chat lines that scroll away give their memory back.

Our own SDL 1.2 shim, acsdl12, draws in 256 colours only, and OpenLieroX
draws in 16/32 bits with alpha blending everywhere, so acsdl12 cannot run
it (it also has no SDL_image beyond 8-bit PNGs; it has threads, which
OpenLieroX needs). The OpenGPU kit's SDL 1.2 can: it is sdl12-compat on
SDL 2 (lib/libSDL.a), with true colour, threads, audio and SDL_LoadBMP.
The kit has no SDL_image or SDL_mixer for SDL 1.2, so the port has small
ones of its own:

- amiga/sdl_image_lite.c: IMG_Load. PNGs through libpng (32-bit surfaces,
  SDL_SRCALPHA when a pixel isn't opaque), everything else to SDL_LoadBMP
  (OpenLieroX's pictures are PNG and BMP). OpenGPU's SDL2_image.module is
  not used, so its RGB-PNG heap bug can't happen; the drawer's RGB PNGs are
  stored as RGBA anyway (amiga/tools/png_rgba.py, as the Team does).
- amiga/sdl_mixer_lite.c: the SDL_mixer calls the game makes (open, load
  WAV, play once on a free channel, volume, free, close) on SDL 1.2's
  audio: 16 channels mixed into 16-bit mono at 22050 Hz (the game asks
  44100). It reads the WAVs itself (PCM, 8/16-bit, mono/stereo, any rate,
  converted once at load): sdl12-compat's SDL_LoadWAV_RW never returned in
  the lab, and the game hung at "Loading sounds". There is no music in
  OpenLieroX 0.58 (Music.cpp is a dummy upstream).
- amiga/png_rgba.c, amiga/gd_png.c: the part of libgd OpenLieroX uses, on
  libpng: reading a PNG from memory (the hi-res .lxl levels) and writing a
  PNG (screenshots; JPEG and GIF screenshots are not written).

Networking. A local game is a network game to 127.0.0.1: the menu starts
a server on a UDP port and the client connects to it. So
amiga/Networking_loopback.cpp replaces src/common/Networking.cpp (and
HawkNL): UDP sockets in memory, a datagram to 127.0.0.1:port goes onto
the queue of the socket bound to that port in the program, and Read()
makes its sender the socket's remote address, as HawkNL does. Datagrams
to anywhere else are dropped; TCP and name lookups fail, as with no
network. UDP sockets may "listen" (the server does) and the menu's LAN
broadcast socket opens (the menu won't start without it), but its
broadcasts go nowhere. No bsdsocket.library is needed.

Cut, with stub headers in amiga/include so that upstream's code stays:
libcurl (every transfer fails at once: curl/curl.h), libzip (mod
download unzip: zip.h), the dedicated server's Python and other scripts
(amiga/Process_stub.cpp instead of src/common/Process.cpp), the crash
reporter (breakpad: NBREAKPAD, as upstream builds by default; no signal
handlers, no core dumps), GeoIP's database (not in the drawer: countries
show as unknown), opening a web browser, the forked stdout tee (the log
goes to a file with freopen(), as on Windows).


Changes to upstream's files
---------------------------

- src/common/FindFile.cpp: the one base search path is PROGDIR:; home
  is PROGDIR:, the temporary drawer T:; a name with a colon is absolute;
  GetExactFileName() needs no case-insensitive search (AmigaDOS names are
  case-insensitive); no realpath() or pwd.h.
- amiga/amiga_paths.c (from the Commander Genius port): the program is
  linked with --wrap for fopen, open, stat, opendir, mkdir, remove,
  rename, chdir, access, SDL_RWFromFile ..., so that upstream's
  Unix-style joins ("PROGDIR:" + "/" + "cfg/options.cfg", "./x", "a//b")
  become AmigaDOS names.
- src/client/AuxLib.cpp: 16 or 32-bit colour asked for outright (OpenGPU's
  SDL 2 calls its window 24-bit), no OpenGL; amiga/amiga_screens.c (from
  the Seven Kingdoms port) closes the empty screens SDL2.module 0.6 leaves
  behind after a full-screen quit.
- src/common/ThreadPool.cpp, include/CGameScript.h: see above.
- src/main.cpp: the thread pool starts with 4 threads, not 40 (each is an
  AmigaOS process; the pool grows when it needs more); no fork/mmap stdout
  tee; SDL's events polled (above); the program's drawer is PROGDIR: (upstream searched getenv("PATH"),
  NULL here, and from the Workbench argv[0] is no name).
- src/common/Debug.cpp: no gdb core dumps. src/client/CrashHandler.cpp: the
  dummy crash handler. src/client/OpenExternBrowser.cpp: no browser.
- include/StringUtils.h: libnix has its own strlwr(). include/XMLutils.h:
  newer libxml2 declares xmlNodePtr in libxml/tree.h (amiga/include/libxml
  has it there too).
- amiga/include/amiga_prelude.h (included first): sigjmp_buf, sigsetjmp
  and siglongjmp as plain setjmp/longjmp (libnix has no sig* forms).

Threads. OpenLieroX runs its game loop on a thread of its own pool and
keeps the first task for SDL's events and the screen; skins, file lists
and timers run on more threads. Three things in OpenGPU 0.6's SDL 2 under
sdl12-compat did not hold up in the lab, so the port goes around them:
- SDL_WaitEvent did not wake for the events the game thread pushes (each
  frame, quit): the game stalled until the mouse moved. src/main.cpp polls
  (SDL_PollEvent and a 2 ms SDL_Delay) on the Amiga.
- Waits on conditions (and SDL_Delay on a task other than the main one)
  did not always come back. amiga/include/amiga_prelude.h makes
  SDL_CondWait let go of the mutex, sleep one tick through dos.library and
  take it again (every wait in OpenLieroX re-checks its condition in a
  loop); amiga/amiga_delay.c (--wrap=SDL_Delay) sleeps other tasks
  through dos.library's Delay().
- SDL_WaitThread never came back for one of the pool's threads at quit:
  src/common/ThreadPool.cpp counts its threads and waits for the count.
- libnix's stdio has no lock: two threads opening files at once lost
  files at start-up ("Could not initialize the menu system").
  amiga/amiga_paths.c opens and closes files and drawers under one Exec
  semaphore.

Big-endian and alignment: OpenLieroX ran on PowerPC Macs, and its file and network
readers swap bytes through include/EndianSwap.h on SDL_BYTEORDER, which
sdl12-compat sets to big-endian on the 68k. The 68k aligns an int to 2
bytes, not 4: a mod's script.lgs header (char[18], then the version) is
read whole, so include/CGameScript.h aligns that field to 4 on the Amiga
(the Classic mod said "wrong version 458752" before).



Standard keys
-------------

Every AmiCradle game has the same keys on Right Amiga (GFOpenUp's
SDL2.module, after 0.8.26):

- Right Amiga + F: window or full screen, remembered for next time
- Right Amiga + P: pause (Right Amiga + P again goes on)
- Right Amiga + Q: quit
- Right Amiga + M: free the mouse pointer, or let the game take it again
- Right Amiga + S: a screenshot in the drawer's Screenshots
- Help: this game's keys (the drawer's Controls file)
- Right Amiga + F1, F2: F11, F12; Right Amiga + cursor keys: Page Up,
  Page Down, Home, End

The FULLSCREEN tooltype on the game's icon (take the brackets off) starts
it full screen.


Building
--------

    make -f amiga/Makefile.amigaos -j2 \
         STOVE=~/CradleLab/toolchain/openamigagcc-16.2.0-open4-x86_64-linux \
         NDK=~/CradleLab/gfopensdk/OpenNDK/Include_H \
         SDLKIT=/mnt/amigachrome/team2/deps/opengpu-sdk \
         DEPS=/mnt/amigachrome/team2/deps/m68k-amigaos \
         OXP=~/gfopenxml/oxp/build-amiga

(those are the defaults). DEPS holds the Team's static zlib and libpng:
amiga/deps/build-m68k-libs.sh builds them (zlib, png). OXP is
DalsinAI/gfopenxml's oxp/build-amiga.sh output (include/, lib/liboxp.a).
package/build-amiga.sh does all of it on a Cradle stove.
The program is build/amigaos/OpenLieroX (OpenLieroX.debug unstripped), the
drawer dist/OpenLieroX with dist/OpenLieroX.info.

Flags: -noixemul -m68040 -m68881 -O2 -std=gnu++11 -fpermissive and GCC
16's -fno-tree-loop-distribute-patterns; linked with the toolchain's
libpthread (libstdc++'s threads; OpenLieroX's own are SDL threads). The
program passes openrtg's fpcr_check.py: src/client/CClient.cpp is built
with -m68020 -m68881, because GCC 16 at -m68040 put FPCR in the register
of a float-to-int store in CClient::ProcessMapDownloads (network map
downloads, never reached here; -m68020 converts with fintrz).

Icons: amiga/icons/icons.json, from our own GlowIcon art
(amiga/icons/art/openlierox.svg):

    python3 /mnt/amigachrome/team2/tools/amicradle/scripts/port_icons.py \
        amiga/icons/icons.json --out amiga/icons


Data and licences
-----------------

The drawer has upstream's data from share/gamedir: data/ (graphics,
sounds, menus, fonts; the flags are public domain), the Classic mod (the
default mod), the levels, a few worm skins, the default profiles and
weapon restrictions. Left out: the mods, levels and skins drawn after
other people's games and characters (Zelda, Mario, Naruto, Star Wars,
Warhammer, Simpsons, Kirby ...), the other community mods (they stay in
the repository; copy any of them into the drawer to play them), the
dedicated server's scripts and server lists, GeoIP.dat and the Commander
Keen tile tables. The repository states no licence of their own for the
mods, levels and skins; OpenLieroX ships them with the game under its
packaging (LGPL). See Licences in the drawer, and COPYING.LIB.


Known limits
------------

- About 24 frames a second in the lab with three worms on Dirt Level.
- Sound not heard yet (the lab is muted); 16 channels, 22050 Hz mono.
- A few pictures the mods and menus name are not in upstream's data
  either (Classic/gfx/None.png, MiniNuke.png, ZimmFlash.png,
  data/frontend/box_*.png, background_chat*.png): the game uses its
  defaults, as on other systems.
- No network play, server list, IRC, news, downloads, dedicated server;
  no JPEG/GIF screenshots (PNG and BMP are written).
