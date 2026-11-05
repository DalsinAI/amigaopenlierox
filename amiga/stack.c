/* OpenLieroX recurses in its map, GUI and CSS code and keeps big locals;
 * a Shell gives 4 KB. libnix's startup sets this much up instead.
 * The AmiCradle Team. LGPL 2 or later, like OpenLieroX. */
unsigned long __stack = 1024 * 1024;
static const char version[] __attribute__((used)) = "$VER: OpenLieroX 0.58_rc5 Amiga (05.11.2026)";
