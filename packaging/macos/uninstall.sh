#!/bin/bash
# The macOS uninstaller's work, run as root by Uninstall Knect 4.app (packaging/macos/uninstall.applescript, which the
# installer carries in /Applications/Reckless Technology): what packaging/macos/make_installer.sh put on the Mac taken
# off it again, and nothing of the other Reckless products that share the folder -
#   - Knect4.app and this uninstaller in /Applications/Reckless Technology, and the folder when nothing else is left in
#     it;
#   - the licence and the notices in /Library/Documentation/Knect 4;
#   - and the installer's three receipts (pkgutil --forget), so the system no longer counts the parts as installed.
# A bundle goes only when its identifier is the game's own (com.recklesstechnology.knect4), so nothing else that happens
# to share a name is touched. The settings are the person's: the app removes them as the person, if asked. It prints
# how many bundles it took off.
#
# Usage (as root):  uninstall.sh
# RECKLESS_UNINSTALL_ROOT names a folder to stand for / (packaging/macos/test_installer.sh unpacks the package into
# one): the same removals under it, and the receipts left alone.
set -u
root="${RECKLESS_UNINSTALL_ROOT:-}"

removed=0
ours() { # whether a bundle is the game's own, by its identifier
    local id
    id=$(/usr/bin/defaults read "$1/Contents/Info" CFBundleIdentifier 2> /dev/null) || return 1
    [[ "$id" == com.recklesstechnology.knect4 || "$id" == com.recklesstechnology.knect4.* ]]
}

folder="$root/Applications/Reckless Technology"
for bundle in "$folder/Knect4.app" "$folder/Uninstall Knect 4.app"; do
    if [ -d "$bundle" ] && ours "$bundle"; then
        /bin/rm -rf "$bundle" && removed=$((removed + 1))
    fi
done
/bin/rm -f "$folder/.DS_Store" 2> /dev/null
/bin/rmdir "$folder" 2> /dev/null || true # the other products' apps, where they are installed, keep it
/bin/rm -rf "$root/Library/Documentation/Knect 4"
if [ -z "$root" ]; then
    for part in app docs uninstaller; do
        /usr/sbin/pkgutil --forget "com.recklesstechnology.knect4.$part" > /dev/null 2>&1 || true
    done
fi
echo "$removed"
