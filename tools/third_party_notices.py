#!/usr/bin/env python3
"""Write THIRD-PARTY-NOTICES.txt: what Knect 4's executable contains from elsewhere, each part's licence as it ships.

Every licence text is read from the source it governs, in the trees the build fetched (FETCHCONTENT_BASE_DIR, read from
the build tree's CMakeCache.txt, or the tree a FETCHCONTENT_SOURCE_DIR_<NAME> entry there names in its place), and every
version from the toolkit's pins (external/reckless-toolkit/cmake/ToolkitPins.cmake), the fetched headers or the fonts
themselves, so the notices can only say what the build compiles. The parts are the ones the executable holds, read off
its link map: Skia and the libraries it bundles, which the window draws through - Skia's own code with its colour
management (skcms) and its PNG decoder, libpng and zlib, whose SIMD routines are the Chromium authors', and on Linux
FreeType; the Noto Serif typeface it carries; and RtAudio and RtMidi, the toolkit's audio device layer, which plays the
clicks. Of the toolkit's other fetches none is in the executable: Monocypher (the licence check), the audio file
decoders (dr_libs, stb) and the ASIO SDK are on no link line of it or have nothing of theirs linked, and GoogleTest is
the suites' alone. The skia-pack carries the licences of libpng, zlib and FreeType, read from it here; Skia's own and
the Chromium source tree's, which the pack lacks, are fetched by the toolkit beside it
(external/reckless-toolkit/skia/CMakeLists.txt) and read from there. The four platforms' packs carry the same files, so
the notices are the same from any build tree; a licence file the pack gains stops the script until it is read.

    python3 tools/third_party_notices.py <build-dir>           writes THIRD-PARTY-NOTICES.txt at the root
    python3 tools/third_party_notices.py <build-dir> --check   exits 1 if the committed file is not what it writes
                                                               (CTest notices)

Any build tree with the window on can write the notices or check them. Run it when the submodule moves, and read the
diff. Any Python 3 runs it, with nothing installed.
"""
import re
import struct
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLKIT = ROOT / "external" / "reckless-toolkit"
WIDTH = 100

# Every licence-like file of the skia-pack release: the three the notices reproduce (libpng's, zlib's, FreeType's), the
# GPL FreeType also offers, those of libwebp and HarfBuzz, which nothing links, and those of ANGLE, SwiftShader and the
# Vulkan headers, whose headers alone the pack carries. A file outside the list is a licence the pack has gained, to be
# read before the notices are written again.
SKIA_PACK_LICENCE_FILES = {
    "third_party/externals/libpng/LICENSE", "third_party/externals/zlib/LICENSE",
    "third_party/externals/libwebp/COPYING", "third_party/externals/libwebp/PATENTS",
    "third_party/externals/harfbuzz/COPYING", "third_party/externals/freetype/docs/FTL.TXT",
    "third_party/externals/freetype/docs/GPLv2.TXT", "third_party/externals/angle2/LICENSE",
    "third_party/externals/swiftshader/LICENSE.txt", "include/third_party/vulkan/LICENSE",
}

APP = "Knect 4"


def read(path):
    return Path(path).read_text(encoding="utf-8", errors="strict").replace("\r\n", "\n")


def uncomment(block):
    """A licence quoted in a C or C++ comment, with the comment's markers taken off and its own layout kept: a run of //
    lines loses each line's // and the space after it; a /* */ block loses its two delimiters alone, since a line inside
    one that starts with an asterisk is the licence's own bullet. Then the common indent goes."""
    lines = [line.rstrip() for line in block.split("\n")]
    if all(line.lstrip().startswith("//") or not line.strip() for line in lines):
        lines = [re.sub(r"^\s*// ?", "", line) for line in lines]
    else:
        lines[0] = re.sub(r"^\s*/\*+ ?", "", lines[0])
        lines[-1] = re.sub(r"\s*\*+/$", "", lines[-1])
    return textwrap.dedent("\n".join(lines)).strip("\n")


def pins():
    """The toolkit's pins the notices name, from its pins file, which its CMakeLists.txt reads."""
    found = dict(re.findall(r'set\((RECKLESS_[A-Z0-9_]+)\s+"([^"]*)"', read(TOOLKIT / "cmake" / "ToolkitPins.cmake")))
    names = ("RECKLESS_RTAUDIO_VERSION", "RECKLESS_RTMIDI_VERSION", "RECKLESS_SKIA_TAG", "RECKLESS_SKIA_COMMIT",
             "RECKLESS_CHROMIUM_COMMIT")
    missing = [n for n in names if n not in found]
    if missing:
        sys.exit(f"the toolkit's pins do not set {missing}")
    return {n: found[n] for n in names}


def fetched_trees(build):
    """A lookup of a fetch's source by its name: the tree a FETCHCONTENT_SOURCE_DIR_<NAME> entry of the cache names in
    the fetch's place, else <FETCHCONTENT_BASE_DIR>/<name>-src."""
    cache_file = Path(build) / "CMakeCache.txt"
    if not cache_file.is_file():
        sys.exit(f"{build} is no configured build tree (it has no CMakeCache.txt)")
    cache = read(cache_file)
    m = re.search(r"^FETCHCONTENT_BASE_DIR:PATH=(.*)$", cache, re.M)
    base = Path(m.group(1).strip()) if m else Path(build) / "_deps"

    def src(name):
        given = re.search(rf"^FETCHCONTENT_SOURCE_DIR_{name.upper()}:PATH=(.*)$", cache, re.M)
        if given and given.group(1).strip():
            return Path(given.group(1).strip())
        return base / f"{name}-src"

    return src


def skia_pack_dir(src):
    """The skia-pack release's root: whichever platform's package the build fetched (each carries the same headers and
    licence files)."""
    for name in ("skia_macos_arm64", "skia_macos_x64", "skia_windows_x64", "skia_linux_x64"):
        if (src(name) / "third_party" / "externals").is_dir():
            return src(name)
    sys.exit("the notices carry the licences of what the window links of the skia-pack, read from the pack, which this "
             "build tree did not fetch: a tree with the window on (KNECT4_GUI)")


def font_revision(path):
    """A font's revision as its head table records it (a 16.16 fixed-point number), to three decimals."""
    data = Path(path).read_bytes()
    count = struct.unpack(">H", data[4:6])[0]
    for i in range(count):
        tag, _, offset, _ = struct.unpack(">4sIII", data[12 + 16 * i: 28 + 16 * i])
        if tag == b"head":
            return f"{struct.unpack('>i', data[offset + 4: offset + 8])[0] / 65536:.3f}"
    sys.exit(f"{path} has no head table")


def wrap(paragraph):
    words, lines, line = paragraph.split(), [], ""
    for word in words:
        if len(line) + len(word) + (line != "") > WIDTH:
            lines.append(line)
            line = ""
        line += ("" if line == "" else " ") + word
    lines.append(line)
    return "\n".join(lines)


def prose(*paragraphs):
    return "\n\n".join(wrap(p) for p in paragraphs)


def section(number, title, where, *parts):
    rule = "=" * WIDTH
    body = "\n\n".join(p.strip("\n") for p in parts if p)
    return f"{rule}\n{number}. {title}\nUsed in: {where}\n{rule}\n\n{body}\n"


def build(build_dir):
    p = pins()
    src = fetched_trees(build_dir)

    fonts = TOOLKIT / "assets" / "fonts"
    ofl = read(fonts / "OFL.txt").strip("\n")
    noto_versions = {font_revision(fonts / f"NotoSerif-{face}.ttf") for face in ("Regular", "Bold")}
    if len(noto_versions) != 1:
        sys.exit(f"Noto Serif's two faces are of different revisions ({sorted(noto_versions)}): name each")
    noto_version = noto_versions.pop()

    for fetched in ("rtaudio", "rtmidi"):
        if not (src(fetched) / "LICENSE").is_file():
            sys.exit(f"this build tree did not fetch {fetched} (the toolkit's audio device layer): configure it with "
                     "the window on")
    rtaudio = read(src("rtaudio") / "LICENSE").strip("\n")
    rtmidi = read(src("rtmidi") / "LICENSE").strip("\n")

    pack = skia_pack_dir(src)
    externals = pack / "third_party" / "externals"
    licence_files = {f.relative_to(pack).as_posix() for f in pack.rglob("*")
                     if f.is_file() and re.fullmatch(r"(licen[cs]e|copying|patents|ftl|gplv?\d*)(\.txt|\.md)?",
                                                     f.name, re.I)}
    if licence_files != SKIA_PACK_LICENCE_FILES:
        sys.exit(f"the skia-pack release's licence files are not the ones the notices read - new: "
                 f"{sorted(licence_files - SKIA_PACK_LICENCE_FILES)}, gone: "
                 f"{sorted(SKIA_PACK_LICENCE_FILES - licence_files)}: read them")
    for header in (pack / "include" / "core" / "SkCanvas.h", pack / "modules" / "skcms" / "skcms.h"):
        if "Use of this source code is governed by a BSD-style license" not in read(header):
            sys.exit(f"{header.name} no longer gives its licence as Skia's BSD-style licence: read it")
    for header in ("adler32_simd.h", "crc32_simd.h", "cpu_features.h", "slide_hash_simd.h"):
        if "found in the Chromium source repository LICENSE file" not in read(externals / "zlib" / header):
            sys.exit(f"zlib's {header} no longer points at the Chromium source repository's LICENSE: read it")
    for fetched in ("skia_licence", "chromium_licence"):
        if not (src(fetched) / "LICENSE").is_file():
            sys.exit(f"the toolkit fetches {fetched} beside the skia-pack, and this build tree has the pack without "
                     "it: configure it again")
    skia_licence = read(src("skia_licence") / "LICENSE").strip("\n")
    chromium_licence = uncomment(read(src("chromium_licence") / "LICENSE"))
    skia_milestone = re.search(r"#define SK_MILESTONE (\d+)",
                               read(pack / "include" / "core" / "SkMilestone.h")).group(1)
    skia_png_version = re.search(r'#define PNG_LIBPNG_VER_STRING "([^"]+)"',
                                 read(externals / "libpng" / "png.h")).group(1)
    skia_zlib_version = re.search(r'#define ZLIB_VERSION "([^"]+)"', read(externals / "zlib" / "zlib.h")).group(1)
    freetype_header = read(externals / "freetype" / "include" / "freetype" / "freetype.h")
    freetype_version = ".".join(re.search(rf"#define FREETYPE_{part}\s+(\d+)", freetype_header).group(1)
                                for part in ("MAJOR", "MINOR", "PATCH"))
    freetype_year = re.search(r"Copyright \(C\) \d{4}-(\d{4}) by", freetype_header).group(1)
    ftl = read(externals / "freetype" / "docs" / "FTL.TXT")
    freetype_credit = " ".join(re.search(r'"""\s*(Portions of this software.*?)\s*"""', ftl, re.S).group(1).split())
    freetype_credit = freetype_credit.replace("<year>", freetype_year)

    # The sections in their order, each numbered by its place.
    keys = ["noto", "rtaudio", "rtmidi", "skia", "png_skia", "zlib_skia", "freetype"]
    n = {key: i + 1 for i, key in enumerate(keys)}
    sections = {
        "noto": (f"Noto Serif {noto_version}", APP, prose(
            "Knect 4 carries Noto Serif's regular and bold faces inside its executable and draws its text in them."),
            ofl),
        "rtaudio": (f"RtAudio {p['RECKLESS_RTAUDIO_VERSION']}", APP, prose(
            "Knect 4 plays its clicks through RtAudio, by way of the toolkit's audio device layer. Modified, as the "
            "build fetches it: its ALSA back end, compiled on Linux, stops a device on an xrun, its stop threshold the "
            "buffer's size where it was past the boundary, and starts a duplex stream's playback on a buffer of "
            "silence; and its ASIO back end, which Knect 4 does not compile, hands the driver a bufferSwitchTimeInfo "
            "callback where it left a null pointer - the fix the project made after this release."), rtaudio),
        "rtmidi": (f"RtMidi {p['RECKLESS_RTMIDI_VERSION']}", APP, prose(
            "Linked with RtAudio in the toolkit's audio device layer."), rtmidi),
        "skia": (f"Skia m{skia_milestone}, skia-pack {p['RECKLESS_SKIA_TAG']}", APP, prose(
            f"Knect 4 draws its window through Skia m{skia_milestone}, linked from the prebuilt static libraries of "
            f"the skia-pack release {p['RECKLESS_SKIA_TAG']}, built from Skia's commit {p['RECKLESS_SKIA_COMMIT']}. "
            "It holds Skia's own code - its raster renderer, its colour management (skcms) and its PNG decoder - and, "
            f"of the libraries the release bundles, libpng and zlib (sections {n['png_skia']} and {n['zlib_skia']}), "
            f"and on Linux FreeType (section {n['freetype']}), which draws the text there; it declines every other "
            "image decoder, so no other bundled library is linked.",
            "Skia's licence, the LICENSE file of its source tree at that commit, which skcms, a part of the same tree, "
            "shares:"), skia_licence),
        "png_skia": (f"libpng {skia_png_version}, in Skia", APP, read(externals / "libpng" / "LICENSE")),
        "zlib_skia": (f"zlib {skia_zlib_version}, in Skia", APP, read(externals / "zlib" / "LICENSE"), prose(
            "Its SIMD routines (adler32_simd, crc32_simd and crc_folding, inffast_chunk, cpu_features and "
            "slide_hash_simd) are the Chromium authors', under the LICENSE file of the Chromium source tree at "
            f"revision {p['RECKLESS_CHROMIUM_COMMIT']}, the revision the zlib commit Skia's DEPS names was copied "
            "from:"), chromium_licence),
        "freetype": (f"FreeType {freetype_version}, in Skia", f"{APP} on Linux", prose(
            "This software is based in part on the work of the FreeType Team. FreeType is used under the FreeType "
            "License, the first of the two licences it offers.", freetype_credit), ftl),
    }

    rows = []
    for key in keys:
        title, used = sections[key][0], sections[key][1]
        rows.append(f"  {n[key]:>2}. {title.split(',')[0]:<44} {used}")
    head = ("THIRD-PARTY NOTICES\n\n" + prose(
        "Knect 4 is copyright (C) 2014-2026 Jonny Reckless and licensed under the GNU General Public License "
        "version 3 or later (LICENSE). Its "
        "executable contains the third-party software below, each part used under its own licence, which this file "
        "reproduces wherever the licence asks for it.") +
        "\n\nWhere each part is used\n\n" + "\n".join(rows) + "\n")
    parts = [head] + [section(n[key], *sections[key]) for key in keys]
    return "\n\n".join(s.rstrip("\n") for s in parts) + "\n"


def main():
    if len(sys.argv) < 2 or sys.argv[1].startswith("--"):
        sys.exit(__doc__)
    text = build(sys.argv[1])
    out = ROOT / "THIRD-PARTY-NOTICES.txt"
    if "--check" in sys.argv:
        current = out.read_text(encoding="utf-8").replace("\r\n", "\n") if out.exists() else ""
        if current != text:
            sys.exit(f"{out.name} is not what {Path(__file__).name} writes from {sys.argv[1]}: run it again")
        print(f"{out.name} is current")
        return
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"wrote {out} ({len(text.splitlines())} lines)")


if __name__ == "__main__":
    main()
