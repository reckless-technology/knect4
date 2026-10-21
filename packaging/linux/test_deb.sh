#!/usr/bin/env bash
# The Debian package held to the stage it was made from, with no root and nothing installed: its control fields, every
# staged file byte for byte at its place with its mode, the menu entry (desktop-file-validate where it is installed) and
# the icon, its md5sums, nothing outside its own files, and every library the binary needs owned by a package its
# Depends names.
#
#   packaging/linux/test_deb.sh <dist-dir> <version> <deb>
set -euo pipefail

[ $# -eq 3 ] || { sed -n '2,7p' "$0" >&2; exit 2; }
dist=$(cd "$1" && pwd)
version=$2
deb=$3
root=$(cd "$(dirname "$0")/../.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/knect4-deb-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
fail() { echo "::error::test_deb.sh: $*" >&2; exit 1; }
id=com.recklesstechnology.knect4

field() { dpkg-deb --field "$deb" "$1"; }
[ "$(field Package)" = knect4 ] || fail "Package is $(field Package)"
[ "$(field Version)" = "$version" ] || fail "Version is $(field Version), not $version"
[ "$(field Architecture)" = amd64 ] || fail "Architecture is $(field Architecture)"
depends=$(field Depends)
echo "Depends: $depends"

dpkg-deb --raw-extract "$deb" "$work/x"
x="$work/x"
cmp -s "$dist/knect4" "$x/opt/reckless-technology/bin/knect4" || fail "knect4 is not the staged one"
[ "$(stat -c %a "$x/opt/reckless-technology/bin/knect4")" = 755 ] || fail "knect4 is not 0755"
for doc in LICENSE.txt THIRD-PARTY-NOTICES.txt; do
    cmp -s "$dist/$doc" "$x/usr/share/doc/knect4/$doc" || fail "$doc is not the staged one"
    [ "$(stat -c %a "$x/usr/share/doc/knect4/$doc")" = 644 ] || fail "$doc is not 0644"
done
[ -s "$x/usr/share/doc/knect4/copyright" ] || fail "no copyright file"
cmp -s "$root/assets/icon/icon_256.png" "$x/usr/share/icons/hicolor/256x256/apps/$id.png" ||
    fail "the icon is not assets/icon/icon_256.png"
desktop="$x/usr/share/applications/$id.desktop"
grep -qx "Exec=/opt/reckless-technology/bin/knect4" "$desktop" || fail "the menu entry runs something else"
grep -qx "Icon=$id" "$desktop" || fail "the menu entry names another icon"
if command -v desktop-file-validate > /dev/null; then desktop-file-validate "$desktop" || fail "the menu entry"; fi
(cd "$x" && md5sum --quiet -c DEBIAN/md5sums) || fail "md5sums"
for script in postinst postrm; do
    [ -x "$x/DEBIAN/$script" ] || fail "$script is not executable"
done
outside=$(cd "$x" && find . \( -type f -o -type l \) ! -path './DEBIAN/*' | grep -v -x -e './opt/reckless-technology/bin/knect4' \
    -e "./usr/share/applications/$id.desktop" -e "./usr/share/icons/hicolor/256x256/apps/$id.png" \
    -e './usr/share/doc/knect4/LICENSE.txt' -e './usr/share/doc/knect4/THIRD-PARTY-NOTICES.txt' \
    -e './usr/share/doc/knect4/copyright' || true)
[ -z "$outside" ] || fail "files the package should not hold: $outside"

# Every library the binary needs is owned by a package Depends names.
for library in $(objdump -p "$dist/knect4" | awk '/NEEDED/ {print $2}' | sort -u); do
    path=$(ldconfig -p | awk -v lib="$library" '$1 == lib && /x86-64/ {print $NF; exit}')
    owner=$(dpkg -S "$(readlink -f "$path")" 2> /dev/null | head -1 | cut -d: -f1 || true)
    [ -n "$owner" ] || owner=$(dpkg -S "$path" 2> /dev/null | head -1 | cut -d: -f1 || true)
    # A package name as a string, not a pattern: libstdc++6's pluses are regex syntax.
    case ", $depends," in
    *", $owner "* | *", $owner,"*) ;;
    *) fail "$library ($owner) is not in Depends" ;;
    esac
done
echo "the package holds the stage at $version; its Depends covers every library the binary needs"
