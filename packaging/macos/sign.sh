#!/usr/bin/env bash
# A staged macOS release (packaging/stage.py macos) signed with the Developer ID Application identity in the keychain
# the toolkit's tools/sign-mac.sh opened: Knect4.app with the hardened runtime, a secure timestamp and no entitlement -
# the game plays its clicks on the system's output and asks for nothing, so the toolkit's sign-mac.sh sign, which gives
# an app the audio-input entitlement, is not the one used here - then held as the system checks it.
#
#   packaging/macos/sign.sh <dist-dir>
set -euo pipefail

dist=${1:?usage: packaging/macos/sign.sh <dist-dir>}
keychain="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/signing.keychain-db"
identity=$(security find-identity -v -p codesigning "$keychain" |
    sed -n 's/.*"\(Developer ID Application: .*\)"/\1/p' | head -n 1)
[ -n "$identity" ] || { echo "::error::no signing keychain: run the toolkit's tools/sign-mac.sh open first" >&2; exit 1; }
app="$dist/Knect4.app"
[ -d "$app" ] || { echo "::error::$dist has no Knect4.app: stage it with packaging/stage.py" >&2; exit 1; }

codesign --force --options runtime --timestamp --keychain "$keychain" --sign "$identity" "$app"
codesign --verify --deep --strict "$app"
codesign --display --verbose=2 "$app" 2>&1 | grep -E "^(Identifier|Authority=Developer|Timestamp|flags)" | head -4
if codesign --display --entitlements - --xml "$app" 2> /dev/null | grep -q "<key>"; then
    echo "::error::$app carries entitlements it does not need" >&2
    exit 1
fi
echo "signed Knect4.app as $identity"
