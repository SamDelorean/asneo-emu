# asneo-emu — AlphaSmart NEO / NEO2 MAME emulator

**Estado R0 (2026-11-14): código inicial publicado; pruebas C++ independientes PASS en contenedor local, compilación MAME y ejecución de firmware pendientes.** Este repositorio es el **único** destino de integración NEO. Nada de imágenes de pantalla simuladas, ROMs propietarias, tramas USB inventadas o emuladores paralelos.

## Código presente
- `src/mame/skeleton/alphasneo.cpp`: driver MAME inicial, bus M68000 de 32 bits adaptado de nuestro DANA-emu, apertura de RAM y Flash NEO, composición de paquetes SmallROM y System Neo, matriz física de teclado, dos controladores LCD y reporte explícito de registros sin implementar.
- `src/mame/skeleton/neo_firmware.h`: parser C++ de segmentos de actualizador System 3 Neo con validación completa antes de escribir en Flash y validación de vectores SmallROM.
- `src/mame/skeleton/neo_keyboard.h`: filas y columnas GPIO con tabla lógica derivada de la ingeniería inversa real del NEO.
- `src/mame/skeleton/neo_vz_core.h`: adaptación de reloj, Timer1/2, RTC e interrupciones VZ desde las rutinas calificadas de Dana; conexión MMIO y temporizadores MAME documentada en `docs/R1_VZ_INTEGRATION.md`.
- `src/mame/skeleton/neo_lcd.h`: dispositivo C++ gráfico 264×64 visible, dos controladores, comandos por páginas, lectura/escritura, scrolling y RMW.
- `tests/neo_components.cpp`: pruebas independientes sin firmware privado.

## Proveniencia y límites
CPU carrier: `SamDelorean/DANA-emu/src/mame/skeleton/alphasdana.cpp`, licencia BSD-3-Clause. Protocolo LCD/teclado y estructura de actualizador: `ioma8/neo-re/alpha-emu/src/{lcd,keyboard,firmware,memory}.rs`; comportamiento reimplementado en C++ sin copiar su código. Patrones de imagen MAME: `isotherm/mame`, rama `alphasmart`. Ver `docs/REUSE_MATRIX.md`, `docs/R0_IMPLEMENTATION_STATUS.md`, `AGENTS.md`.

**NO CALIFICADO:** Boot NEO MAME, integración completa del SoC VZ (PLL/RTC/IRQ/timers incorporados pero no comprobados en MAME), NOR CFI y persistencia, ASIC, teclado físico integral, USB/IR/impresión, arranque SmallROM y OS, interfaz NEO2 y compilación MAME. El driver conserva `MACHINE_NOT_WORKING`. El código *no* inventa una pantalla de bienvenida ni transforma OS3K en un programa host.

## Prueba independiente de componentes
```sh
c++ -std=c++17 -Wall -Wextra -pedantic -Isrc/mame/skeleton tests/neo_components.cpp -o /tmp/neo_components
/tmp/neo_components
```
**Resultado observado (14-nov-2026): PASS local**, compilado con g++ 14.2.0, exit 0; se verificaron las identidades Git SHA de las 3 cabeceras y el test contra los blobs publicados en GitHub. Sólo prueba lógica C++ autocontenida; **no verifica compilación ni arranque MAME**. Ver `docs/R0_IMPLEMENTATION_STATUS.md`.

## Integración MAME existente
Trabajar con el **MAME canónico autorizado**, incorporando este driver y cabeceras en `src/mame/skeleton`, sin crear un fork/tree/binario alternativo. Una vez esté autorizado el uso de la máquina compiladora, comprobar el target de MAME y compilar una sola variante `asneo-diag`; no afirmar que el comando es válido para una versión distinta sin verificar sus scripts de build. Copiar ROMs sólo al directorio privado esperado por MAME, jamás al Git público.

Las imágenes privadas `smallos3kneorom.os3kos` (22554 bytes) y `os3kneorom.os3kos` (395264 bytes) se ensamblan en la ventana Flash simulada según las tablas de segmentos del paquete, **no** se consideran dumps Flash físicos completos.
