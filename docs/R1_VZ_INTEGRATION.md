# R1 — Integración VZ desde DANA-emu (14-nov-2026)

**Commit de código; NO es boot PASS ni compilación MAME.** No se utilizaron Commander, T, Ghidra ni ROMs privadas.

## Código integrado
- \`neo_vz_core.h\`: adaptación directa de ecuaciones H2 / estado de registros de **DANA-emu** (licencia BSD-3-Clause) para MC68VZ328. Incluye PLLCR/PLLFSR, divisor SYSCLK/LCDCLK, reloj cristal 32768, PCTL registrado, interrupt mask/pending/IRQ routing, IVR/IACK, Timer1/2 (TCTL, TPRER, TCMP, TCN, TSTAT y lectura-antes-de-borrado), RTC/alarma/día/stopwatch/RTI/watchdog, no-ops controlados en registros RO, sin periféricos de placa Dana.
- \`alphasneo.cpp\`: conexiones de los registros a MMIO NEO, reloj CLK32 de MAME, timers virtuales de comparación, CPU IRQ líneas 1–6, CPU-space autovector IACK, reloj CPU ajustable a PLL.
- \`tests/neo_vz_core.cpp\`: pruebas nativas portables sin firmware privado para coherencia del PLL, enrutamiento IRQ, timers y RTC.

## Origen específico
\`https://github.com/SamDelorean/DANA-emu/blob/main/src/mame/skeleton/alphasdana_h2.h\`
y \`alphasdana.cpp\` del repo principal. Fórmulas y máscaras conservadas en la adaptación. NO se copian las fuentes de controlador USB ISP1161, pen, ST keyboard, SD ni el LCD de Dana. Las diferencias exactas de reset de NEO están pendientes de contraste físico/firmware.

## Límites y observaciones
- Los valores de reset \`PLLCR=0x24b3\` / \`PLLFSR=0x0347\` vienen de Dana; deben acreditarse para NEO. No se infiere 33MHz exactos en NEO por su clock fijo anterior.
- Retiming PLL / timers es candidato a prueba en MAME. PCTL se registra, pero el modelo de PCM/doze y sleep/wake diferido de Dana **todavía no está conectado** al CPU NEO.
- La ocurrencia de watchdog está registrada como solicitud, **no** hace un reset externo no acreditado.
- Las lecturas de PDDR/f449 y matriz siguen siendo la aproximación de NEO basada en \`neo-re\`; adaptar únicamente con evidencia de firmware/placa.
- Las señales de IRQ específicas de SPI1, USB y teclas power están sin conexión NEO, aunque el IRQ controller común ya admite las rutas VZ.
- NOR CFI, estado persistente, USB, IR/impresión, NEO2 no implementados.
- No se han probado compilación MAME ni arranque System3/SmallROM; se mantiene \`MACHINE_NOT_WORKING\`.

## Pruebas independientes
\`\`\`sh
g++ -std=c++17 -Wall -Wextra -pedantic -Isrc/mame/skeleton \
    tests/neo_vz_core.cpp -o /tmp/neo_vz_core && /tmp/neo_vz_core
\`\`\`
La prueba verifica sólo las invariantes señaladas. El siguiente paso auténtico es ejecutar código de firmware con MMIO capturado en MAME autorizado y resolver diferencias de dirección/timing observadas.
