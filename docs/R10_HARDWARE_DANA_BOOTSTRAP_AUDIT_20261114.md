# R10 — Auditoría de hardware NEO y bootstrap MC68VZ328 de Dana
Fecha: 2026-11-14. Fuente NEO: SamDelorean/asneo-emu, rama main; Dana: SamDelorean/DANA-emu, rama main. Auditoría estática y de evidencias anteriores. No recompilación, ni acceso a medios privados del usuario, ni cambio de ROM/driver.

## Estado por bloques
| Bloque | Estado que acredita el código | Faltante / límite |
| --- | --- | --- |
| CPU y memoria | M68000 VZ, SRAM 512 KiB, NOR virtual 4 MiB, reset SmallROM, System 3 ejecuta, AlphaWord alcanzado por PC | Chip selects/bancos/mirror de reset en hardware físico no calificados; mapa fijo virtual |
| Flash | ST M29W320EB perfil optativo, CFI QRY real y geometría validada R9, escritura/borrado de palabra parcial | Perfil por defecto es desconocido/sólo lectura; x8 program/erase, protección, temporización y chip real del usuario no calificados |
| SRAM/NVRAM | Dos nvram_device para RAM y NOR; sistema recupera estado inicial con Y y Enter | Texto guardado tras reinicio NO validado; no hay save_item para estado local VZ, teclado, LCD, NOR ni GPIO |
| Teclado | Matriz 16×8 y selectores GPIO; Y y Enter probados | Texto T/E/S/T no visible en R9; rastrear lectura F419, selección de filas, entrega IRQ, STOP y los registros de archivo |
| LCD | Dos controladores externos, comandos básicos, página/datos, vista 264×64 y diálogo de batería reales | No se ha validado protocolo completo, cursor/blink y control físico; no afirmar fallo por 32 píxeles |
| Clock, timer, RTC, IRQ | PLL, SYSCLK, timers 1/2, RTC e IRQ comunes con Dana, pruebas de núcleo | Periféricos eléctricos y GPIO/IRQ externos, PLL sleep/wake y resets reales no completos; watchdog sólo registra solicitud sin aplicar reset |
| GPIO | Filas de teclado F408/F410/F440 y lectura F419, F449 con bit ready forzado | Portas y multiplexación adicionales/IRQ por borde/straps sin calificar; otros MMIO desconocidos devuelven FF |
| UART1/2 | Registros, FIFOs, timing, IRQ y relojes internos adaptados de Dana | output/RTS sin peer (set_nop), cable RX/TX/CTS, SIR externo y prueba bidireccional NEO no implementados |
| ASIC NEO | Documentada región 02000000..02000007 en neo-re | Sin mapear en driver; necesidad exacta requiere traza de CPU |
| USB/IR/impresión | No hay dispositivo externo enlazado al driver NEO | HID/direct USB, USB host, impresora, óptica IrDA, conectores y transferencias no implementados |
| SPI1/2, PWM, sonido, CS/DRAM | Silicio MC68VZ328 incluye estos bloques, pero el driver no implementa su conjunto completo; MACHINE_NO_SOUND_HW | Determinar conectividad PCB antes de copiar periféricos Dana |
| Bootstrap silicon VZ | Ausente en NEO | Sin EMUBRK, B-record Quickload, IBUFF ni vectores internos de bootstrap |

### Hallazgo prioritario R9
El job R9_CFI_NATIVE terminó PASS: QRY=C51/52/59, límites de applets 00470000–00800000, slot1=00470000, PC ejecuta AlphaWord en 00472C28. La traza R9_WRITER_UI_V2 después de Y→Enter→TEST registró 32 píxeles en LCD, sin cambio aparente de texto, y PC 00426758.

El emulador de referencia ioma8/neo-re documenta precisamente 32 píxeles en el cursor de un editor AlphaWord vacío (dos columnas × dieciséis filas), y la instrucción STOP en 00426752 con despertar por IRQ. Por tanto, el resultado actual es compatible con un editor vacío esperando entradas: no demuestra fallo de LCD ni obliga a agregar Font/Calculator/ControlPanel. Prioridad P0: comprobar posición de los 32 píxeles, IRQ pendiente/máscara, despertar de STOP, los accesos reales a F419 y filas GPIO durante T/E/S/T, y estado de ficheros AlphaWord (SRAM 0FDA/0FDE y 35E2/35E6/35EC). Después exigir texto visible y persistente en NVRAM aislada, sin sintéticos.

Fuentes: https://drive.google.com/file/d/13s_iYLA8gtdVexgL9paJ3hSiqQm8wSgJ/view
https://drive.google.com/file/d/1Lipjugf7W3z2zvjH_8KEJaikBjvwTIkc/view
https://github.com/ioma8/neo-re/blob/main/docs/2026-04-20-alpha-emu-memory-map.md

## Auditoría bootstrap Dana — estado verificado
En DANA-emu:
- vz328_bootstrap.h analiza B-records Motorola de dirección 32-bit, hasta 31 bytes, hexadecimal mayúsculo + CR; ejecución con contador cero y rechazos explícitos.
- alphasdana.cpp integra selector EMUBRK normal alto / bootstrap bajo, vectores internos SSP FFFCFFFC / PC FFFFFF00, IBUFF de 32 bytes en 00FFFFC0..00FFFFDF, y transporte de archivo .b mediante Quickload. Escribe los bytes por el bus AS_PROGRAM y ejecuta 68k descargado. Acepta ROM instalada sin alterarla.
- Prueba nativa de IBUFF NOP + BRA.S -2 con y sin ROM y regresión al modo normal: PASS documentado.
- NO equivale al cargador completo en silicio: faltan UART 19200 8N1 a nivel de bit, elección de UART por primer byte, sync/ACK @, eco, retorno automático de IBUFF a máscara, y calificación de pines RESET/EMUBRK del PCB. La ejecución tras B-record se activa mediante set_state_int(M68K_PC), no mediante ROM de silicio ejecutada ciclo a ciclo.
- No confundir bootstrap EMUBRK del MC68VZ328 con SmallROM/Updater NEO. Su ausencia no causa por sí sola que AlphaWord no acepte letras.

Referencias:
https://github.com/SamDelorean/DANA-emu/blob/main/src/mame/skeleton/vz328_bootstrap.h
https://github.com/SamDelorean/DANA-emu/blob/main/src/mame/skeleton/alphasdana.cpp
https://github.com/SamDelorean/DANA-emu/blob/main/docs/VZ328_BOOTSTRAP_NATIVE_PASS_20261026.md

## Integración eventual a NEO (no ejecutada)
1. Reutilizar el parser original BSD-3 y sus tests en el único neo-diag; no tocar árbol/emulador Dana.
2. Selector EMUBRK optativo antes de reset, dejando normal SmallROM como predeterminado; no renombrar imágenes ni crear otro ejecutable.
3. Vista de memoria bootstrap separada con vectores internos, IBUFF, stub de espera y Quickload B; todos los datos por el bus NEO sin duplicar RAM ni alterar NOR.
4. Validación fail-closed con casos B erróneos, ejecución de payload IBUFF, ROM BOOT/WRITER normales sin bootstrap, ROM presente + B válido, restauración de reset. No acreditar UART serial físico sin transporte real.
5. Mantener HW opcional y fuera de la corrección P0 del editor; no inventar GPIO físicos/USB ni programar Flash por B hasta que lo pruebe un escenario real.

## Priorización
- P0: editor AlphaWord — STOP, teclado, IRQ, archivo actual y persistencia de texto.
- P1: exactitud del GPIO/ASIC/MMIO relevante, watchdog/reset/energía, NOR/flash y save-state.
- P2: bootstrap VZ lógico reutilizado de Dana (optativo), canales externos UART/IrDA/USB, chip selects/DRAM/SPI/PWM y hardware sin evidencia de uso NEO.

Conclusión: el emulador de trabajo NEO no tiene una implementación integral del hardware de placa. La UART interna y la consulta NOR CFI sí existen; no son pendientes globales. El bootstrap Dana está integrado y probado como Quickload + IBUFF/EMUBRK lógico, pero **no** como UART bootloader bit a bit. Esta auditoría no cambia código, pruebas ni estados de instalación.