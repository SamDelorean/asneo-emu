#!/usr/bin/env bash
# Single neo-diag MAME executable + original firmware USB print -> PDF.
# A spool adapter, never a second emulator.
set -Eeuo pipefail
umask 077
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/neo-diag"
MODE=asneow
if [[ "$#" -gt 0 ]];then MODE="$1";shift;fi
[[ "$MODE" == asneow || "$MODE" == asneob ]] || { echo "Expected asneow or asneob" >&2;exit 2;}
[[ -x "$BIN" ]] || { echo "Missing canonical emulator: $BIN" >&2;exit 3;}
ROM=/home/spc/Projects/alphasmart/private/roms/neo/mame_sets
OUT="$HOME/Documentos/AlphaSmart/PDF"
SPOOL="$HOME/.cache/neo-diag-print"
if [[ -v NEO_ROMPATH ]];then ROM="$NEO_ROMPATH";fi
if [[ -v ALPHASMART_PDF_DIR ]];then OUT="$ALPHASMART_PDF_DIR";fi
if [[ -v ALPHASMART_PRINT_SPOOL ]];then SPOOL="$ALPHASMART_PRINT_SPOOL";fi
mkdir -p "$OUT" "$SPOOL"
stamp="$(date -u +%Y%m%d_%H%M%S)_$$"
prn="$SPOOL/NEO_$stamp.prn"
pdf="$OUT/NEO_$stamp.pdf"
: > "$prn"
set +e
"$BIN" "$MODE" -rompath "$ROM" -prin "$prn" "$@"
rc=$?
set -e
if [[ ! -s "$prn" ]];then
  echo "NEO USB printer received no bytes. PDF not created." >&2
  exit "$((rc?rc:4))"
fi
format=$(python3 - "$prn" "$SPOOL/NEO_$stamp.ps" <<'PY'
import sys
data=open(sys.argv[1],'rb').read()
if data.startswith(b'%PDF-'):
 print('pdf')
elif 0<=data.find(b'%!PS')<8192:
 p=data.find(b'%!PS'); eof=data.rfind(b'%%EOF')
 eof=eof+5 if eof>=p else len(data)
 open(sys.argv[2],'wb').write(data[p:eof]+b'\n')
 print('ps')
elif b'\x1bE' in data[:32] or data.startswith(b'\x1b%'):
 print('pcl')
else:print('unknown')
PY
)
case "$format" in
 pdf) cp "$prn" "$pdf";;
 ps)
   command -v gs >/dev/null || { echo "Ghostscript unavailable; raw PRN: $prn" >&2;exit 5; }
   gs -q -dSAFER -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile="$pdf" "$SPOOL/NEO_$stamp.ps";;
 pcl)
   if command -v gpcl6 >/dev/null;then
     gpcl6 -q -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile="$pdf" "$prn"
   elif command -v pcl6 >/dev/null;then
     pcl6 -q -dBATCH -dNOPAUSE -sDEVICE=pdfwrite -sOutputFile="$pdf" "$prn"
   else
     echo "PCL captured but GhostPCL unavailable. Raw PRN: $prn" >&2;exit 6
   fi;;
 *) echo "Unknown print language. Raw PRN retained: $prn" >&2;exit 7;;
esac
if [[ ! -s "$pdf" ]] || ! command -v pdfinfo >/dev/null || ! pdfinfo "$pdf" | grep -q '^Pages:';then
 rm -f "$pdf"
 echo "PDF invalid; raw PRN retained: $prn" >&2;exit 8
fi
echo "Original AlphaWord print converted to verified PDF: $pdf"
exit "$rc"
