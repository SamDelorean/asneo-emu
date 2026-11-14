#!/usr/bin/env bash
# NEO driver registration in the ONLY existing local MAME tree for neo-diag.
set -Eeuo pipefail
root="${1:?Use: register-neo-driver.sh /path/to/canonical/neo-diag}"
list="$root/src/mame/mame.lst"
driver="$root/src/mame/skeleton/alphasneo.cpp"
test -f "$list" && test -s "$driver"
if ! grep -Fqx '@source:skeleton/alphasneo.cpp' "$list"; then
    printf '\n@source:skeleton/alphasneo.cpp\nasneo\n' >> "$list"
fi
grep -Fxq 'asneo' "$list"
echo "NEO_DRIVER_REGISTRATION_OK"
