#!/usr/bin/env bash
# The Linux installer of a staged release (packaging/stage.py linux): a Debian package, knect4, made with dpkg-deb on the
# machine that built the binary -
#
#   /opt/reckless-technology/bin/knect4
#       the game, beside the organisation's other apps
#   /usr/share/applications/com.recklesstechnology.knect4.desktop
#       the menu entry
#   /usr/share/icons/hicolor/256x256/apps/com.recklesstechnology.knect4.png
#       the game's icon (assets/icon/icon_256.png, tools/make_icon.py)
#   /usr/share/doc/knect4/
#       LICENSE.txt, THIRD-PARTY-NOTICES.txt, copyright
#
# every file the staged one byte for byte, owned by root, 0755 where it runs and 0644 elsewhere, its time
# SOURCE_DATE_EPOCH's or the last commit's, so one stage makes one package. Its Depends is read off the binary: every
# library it needs (objdump -p) found where the loader finds it and named by the package that owns it (dpkg -S), libc6
# at the highest GLIBC_ version it asks for and libstdc++6 at the highest GLIBCXX_ - a library no package owns stops it.
# Removing it takes its own files alone: dpkg removes a folder it shares with another product's package -
# /opt/reckless-technology/bin among them - only once nothing is left in it.
#
#   packaging/linux/make_deb.sh <dist-dir> <version> <out.deb>
set -euo pipefail

[ $# -eq 3 ] || { sed -n '2,20p' "$0" >&2; exit 2; }
dist=$(cd "$1" && pwd)
version=$2
out=$3
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "make_deb.sh: the version is MAJOR.MINOR.BUILD, not $version" >&2; exit 2; }
for file in knect4 LICENSE.txt THIRD-PARTY-NOTICES.txt; do
    [ -e "$dist/$file" ] || { echo "make_deb.sh: $dist has no $file: stage it with packaging/stage.py" >&2; exit 1; }
done
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
for tool in dpkg-deb dpkg objdump ldconfig; do
    command -v "$tool" > /dev/null || { echo "make_deb.sh: $tool is not on the PATH (a Debian-family Linux)" >&2; exit 1; }
done
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-$(git -C "$root" log -1 --format=%ct 2> /dev/null || date +%s)}
work=$(mktemp -d "${TMPDIR:-/tmp}/knect4-deb.XXXXXX")
trap 'rm -rf "$work"' EXIT
pkg="$work/knect4"
id=com.recklesstechnology.knect4

install -d -m 0755 "$pkg/DEBIAN" "$pkg/opt/reckless-technology/bin" "$pkg/usr/share/applications" \
    "$pkg/usr/share/icons/hicolor/256x256/apps" "$pkg/usr/share/doc/knect4"
install -m 0755 "$dist/knect4" "$pkg/opt/reckless-technology/bin/knect4"
install -m 0644 "$here/$id.desktop" "$pkg/usr/share/applications/$id.desktop"
install -m 0644 "$root/assets/icon/icon_256.png" "$pkg/usr/share/icons/hicolor/256x256/apps/$id.png"
install -m 0644 "$dist/LICENSE.txt" "$dist/THIRD-PARTY-NOTICES.txt" "$pkg/usr/share/doc/knect4/"
cat > "$pkg/usr/share/doc/knect4/copyright" <<COPYRIGHT
Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
Upstream-Name: Knect 4
Upstream-Contact: Jonny Reckless <hello@recklesstechnology.com>
Source: https://recklesstechnology.com

Files: *
Copyright: 2014-2026 Jonny Reckless
License: MIT
 The text is /usr/share/doc/knect4/LICENSE.txt. The third-party parts the game contains are used under the licences
 /usr/share/doc/knect4/THIRD-PARTY-NOTICES.txt reproduces.
COPYRIGHT
chmod 0644 "$pkg/usr/share/doc/knect4/copyright"

# What the binary needs, by package.
binary="$dist/knect4"
libraries=$(objdump -p "$binary" | awk '/NEEDED/ {print $2}' | sort -u)
packages=()
for library in $libraries; do
    path=$(ldconfig -p | awk -v lib="$library" '$1 == lib && /x86-64/ {print $NF; exit}')
    [ -n "$path" ] || { echo "make_deb.sh: $library is not where the loader finds libraries" >&2; exit 1; }
    owner=$(dpkg -S "$(readlink -f "$path")" 2> /dev/null | head -1 | cut -d: -f1) || true
    [ -n "$owner" ] || owner=$(dpkg -S "$path" 2> /dev/null | head -1 | cut -d: -f1) || true
    [ -n "$owner" ] || { echo "make_deb.sh: no package owns $library ($path)" >&2; exit 1; }
    packages+=("$owner")
done
version_of() { # the highest $1 version symbol the binary asks for
    { objdump -T "$binary" 2> /dev/null || true; } | { grep -o "$1_[0-9.]*" || true; } | sed "s/$1_//" | sort -V | tail -1
}
glibc=$(version_of GLIBC)
glibcxx=$(version_of GLIBCXX)
# The GCC release whose libstdc++ first carried that symbol version.
case "$glibcxx" in
3.4.29) gcc=11 ;;
3.4.30) gcc=12 ;;
3.4.31) gcc=13.1 ;;
3.4.32) gcc=13.2 ;;
3.4.33) gcc=14 ;;
3.4.34) gcc=15 ;;
*) echo "make_deb.sh: GLIBCXX_$glibcxx: name the GCC release that brought it in this table" >&2; exit 1 ;;
esac
depends=$(for p in "${packages[@]}"; do
    case "$p" in
    libc6) echo "libc6 (>= $glibc)" ;;
    libstdc++6) echo "libstdc++6 (>= $gcc)" ;;
    *) echo "$p" ;;
    esac
done | sort -u | paste -sd, - | sed 's/,/, /g')
size=$(du -sk --apparent-size "$pkg" | cut -f1)

cat > "$pkg/DEBIAN/control" <<CONTROL
Package: knect4
Version: $version
Architecture: amd64
Maintainer: Jonny Reckless <hello@recklesstechnology.com>
Installed-Size: $size
Depends: $depends
Section: games
Priority: optional
Homepage: https://recklesstechnology.com
Description: four in a row, against the computer or a friend
 Knect 4 drops pucks into an upright board: four in a row across, up or
 diagonally wins. Play the computer at three levels, or a friend at one
 board, with the drop's fall, bounce and click.
CONTROL
cat > "$pkg/DEBIAN/postinst" <<'SCRIPT'
#!/bin/sh
set -e
if [ "$1" = configure ]; then
    command -v update-desktop-database > /dev/null && update-desktop-database -q /usr/share/applications || true
    command -v gtk-update-icon-cache > /dev/null && gtk-update-icon-cache -q -t /usr/share/icons/hicolor || true
fi
SCRIPT
cat > "$pkg/DEBIAN/postrm" <<'SCRIPT'
#!/bin/sh
set -e
if [ "$1" = remove ] || [ "$1" = purge ]; then
    command -v update-desktop-database > /dev/null && update-desktop-database -q /usr/share/applications || true
    command -v gtk-update-icon-cache > /dev/null && gtk-update-icon-cache -q -t /usr/share/icons/hicolor || true
fi
SCRIPT
chmod 0755 "$pkg/DEBIAN/postinst" "$pkg/DEBIAN/postrm"
(cd "$pkg" && find . -type f ! -path './DEBIAN/*' -printf '%P\0' | sort -z | xargs -0 md5sum > DEBIAN/md5sums)
chmod 0644 "$pkg/DEBIAN/md5sums"
find "$pkg" -exec touch -h -d "@$SOURCE_DATE_EPOCH" {} +
mkdir -p "$(dirname "$out")"
dpkg-deb --root-owner-group -Zxz --build "$pkg" "$out" > /dev/null
echo "wrote $out ($(du -h "$out" | cut -f1)): Depends: $depends"
