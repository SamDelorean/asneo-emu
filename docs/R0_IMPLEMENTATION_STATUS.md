# R0 — Código incorporado, contrato de pruebas y límites

Fecha: 2026-11-14. Esta etapa consiste en **incorporar la base reutilizable**; no se ejecutó Commander/T/Ghidra ni se lanzó MAME.

| Código | Fuente/alcance | Estado |
|---|---|---|
| `alphasneo.cpp` | Port de CPU VZ 32bit desde Dana (NO CPU EZ); MAME driver adaptado | ESCRITO; **sin compilar/ejecutar** |
| `neo_firmware.h` | Segmentos OS3K y SmallROM; base `neo-re` | ESCRITO; pruebas sintéticas disponibles |
| `neo_keyboard.h` | Matriz 16×8 original NEO; `neo-re` | ESCRITO; pruebas sintéticas disponibles |
| `neo_lcd.h` | Controladores LCD izquierdo/derecho; `neo-re` | ESCRITO; pruebas sintéticas disponibles |
| `tests/neo_components.cpp` | Unit tests offline sin ROMs | PREPARADOS; ejecutarlos antes de declarar PASS |
| H2 VZ completo | `DANA-emu/src/mame/skeleton/alphasdana_h2.h` | **NO IMPORTADO**; depende de adaptar state/clock/IRQ; prohibido copiar sin integración |
| NOR CFI física | Dana NOR y datasheet | **NO IMPORTADA**; sólo log de escrituras y lectura Flash; firmware podría fallar en reset/boot |
| USB / IR / NEO2 | Dana, AS3K, neo-re, Ghidra | PENDIENTE identificación de conectividad y driver físico |

## Dependencias técnicas abiertas
1. Completar H2 VZ reutilizando exactos contratos PLL/RTC/timer/IRQ de Dana, sin pervivencia de la placa Dana; anotar no-equivalencias.
2. Verificar reset-vectors reales en MAME y carga de todos los segmentos 2013/2005: comprobar encabezados en Ghidra. El loader sintético rechaza tablas sin terminador en el área 0x50–0x70 para no leer código como tabla.
3. Verificar validez de ROM_LOAD con hash vacío y modo de staging MAME en el build pinned; calcular hash oficial de ROMs privadas antes de acreditar verificación.
4. Integrar NOR/CFI y RAM persistente sin destruir trabajos NEO SD/NeoManager previos.
5. Compilar con MAME autorizado y registrar logs de CPU; **no** declarar PASS por compilación aislada de las cabeceras.
6. No invocar Commander sin intervención autorizada y respetar máximo 3 llamadas.
