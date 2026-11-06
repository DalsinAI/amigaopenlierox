#!/bin/sh
# The release archive, for Cradle's stove_build (target package): OpenLieroX
# built with amiga/Makefile.amigaos on the stove's GFOpenGPU developer kit
# (SDL 1.2 on SDL2.module), with zlib 1.3.1 and libpng 1.6.43 fetched at
# pinned commits and built by amiga/deps/build-m68k-libs.sh, and OXP (the
# Team's XML and HTML parser, DalsinAI/gfopenxml at a pinned commit, in
# place of libxml2). The Makefile's package target makes the drawer, and
# amiga/tools/make_package.py makes openlierox-VERSION-68040.lha with its
# .sha256 and .readme: the OpenLieroX drawer (the program and its
# GlowIcons, data, levels, the Classic mod, ReadMe and licences), its icon,
# and beside them the Amiga Installer script Install-OpenLieroX and its icon
# (package/installer/, made by amicradle scripts/port_installer.py from
# package/installer.json). OUT also gets the drawer as it unpacks, as
# OpenLieroX.tar (for a lab), and package.json. VERSION is package/VERSION.
# Nothing is published from here.
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
OUT=${OUT:-"$ROOT/build-amiga-package"}
VERSION=$(cat "$HERE/VERSION")
mkdir -p "$OUT"
OUT=$(CDPATH= cd -- "$OUT" && pwd)
BUILD=$(mktemp -d "${TMPDIR:-/tmp}/openlierox-package.XXXXXX")
trap 'rm -rf "$BUILD"' EXIT INT TERM
cd "$ROOT"

# A stove's compiler is in its prefix/; the GFOpenGPU kit is in the prefix's
# m68k-amigaos/ (include/SDL, lib/libSDL.a).
STOVEROOT=$STOVE
if [ ! -x "$STOVE/bin/m68k-amigaos-gcc" ] && [ -x "$STOVE/prefix/bin/m68k-amigaos-gcc" ]; then
	STOVE="$STOVE/prefix"
else
	STOVEROOT=$(CDPATH= cd -- "$STOVE/.." && pwd)
fi
SDLKIT="$STOVE/m68k-amigaos"
[ -f "$SDLKIT/lib/libSDL.a" ] || { echo "the stove has no GFOpenGPU kit (libSDL.a)"; exit 1; }

fetch() {	# repo ref dir
	git init -q "$3"
	git -C "$3" fetch -q --depth 1 "https://github.com/$1" "$2"
	git -C "$3" checkout -q FETCH_HEAD
}
DEPS="$BUILD/deps"
mkdir -p "$DEPS"
if [ -z "${NDK:-}" ]; then
	git clone -q --depth 1 https://github.com/DalsinAI/gfopensdk "$DEPS/gfopensdk"
	NDK="$DEPS/gfopensdk/OpenNDK/Include_H"
fi
fetch madler/zlib 51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf "$DEPS/zlib-1.3.1"	# v1.3.1
fetch pnggroup/libpng refs/tags/v1.6.43 "$DEPS/libpng-1.6.43"
PNG_COMMIT=$(git -C "$DEPS/libpng-1.6.43" rev-parse HEAD)
for l in zlib png; do
	STOVE="$STOVE" DEPS="$DEPS" PREFIX="$DEPS/m68k-amigaos" sh amiga/deps/build-m68k-libs.sh $l \
		>> "$OUT/deps-output.txt" 2>&1 || { tail -n 60 "$OUT/deps-output.txt"; exit 1; }
done
# OXP with its HTML reader (gfopenxml #21)
OXP_COMMIT=b116948fd738f6df9e6db7388cf8347dad5d7619
fetch DalsinAI/gfopenxml $OXP_COMMIT "$DEPS/gfopenxml"
OS32_GCC16="$STOVEROOT" OUT="$DEPS/oxp" sh "$DEPS/gfopenxml/oxp/build-amiga.sh" \
	>> "$OUT/deps-output.txt" 2>&1 || { tail -n 60 "$OUT/deps-output.txt"; exit 1; }

make -f amiga/Makefile.amigaos STOVE="$STOVE" NDK="$NDK" SDLKIT="$SDLKIT" DEPS="$DEPS/m68k-amigaos" \
	OXP="$DEPS/oxp" -j4 > "$OUT/build-output.txt" 2>&1 || { tail -n 60 "$OUT/build-output.txt"; exit 1; }
cp dist/OpenLieroX/OpenLieroX "$BUILD/OpenLieroX"
# The rest of the drawer as the package target made it. LhA keeps files
# only, so a drawer with nothing in it loses its icon too.
(cd dist/OpenLieroX &&
	find . -mindepth 1 -type d -empty | while read -r d; do rm -f "$d.info"; done &&
	tar --format=ustar --exclude=./OpenLieroX -cf "$BUILD/drawer.tar" .)

# What the ReadMe and the check name: the program's SHA-256, the commit, the
# compiler and the commit's date (the archive's dates, so the same inputs
# make the same bytes).
python3 - "$BUILD" "$STOVE/bin/m68k-amigaos-gcc" "OpenLieroX" "$PNG_COMMIT" "$OXP_COMMIT" <<'PY'
import hashlib, json, subprocess, sys
from pathlib import Path
build, cc, exe, png, oxp = Path(sys.argv[1]), sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5]
git = lambda *a: subprocess.run(["git", *a], capture_output=True, text=True).stdout.strip()
receipt = {
    "executable_sha256": hashlib.sha256((build / exe).read_bytes()).hexdigest(),
    "source_commit": git("rev-parse", "HEAD"),
    "compiler_release": subprocess.run([cc, "--version"], capture_output=True,
                                       text=True).stdout.splitlines()[0],
    "source_date_epoch": int(git("log", "-1", "--format=%ct") or 0),
    "libpng_commit": png,
    "oxp_commit": oxp,
}
(build / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
PY
cp "$BUILD/receipt.json" "$OUT/receipt.json"
python3 "$ROOT/amiga/tools/make_package.py" --build "$BUILD" --version "$VERSION" --out "$OUT"
