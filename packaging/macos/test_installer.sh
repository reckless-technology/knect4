#!/usr/bin/env bash
# The macOS installer held to the stage it was made from, with nothing installed and no root: expanded whole, each part
# at its version and identifier, its payload exactly the staged files byte for byte at their places, the app not
# relocatable, the distribution's floor and licence; the uninstaller whole, the game's, and its script run on the
# payloads laid out as a stand-in for / - taking off the game and leaving another product's app beside it, and a bundle
# at the game's place that is not the game; and with --signed, the product's signature a Developer ID Installer's and
# the app's and the uninstaller's Developer ID Application's with the hardened runtime.
#
#   packaging/macos/test_installer.sh <dist-dir> <version> <pkg> [--signed]
set -euo pipefail

[ $# -ge 3 ] || { sed -n '2,9p' "$0" >&2; exit 2; }
dist=$(cd "$1" && pwd)
version=$2
pkg=$3
signed=${4:-}
work=$(mktemp -d "${TMPDIR:-/tmp}/knect4-pkg-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
fail() { echo "::error::test_installer.sh: $*" >&2; exit 1; }
id=com.recklesstechnology.knect4

pkgutil --expand-full "$pkg" "$work/x" > /dev/null
distribution="$work/x/Distribution"
grep -q '<os-version min="13.3"/>' "$distribution" || fail "the distribution does not hold the floor 13.3"
grep -q '<license file="LICENSE.txt"/>' "$distribution" || fail "the distribution shows no licence"
for part in app docs uninstaller; do
    info="$work/x/$part.pkg/PackageInfo"
    [ -f "$info" ] || fail "no $part.pkg in the product"
    grep -q "identifier=\"$id.$part\"" "$info" || fail "$part.pkg's identifier"
    grep -q "version=\"$version\"" "$info" || fail "$part.pkg is not at $version"
done
grep -q 'relocatable="false"' "$work/x/app.pkg/PackageInfo" || fail "the app is relocatable"
grep -q 'install-location="/Applications"' "$work/x/app.pkg/PackageInfo" || fail "the app's install location"

# The payloads, file for file against the stage.
app="$work/x/app.pkg/Payload/Reckless Technology/Knect4.app"
[ -d "$app" ] || fail "the app is not at /Applications/Reckless Technology"
diff -r "$dist/Knect4.app" "$app" > /dev/null || fail "the app is not the staged app"
docs="$work/x/docs.pkg/Payload/Library/Documentation/Knect 4"
for doc in LICENSE.txt THIRD-PARTY-NOTICES.txt; do
    cmp -s "$dist/$doc" "$docs/$doc" || fail "$doc is not the staged one"
done
extra=$(cd "$work/x" && find app.pkg/Payload docs.pkg/Payload -type f | grep -v "/Knect4.app/" |
    grep -v -e "Library/Documentation/Knect 4/LICENSE.txt$" -e "Library/Documentation/Knect 4/THIRD-PARTY-NOTICES.txt$" || true)
[ -z "$extra" ] || fail "files the stage does not hold: $extra"

# The uninstaller: whole, the game's, at the version, not relocatable - and its script, run as the app runs it (an
# empty environment) on the payloads laid out as a stand-in for /, takes off the game, leaves another product's app in
# the shared folder, and leaves a bundle at the game's own place whose identifier is not the game's.
uninstaller="$work/x/uninstaller.pkg/Payload/Reckless Technology/Uninstall Knect 4.app"
[ -d "$uninstaller" ] || fail "the uninstaller is not at /Applications/Reckless Technology"
codesign --verify --deep --strict "$uninstaller" 2> /dev/null || fail "the uninstaller's signature does not hold"
[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$uninstaller/Contents/Info.plist")" = "$id.uninstall" ] ||
    fail "the uninstaller's identifier"
[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$uninstaller/Contents/Info.plist")" = "$version" ] ||
    fail "the uninstaller is not at $version"
[ -x "$uninstaller/Contents/Resources/uninstall.sh" ] || fail "the uninstaller has no script"
uninstaller_icon=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIconFile' "$uninstaller/Contents/Info.plist")
uninstaller_icon=${uninstaller_icon%.icns}.icns
cmp -s "$uninstaller/Contents/Resources/$uninstaller_icon" "$app/Contents/Resources/icon.icns" ||
    fail "the uninstaller's icon ($uninstaller_icon) is not the app's"
grep -q 'relocatable="false"' "$work/x/uninstaller.pkg/PackageInfo" || fail "the uninstaller is relocatable"
stand_in="$work/root"
mkdir -p "$stand_in/Applications/Reckless Technology" "$stand_in/Library/Documentation"
ditto "$app" "$stand_in/Applications/Reckless Technology/Knect4.app"
ditto "$uninstaller" "$stand_in/Applications/Reckless Technology/Uninstall Knect 4.app"
ditto "$docs" "$stand_in/Library/Documentation/Knect 4"
other="$stand_in/Applications/Reckless Technology/RecklessIRCapture.app"
mkdir -p "$other/Contents"
/usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string com.recklesstechnology.ircapture" "$other/Contents/Info.plist" > /dev/null
other_docs="$stand_in/Library/Documentation/Reckless IR Capture"
mkdir -p "$other_docs" && echo "a sibling product" > "$other_docs/EULA.txt"
removed=$(/usr/bin/env -i RECKLESS_UNINSTALL_ROOT="$stand_in" /bin/bash "$uninstaller/Contents/Resources/uninstall.sh")
[ "$removed" = 2 ] || fail "the uninstaller took off $removed bundles, not the app and itself"
[ ! -e "$stand_in/Applications/Reckless Technology/Knect4.app" ] || fail "the uninstaller left the app"
[ ! -e "$stand_in/Applications/Reckless Technology/Uninstall Knect 4.app" ] || fail "the uninstaller left itself"
[ ! -e "$stand_in/Library/Documentation/Knect 4" ] || fail "the uninstaller left the documents"
[ -d "$other" ] || fail "the uninstaller took another product's app"
[ -f "$other_docs/EULA.txt" ] || fail "the uninstaller took another product's documents"
impostor="$stand_in/Applications/Reckless Technology/Knect4.app"
mkdir -p "$impostor/Contents"
/usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string com.example.impostor" "$impostor/Contents/Info.plist" > /dev/null
/usr/bin/env -i RECKLESS_UNINSTALL_ROOT="$stand_in" /bin/bash "$uninstaller/Contents/Resources/uninstall.sh" > /dev/null
[ -d "$impostor" ] || fail "the uninstaller took a bundle at the game's place that is not the game"

if [ "$signed" = "--signed" ]; then
    pkgutil --check-signature "$pkg" | grep -q "Developer ID Installer: " || fail "the product is not signed by a Developer ID Installer"
    for binary in "$app" "$uninstaller"; do
        codesign --verify --deep --strict "$binary" || fail "$binary's signature"
        codesign --display --verbose=2 "$binary" 2>&1 | grep -q "Authority=Developer ID Application: " ||
            fail "$binary is not signed by a Developer ID Application"
        codesign --display --verbose=2 "$binary" 2>&1 | grep -q "flags=.*runtime" || fail "$binary has no hardened runtime"
        codesign --display --verbose=2 "$binary" 2>&1 | grep -q "^Timestamp=" || fail "$binary has no secure timestamp"
    done
    echo "the app and the uninstaller signed by a Developer ID Application with the hardened runtime and a timestamp"
fi
echo "the installer holds the stage: the app, the documents and the uninstaller at $version${signed:+, signed}"
