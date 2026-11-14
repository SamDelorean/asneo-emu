# Identidad única del emulador NEO

Fecha 2026-11-14. Instrucción explícita del usuario: **una sola versión operativa llamada `neo-diag`**.

- Repositorio de fuente único: `SamDelorean/asneo-emu` (nombre histórico; NO renombrar repositorio).
- Árbol local de integración y compilación canónico en T: `/home/spc/Projects/alphasmart/neo-diag` (si se crea, usar sólo éste).
- Ejecutable operativo final único: `neo-diag`, no `asneo-diag` ni nombres temporales instalados.
- `asneo` se conserva como identificador interno del driver/ROM del sistema MAME, distinto del nombre del binario.
- Reutilizar fuente MAME de AS3K/Dana sólo como dependencia de origen; **no** modificar sus árboles ni sus ejecutables.
- Despacho vía cola aislada `googledrive:AS2K_TEMP/NEO_DIAG_PIPELINE` con `DCJOB_REMOTE`; se prohíbe lanzar el trabajo AS3K ya pendiente en la cola compartida.
- Compilación nativa, arranque de firmware y GUI sólo se declaran PASS cuando existan compilación, ejecución y evidencia correspondientes. Checks C++ aislados no equivalen a arranque.
