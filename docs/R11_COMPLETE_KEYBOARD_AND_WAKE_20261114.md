# R11 — Reparación integral del teclado AlphaSmart NEO (2026-11-14)

El NEO 2013 tiene una **matriz de 16×8**, **80 posiciones ocupadas** y **48 libres**, documentada por el firmware en `0x0044C37B`. La correspondencia de códigos internos con USB HID se encuentra en `0x0043C32A`. El código previo ya tenía la matriz, pero omitía los códigos del anfitrión en 15 teclas y no generaba el evento hardware que despierta al CPU del `STOP` durante la edición. La reparación usa ambas tablas completas, no pruebas una tecla cada vez.

## Causa medida antes del cambio
En T, diagnóstico `NEO_DIAG_R11_KEYBOARD_BASELINE_20261114` usando el único binario anterior, firmware WRITER original y perfil NOR ST: Y/Enter funcionan durante la recuperación; después el PC queda en `0x00426758` con SR `0x2000` y no escribe `TEST`. El puerto `0xFFFFF419` **sí** recibe los bytes físicos de T `0xFE`, E `0xF7`, S `0xFB`. IMR=`0xFF8D`, IPR=`0x0020` (Timer2 pendiente), pero ILCR=`0x0000`, inválido como nivel de Timer2. La prueba esperó más de 60 segundos simulados y no hubo salida de STOP. [Registro](https://drive.google.com/file/d/1zBaErhkzipFHVJuiLlPJTnZx1m774Ai9/view).

## Corrección
1. `neo_keyboard.h` conserva la matriz completa original y añade el puente lógico HID de 81 entradas con `static_assert` para 80 celdas. La ruta SmallROM por `F411` queda habilitada sólo antes de que la ruta GPIO completa de System 3 controle las filas.
2. `alphasneo.cpp`: regeneración de los 16 puertos MAME/80 teclas desde las dos tablas originales. Cada celda ocupada tiene código anfitrión **único** y callback de cambio. Se asignan F1–F8 a File1–8; F9 Applets, F10 Send, F11 Spell Check, F12 Find, F13 Clear File, F14 Print y F15 a posición específica `0x2E` de HID desconocido; no se inventa su identidad física.
3. `alphasneo.cpp`: modelado de Port D en `F418–F41F` (dirección, datos, pullup, selector y control de interrupción), entrada de columnas `F419`, y wake de teclado activo bajo via **PDKBEN `F41E`** y IPR bit 6. Los INT[3:0] respetan selección de nivel, polaridad y flanco del VZ, reutilizando la semántica ya implantada en Dana; no se fuerza por software un IRQ cuando PDKBEN está deshabilitado.
4. `neo_vz_core.h`: restaurar `ILCR=0x6533` al reset como manda el MC68VZ328UM §9.6.6 y el modelo probado en Dana; antes se asignaba erróneamente `0`, eliminando el nivel IRQ de Timer2. Tras reset, Timer2 debe poder despertar de STOP a nivel 3 cuando el IMR lo habilita.
5. `tests/neo_components.cpp` recorre 16×8 con 80 válidas y 48 vacías, sin probarlas una a una en diferentes intervenciones; `tests/neo_vz_core.cpp` exige ILCR correcto y enrute de IRQ de Timer2.
6. Las ROMs de usuario permanecen intactas, sin bootstrap adicional, nuevos emuladores ni modificaciones de AlphaWord. El único `neo-diag` se instalará **sólo** si los tests de compilación, mapa completo y prueba real `Y→Enter→TEST` pasan.

**Estado de este commit: cambio de código; las comprobaciones nativas en T todavía deben ejecutarse.**

Fuentes: [tabla original de 80 teclas](https://github.com/ioma8/neo-re/blob/main/docs/2026-04-21-keyboard-matrix-map.md), [MC68VZ328UM §9.6.6/§10.4.5.8](https://www.nxp.com/docs/en/user-guide/MC68VZ328UM.pdf), [Dana PD Keyboard IRQ](https://github.com/SamDelorean/DANA-emu/blob/main/src/mame/skeleton/alphasdana.cpp).