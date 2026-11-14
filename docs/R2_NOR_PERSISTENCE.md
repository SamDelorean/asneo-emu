# R2 — NOR virtual y persistencia (2026-11-14)

## Fuentes y revisión de origen
- `SamDelorean/DANA-emu/src/mame/skeleton/alphasdana.cpp` y `docs/DANA_NOR_FLASH_20261026.md`: patrón de registro, perfiles, evidencia y protección del medio; **Dana usa un Intel/Numonyx M28W640HCB, comandos distintos; NO copiados al NEO**.
- `isotherm/mame` rama `alphasmart`: uso de `NVRAM(config,...).set_custom_handler` para sembrar Flash con firmware sólo sin medio persistente.
- Greg Kennedy / Kristian Sims, Hacking the AlphaSmart NEO (2017): desmontaje NEO con `M29W320EB` reportada por un propietario (https://hackaday.io/project/25732-hacking-the-alphasmart-neo/log/62397-whats-inside); chip de nuestra placa no inspeccionado.
- ST M29W320ET/EB, Rev 5, 2007, tablas de comandos x16, JEDEC ID y geometría: https://www.mouser.com/datasheet/2/389/stmicroelectronics_M29W320ET%2CEB-1206358.pdf
- `ioma8/neo-re`: `smallos3kneorom.os3kos`, sistema segmentado en `0x00410000`, applets a partir de `0x00470000`; referencia de firmware, no ROM física completa.

## Implementación real
- `neo_nor.h`: arreglo de 4 MiB (opcional **ST M29W320EB**, ID ST=0x0020, chip bottom=0x2257), dos regiones 8×8KiB + 63×64KiB; 0x55/0x98 para CFI; 0x555 AA / 0x2AA 55 / 0x555 command; programa 1→0 y bloque/chip erase sólo con secuencia completa; unlock bypass; rechazo de comandos parciales, desconocidos y cambios 0→1.
- `alphasneo.cpp`: ventana de Flash `0x00400000..0x007fffff` completa, NVRAM SRAM 512KiB, NVRAM Flash 4MiB, carga de firmware a **semilla virtual** sin sobrescribir sesión preexistente. NOR profile default UNKNOWN read-only; perfil ST explícito. No modificar ROMs originales.
- `tests/neo_nor.cpp`: regresiones de identidad, comandos de desbloqueo, geometría, CFI, límites de programa/borrado y conservación de bytes después de reset.
- CI en GCC/Clang `-Werror` extendida al controlador NOR.

## Estado y límites
- **NO** es un volcado físico de 4MiB: al crear un medio virtual desde paquetes de actualización, los bytes no suministrados se inician en estado borrado FF por la **convención explícita de medio virtual virgen**. El estado original de Flash de un NEO usado sigue desconocido.
- **NO** se inventa topmark del aparato concreto: NOR desconocida es read-only por defecto; la M29W320EB es una elección diagnóstica explícita.
- **NO** se simulan aún latencias program/erase, DQ6 toggle, estados DQ7/DQ5, protección física ni acceso a x8/byte bus; finalización de operaciones simplificada e inmediata. Si el firmware necesita polling dinámico, se debe adaptar mediante evidencia.
- **NO** se ha acreditado compilación del driver contra MAME, carga de firmware original ni persistencia nativa de MAME; sólo se ensayan pruebas C++ independientes.
- NVRAM de MAME guarda bytes por dispositivo. Los estados transitorios de comandos NOR y los snapshots completos de MAME aún requieren revisión.
- No se usaron T/Commander ni Ghidra ni el ROM original privado.

## CI ejecutado y casos complementarios
- Primer pase de R2 `609f9f1`: fallo bajo `-Werror` por variable sin uso, corregido sin cambios de comportamiento en `1f3c554`.
- Ejecución `43620758227` en `1f3c55436c424ac6e375a7094b4a3c780173d429`: **PASS** en GCC y Clang. https://github.com/SamDelorean/asneo-emu/actions/runs/43620758227
- Se agregaron pruebas nuevas de programación de dato 0x00F0 y borrado completo auténtico (secuencia de seis escrituras), pendientes de ejecutar en el siguiente workflow.
