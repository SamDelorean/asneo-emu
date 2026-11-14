# R5 — UART MC68VZ328 compartida con Dana (2026-11-14)

## Hallazgo R4 y corrección
La SmallROM de NEO accede a UART `0xFFFFF906/07` y no llega al rango del System 3 durante 24 s simulados. El driver anterior rechazaba ese periférico. Se reutilizan sólo las clases serial PHY y VZ UART de `SamDelorean/DANA-emu/src/mame/skeleton/dana_vz_uart.h` con atribución BSD-3, sin el serial peer, IRDA externo, SD, USB, impresora ni GPIO de Dana.

## Integración
- UART1 `0xFFFFF900..0xFFFFF90B`, UART2 `0xFFFFF910..0xFFFFF91D`; reenvío `vz_r/vz_w` de palabras y máscaras de 16 bits, con semántica RX FIFO original.
- IRQ1 bit `0x0004` y UART2 bit `0x1000` en `neo::vz_core` preexistente. La prioridad/ILCR es la original del VZ.
- SYSCLK interno actualizado en reset y tras PLL; sin reloj externo ni línea serial de otro equipo inventado.
- Máquina/configuración/ejecutable únicos: `neo-diag`, con `asneob` y `asneow` como ROM sets. Imágenes privadas sin modificar.
- Adaptación original `neo_vz_uart.h` conserva FIFO, framing, parity, stop bits, save-state, timers del PHY y status.

## Validación requerida
Compilar MAME incremental en T; auditar dos ROM; ejecutar trazas PC sobre BOOT y WRITER y comparar con R4. Código incorporado **no es declaración de PASS** hasta que existan resultados nativos. Sólo declarar System 3 / AlphaWord si se observa el PC / LCD correspondiente.
