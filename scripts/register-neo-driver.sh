#!/usr/bin/env bash
# NEO driver registration in the ONLY existing local MAME tree for neo-diag.
set -Eeuo pipefail
root="${1:?Use: register-neo-driver.sh /path/to/canonical/neo-diag}"
list="$root/src/mame/mame.lst"
driver="$root/src/mame/skeleton/alphasneo.cpp"
test -f "$list" && test -s "$driver"
if ! grep -Fqx '@source:skeleton/alphasneo.cpp' "$list"; then
    printf '\n@source:skeleton/alphasneo.cpp\nasneo\nasneob\nasneow\n' >> "$list"
fi
for entry in asneo asneob asneow; do
    if ! awk '/^@source:skeleton\/alphasneo\.cpp$/ {in_neo=1;next} /^@source:/ {in_neo=0} in_neo && $0 == name {found=1} END {exit !found}' name="$entry" "$list"; then
        # Only register the missing entry in this same source block.
        # The block is already known to exist: add before next @source header.
        awk -v entry="$entry" '
          /^@source:skeleton\/alphasneo\.cpp$/ {print; in_neo=1; next}
          in_neo && /^@source:/ {print entry; in_neo=0}
          {print}
          END {if(in_neo) print entry}
        ' "$list" > "$list.neo.tmp"
        mv "$list.neo.tmp" "$list"
    fi
done
echo "NEO_DRIVER_REGISTRATION_OK (asneo/asneob/asneow)"
