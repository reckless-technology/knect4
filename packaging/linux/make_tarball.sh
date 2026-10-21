#!/usr/bin/env bash
# The portable Linux archive of a staged release (packaging/stage.py linux): the stage whole in one folder,
# knect4-<version>-linux-x64/ - the game, its licence and the notices - in a tar.xz with every file's mode kept, so the
# game stays executable once unpacked, owned by no one in particular (root, numerically) and dated by
# SOURCE_DATE_EPOCH's or the last commit's time, so one stage makes one archive. Then the archive is held to the stage:
# unpacked into a folder of its own, every file the staged one byte for byte with its execute bits, and nothing else.
#
#   packaging/linux/make_tarball.sh <dist-dir> <version> <out.tar.xz>
set -euo pipefail

[ $# -eq 3 ] || { sed -n '2,8p' "$0" >&2; exit 2; }
dist=$(cd "$1" && pwd)
version=$2
out=$3
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "make_tarball.sh: the version is MAJOR.MINOR.BUILD, not $version" >&2; exit 2; }
[ -x "$dist/knect4" ] || { echo "make_tarball.sh: $dist has no knect4: stage it with packaging/stage.py" >&2; exit 1; }
root=$(cd "$(dirname "$0")/../.." && pwd)
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$root" log -1 --format=%ct 2> /dev/null || date +%s)}
work=$(mktemp -d "${TMPDIR:-/tmp}/knect4-tarball.XXXXXX")
trap 'rm -rf "$work"' EXIT
name="knect4-$version-linux-x64"
fail() { echo "::error::make_tarball.sh: $*" >&2; exit 1; }

cp -a "$dist" "$work/$name"
mkdir -p "$(dirname "$out")"
out=$(cd "$(dirname "$out")" && pwd)/$(basename "$out")
rm -f "$out"
(cd "$work" && tar --sort=name --owner=0 --group=0 --numeric-owner --mtime="@$SOURCE_DATE_EPOCH" -cJf "$out" "$name")

mkdir "$work/check"
tar -xJf "$out" -C "$work/check"
[ -d "$work/check/$name" ] || fail "the archive holds no $name/"
files=0
while IFS= read -r -d '' file; do
    relative=${file#"$dist/"}
    unpacked="$work/check/$name/$relative"
    cmp -s "$file" "$unpacked" || fail "$relative is not the staged one"
    if [ -x "$file" ] && [ ! -x "$unpacked" ]; then fail "$relative lost its execute bit"; fi
    files=$((files + 1))
done < <(find "$dist" -type f -print0)
[ "$(find "$work/check/$name" -type f | wc -l)" = "$files" ] || fail "the archive holds files the stage does not"
echo "wrote $out ($(du -h "$out" | cut -f1)): $files files, each the staged one"
