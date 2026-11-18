# R18 — USB host/impresión: resultado de la prueba con ROM original
Fecha: 18 de noviembre de 2026. Alcance: únicamente impresión de `neo-diag`. **NO CALIFICADO COMO FUNCIONAL**.

## Ejecutable instalado y candidato
- Único ejecutable instalado, **R17**: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA-256 `d06a96325657112354172fa82ac96c8fcdd8828e9beaf5286ea23562901eff35`.
- Candidato R18 compilado, **NO instalado**: `/home/spc/Projects/alphasmart/neo-diag/neodiag`, SHA-256 `5750cf0ca4b48dab26fc45309f9e2cb257750c0fa441cd7f8c6118b4025bd4c8`.
- Fuente R18 fijada para prueba nativa: `8b9f5b6fd7d78a550f63acc0c314a8bea3b6c404`.
- [CI GCC y Clang PASS](https://github.com/SamDelorean/asneo-emu/actions/runs/44314898714).

## Evidencia de firmware NEO13 original (sin modificar ROM)
- [Traza de programación HC/PTD y memoria SRAM](https://drive.google.com/file/d/1QgLfd2aJOduy3PZGB_Y_jFggcZRrxJjv/view): **472 escrituras de palabras en host SRAM, 288 escrituras a registros y 48 activaciones de PTD**. Se observan SET_ADDRESS, GET_DESCRIPTOR, SET_CONFIGURATION y exploración de clase USB Hub (descriptor 0x29, PORT_POWER/PORT_RESET/status).
- [Lecturas de retorno de PTD desde firmware](https://drive.google.com/file/d/1EnoFQB23TQK22GyVODhM1b07VCU12Az_/view): 788 lecturas del HC. La mayoría de PTD se completan; algunos de los comandos de Hub sobre la impresora genérica reciben STALL, sin datos impresos.
- El hecho de que el firmware **pruebe** solicitudes de Hub **no prueba** que el hardware del NEO contenga necesariamente un hub físico; se incorporó como opción experimental para verificar compatibilidad del protocolo.

## Código experimental R18 ya incorporado al mismo repositorio
- `neo_usb_function.h`: clase Hub USB 1.1 de dos puertos, descriptores, alimentación, reset 10 ms, status/cambio, ruta de impresora downstream, además de alternativa USB/PCL 03f0:0317 sólo como identidad de periférico virtual (sin motor físico HP).
- `alphasneo.cpp`: cuatro perfiles en `USB_TOPOLOGY`: directo/genérico, hub/genérico, directo/PCL y hub/PCL. Único ejecutable; sin radio y sin cambios a OS3K. Log acotado `NEO_USB_PACKET` para detectar sesiones de paquetes.
- `tests/neo_usb_hub.cpp`: regresión de descriptor Hub, PORT_POWER, PORT_RESET, STATUS, configuración de impresora downstream y transferencia clase printer de 8 bytes.

## Resultado de compilación y prueba integral
Trabajo `NEO_DIAG_R18_USB_PRINT_INTEGRAL_20261118`:
```
MAME_BUILD=PASS
ROM_AUDIT=PASS
UNITS=8_PASS
PROFILE_3_PRN_BYTES=0
PROFILE_1_PRN_BYTES=0
PROFILE_2_PRN_BYTES=0
PROFILE_0_PRN_BYTES=0
PRINT_FROM_ORIGINAL_ROM=NOT_QUALIFIED
STATUS=PARTIAL
R17_INSTALLED=UNCHANGED
```
- [Estado](https://drive.google.com/file/d/1wgl3Q2n9WVd9ckf_fu1IAHV1WNB-knTX/view)
- [Resultados cuatro perfiles](https://drive.google.com/file/d/1qrcceOVL-GAAaBw8SHN3BwNYszMU0IDf/view)
- [Hash instalado/candidato](https://drive.google.com/file/d/1Lt19Qoj1dobjxXvmELKuaeaI_7PT5pGR/view)
- [Trazas del firmware, hub/PCL](https://drive.google.com/file/d/1L10e0YZGEcpG7EvBY0uBLfKQOJAVGL_P/view)
- [Trazas del firmware, directo/genérico](https://drive.google.com/file/d/1mWasmtepbA9cQhDVIeOrnKxjTkd1fEOv/view)

**Defecto de observabilidad pendiente:** las cuatro pruebas exportaron `PROFILE_*_PACKET_LOGS=0`, porque el lanzador MAME no incorporó `-log` para capturar los `logerror("NEO_USB_PACKET")` del driver. Esto **no equivale a ausencia de transacciones**, comprobadas en las trazas PTD anteriores. Para determinar el primer `NAK/STALL/timeout` responsable se requiere ejecutar el candidato con **`-log`**, exportar el archivo `error.log` y correlacionar la última solicitud real con la función `neo_usb::usb_function`. Sin esa captura no hay base para inventar otro registro, puerto, identidad o ACK.

## Límite operativo
Se respetaron las **3 llamadas a Commander** de esta intervención (lanzamiento de los trabajos de traza, readback y prueba integral). Se mantuvo el único ejecutable instalado R17 en todo momento. No generar un archivo PRN ficticio, no marcar PRINT PASS por pruebas de paquete sintéticas y no instalar R18 sin salida de bytes desde la ROM original.
