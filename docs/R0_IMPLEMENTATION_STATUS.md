# R0 — Código incorporado, contrato de pruebas y límites

Fecha: 2026-11-14. Esta etapa consiste en **incorporar la base reutilizable**; no se ejecutó Commander/T/Ghidra ni se lanzó MAME. Se compiló un test C++ independiente en un contenedor local, no en Surface ni T.

| Código | Fuente/alcance | Estado |
|---|---|---|
| `alphasneo.cpp` | Port de CPU VZ 32bit desde Dana (NO CPU EZ); MAME driver adaptado | ESCRITO; **sin compilar/ejecutar** |
| `neo_firmware.h` | Segmentos OS3K y SmallROM; base `neo-re` | ESCRITO; **LOCAL-CXX-PASS** con prueba sintética, no MAME |
| `neo_keyboard.h` | Matriz 16×8 original NEO; `neo-re` | ESCRITO; pruebas sintéticas disponibles |
| `neo_lcd.h` | Controladores LCD izquierdo/derecho; `neo-re` | ESCRITO; pruebas sintéticas disponibles |
| `tests/neo_components.cpp` | Unit tests offline sin ROMs | **LOCAL-CXX-PASS**, compilación directa, ejecución exit 0 |
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

## Evidencia de prueba efectuada (sin MAME)
- Fecha: 2026-11-14
- Compilador: `g++ (Debian 14.2.0-19) 14.2.0`
- Comando: `g++ -std=c++17 -Wall -Wextra -pedantic -Isrc/mame/skeleton tests/neo_components.cpp -o tests/neo_components && tests/neo_components`
- Resultado: **exit_code=0**, sin diagnóstico de compilador.
- Comprobación de identidad: se confrontaron los **SHA de blobs Git** de los archivos locales probados contra las respuestas GitHub sobre la rama `main`:
  - `neo_lcd.h`: `d05b5d84541e2b8c9837350fa3867ca4609abe48` = GitHub.
  - `neo_keyboard.h`: `a7d0535f6d3f97af2436ca463c89410fdd0ff156` = GitHub.
  - `neo_firmware.h`: `281ce7c9dd44d55ecb360be0ebc21de33c7cd146` = GitHub.
  - `tests/neo_components.cpp`: `286684abdf53fa1453ce12cf630ce6151ff54e0a` = GitHub.
- Alcance: GPIO/key matrix, LCD, segmentos de firmware con **datos sintéticos**. **No se tocaron ROMs privadas ni se probó hardware físico**. MAME compilation / native boot = **PENDIENTE**, no PASS.
