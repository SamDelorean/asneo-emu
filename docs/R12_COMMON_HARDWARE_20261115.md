# R12 — Hardware común NEO/NEO2: integración de código y límite nativo
Fecha: 2026-11-15. Destino exclusivo: `neo-diag` en `/home/spc/Projects/alphasmart/neo-diag`.

## Continuidad obligatoria
El binario R11 verificado e instalado continúa vigente, SHA256 `eecd47ad87c6c55b4ea6a68a39e221c938ec9490d2c8c18d0782f8e3d42286cc`. El trabajo R11 acreditó 80 teclas y AlphaWord con `TEST` persistente. Esta intervención NO tocó ese ejecutable, ROM, NVRAM ni el árbol Dana.

## Modificaciones ya publicadas en main
- `neo_vz_uart.h`: `neo_serial_peer_device` por adaptación de `dana_vz_uart.h` (BSD-3), bitbanger independiente y transportes NRZ/SIR configurables; no se liga por defecto a ningún archivo ni red.
- `alphasneo.cpp`: cableado `UART1/2 TX→peer RX`, `peer TX→UART RX`, RTS/CTS; watchdog VZ ahora pide `schedule_hard_reset()`.
- `neo_isp1763.h`: núcleo preliminar de **ISP1763**, con identidad `0x00176320`, HC SRAM 24KiB, DC SRAM 4KiB, registros/interrupts HC y DC, selección de endpoint y buffers acotados. El USB no inventa periféricos conectados. **No contiene todavía** PTD/SIE completo ni transporte host/impresora/PC.
- `alphasneo.cpp`: ventana hardware `0x02000000..0x020000FF` conectada al núcleo, con acceso 16-bit. La ventana es distinta del ISP1161 de Dana (`0x1F000000..0007`).
- `tests/neo_isp1763.cpp` y `.github/workflows/component-ci.yml`: pruebas de ID, dirección de HC SRAM, endpoints, interrupciones y reset.
- HEAD publicado: `6afd33867ab4ceac9ec5b1fa4f092b3f75e09939`.
- [CI GCC y Clang: PASS](https://github.com/SamDelorean/asneo-emu/actions/runs/43690712769). Son pruebas de componentes C++ y **no** una compilación completa de MAME.

## Evidencia física y firmware
El NEO/NEO2 comparte MC68VZ328 y UART1/2, pero su conexión de pines difiere de Dana. El firmware NEO 2013 verifica ISP1763 mediante DC_CHIPID `0x00176320` y usa helpers MMIO en `0x02000000+offset`, especialmente 0x18, 0x70, 0x7C, 0xC4, 0xC6, 0xD4, 0xD6. En una revisión de placa NEO también se identificó ISP1161ABD. No debe copiarse el mapa/controlador ISP1161 de Dana por semejanza del uso USB ni afirmarse todavía una asignación de chip por variante NEO/NEO2.

Fuentes:
- [Bitácora inversa del firmware NEO13](https://docs.google.com/document/d/1UYFdXHzwffz2XfRaJ-LPKpVAYn7xyl2ODQC8Llhxylo/edit)
- [Contrato y ejecución ISP1161 en Dana](https://github.com/SamDelorean/DANA-emu/blob/main/docs/ISP1161_USB_INTEGRATION_20261030.md)
- [Matriz de adaptación NEO/Dana](REUSE_MATRIX.md)

## Estado real y pendiente para el mismo ejecutable
| Elemento | Estado |
|---|---|
| Componentes C++ incluidos USB | **PASS GCC/Clang** |
| UART↔peer SIR y watchdog en fuente | **IMPLEMENTADO EN CÓDIGO; NATIVO NO PROBADO** |
| MMIO ISP1763 preliminar | **IMPLEMENTADO EN CÓDIGO; NATIVO NO PROBADO** |
| Compilación y enlace MAME de R12 | **NO EJECUTADO** |
| Arranque y regresión AlphaWord con R12 | **NO EJECUTADO** |
| ISP1763 HC PTD, paquetes, impresora USB | **PENDIENTE DE IMPLEMENTAR** |
| ISP1763 DC control, USB Send HID, Manager, peer PC | **PENDIENTE DE IMPLEMENTAR** |
| IrDA bidireccional con firmware NEO | **PENDIENTE DE PROBAR** |
| Relojes/energía electrónica de placa, ASIC y arranque VZ | **PARCIAL / SIN CALIFICAR** |
| Radio RF exclusiva NEO2 | **FUERA DEL ALCANCE DE HARDWARE COMÚN** |

## Límite operativo
La preparación/compilación en T mediante Desktop Commander fue bloqueada por el cliente antes de llegar al equipo. No existe `R12 PASS`, no se lanzó trabajo a la cola NEO y el ejecutable R11 quedó intacto. No se repetirán gates R11 ni se abrirá otro emulador. La siguiente intervención nativa debe partir de estos archivos, proteger la instalación R11 y probar el propio binario R12, sin extrapolar CI de componentes a funcionalidad USB.
