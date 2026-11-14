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

## Validación nativa T640 — Acreditada
- Primer despacho `NEO_DIAG_R5_UART_20261114` (commit `06ceee2`): falló compilación GCC `-Werror=reorder` del constructor, no error del componente UART. Se corrigió la inicialización en `ff1bee347634ea1f1294441aa955289ec84de140`.
- Segundo despacho `NEO_DIAG_R5_UART_FIX1_20261114` (commit fijado `ff1bee3`) finalizó **PASS / EXIT_CODE=0 / END**. [Recibo](https://drive.google.com/file/d/1NazEePscCf0eHRIUPI5zah22hZioz1Tq/view).
- MAME 0.289 GCC 15.2: compilación nativa, archivo de biblioteca y **link**; auditoría ROM `asneob/asneow` **PASS**: [auditoría](https://drive.google.com/file/d/1st8MiC6vFZHmG8ApjuAnEoOC3TOnGUAH/view). Sin cambios a ROMs privadas.
- **Binario único instalado**: `/home/spc/Projects/alphasmart/neo-diag/neo-diag` SHA256 `98270ba2c257ccbac6a3ca52d7c6c0c6c991ab5fb536d5fc5ba7a4a96d1ca4d6`. La validación usó `neodiag` como salida temporal normal del compilador; instaló mediante movimiento al **único** ejecutable después de validar. El binario R4 previo era `667a0014fd5809b3ff669df3ec9522d5215caed63ed794fb629ea3a6543db788`.
- Ambas pruebas con NVRAM aislada y 24 segundos simulados reportaron `PROCESS_RC=0` y **1,442 muestras** de PC. BOOT y WRITER coincidieron en `235` PC distintos, `1400` cambios de PC, `0` muestras en `0x00410000–0x0046FFFF` (System 3) y `0` en AlphaWord.
- R4 previo con el mismo tiempo de prueba observó 115 PC distintos; R5 dio **235**. Cambio positivo en la trayectoria de SmallROM, asociado a la habilitación de UART, pero **sin paso acreditado a System 3**. Ya no repite la rutina de Timer/UART que dominaba R4; con UART ejecuta además `0x004009xx..0x004011xx` y `0x00404D6E` (según muestras).
- [Registro completo de salida, hashes y resúmenes](https://drive.google.com/file/d/19vJtv7zctLMjIO1LzR250f19OLnBNVzO/view); [traza BOOT](https://drive.google.com/file/d/1JMIJKgOEZU0-MADbvQ5LpTXaZtEaNYOk/view); [traza WRITER](https://drive.google.com/file/d/1OhDrt9CJtYi1GbGbJZzl0bU4bUyon7lX/view).
- **Clasificación**: `UART_INTEGRATION_NATIVE_PASS`, `DRIVER_BUILD_PASS`, `ROM_AUDIT_PASS`, `SMALLROM_EXECUTION_PASS`; `SYSTEM3_BOOT_NOT_ACREDITED`, `WRITER_VISIBLE_NOT_ACREDITED`.
- Próximo análisis acotado: establecer condición de salida de la rutina activa en SmallROM mediante xrefs/breakpoints concretos y registros MMIO; no forzar PC a `0x00410082`, no alterar ROM, no abrir subsistemas a ciegas.
