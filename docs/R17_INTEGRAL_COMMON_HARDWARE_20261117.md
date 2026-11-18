# R17 — Integración única de hardware NEO/NEO2, sin radio
Fecha: 2026-11-17 (México), ejecución remota UTC 2026-11-18. **No se declara completo el emulador.**
Un único proyecto `neo-diag`; **NO** se crearon motores secundarios, forks o ROM alteradas.

## Versión real
- Producción aún instalada en T640: **R14**, `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA-256 `6a0298daa28c5937b7bb1202d7e2067b1a39198bdcc2f345a2bbd4b391cc7a05`.
- Fuente consolidada R17: commit **`766f23b2693c11a50f0853a70b8f02ba087678cc`**, repositorio `SamDelorean/asneo-emu`.
- Candidato MAME R17 **compilado y enlazado** en T640 como `/home/spc/Projects/alphasmart/neo-diag/neodiag`, **NO instalado ni ensayado con AlphaWord** debido a falso rechazo del script de inspección de periféricos.
- CI GCC + Clang para 7 componentes PASS: https://github.com/SamDelorean/asneo-emu/actions/runs/44247418956

## Hardware unido a alphasneo.cpp
| Subsistema | Implementación actual | Reutilización y límites |
|---|---|---|
| VZ SPI1 F700–F70B | FIFO TX/RX de 8, XCH/SPIEN, temporización de bit, IRQ en VZ bit21 | Adaptado de `dana_sd::controller` sin SD ni radio; MISO desconectado en alto |
| VZ SPI2 F800–F803 | Registro de datos/control, finalización temporizada y bandera de IRQ | Basado en comportamiento VZ ya usado en Dana; sin periférico externo supuesto |
| PWM1 F500–F505 | FIFO cinco muestras, control, temporización del período, nivel digital y dispositivo de sonido MAME | Adaptado de PWM de Dana, sin atribuir conexiones físicas no verificadas |
| PWM2 F510–F517 | Registros, período, anchura y contador | Motor básico de registro, falta calificar salida física |
| GPIO B/C/D/E/F/G/J/K/M | Lectura y escritura DIR/DATA/PUEN/SEL, desconexión de entradas por defecto | Se mantienen las señales específicas del teclado y se observan cambios auxiliares; falta identificar todo el cableado de placa |
| NEO y NEO2 | Selector de placa en un solo emulador | Radio/RF deliberadamente ausente; la selección no demuestra autoidentificación del firmware |
| VZ, RAM, NOR, LCD, teclado, UART1/2, IrDA, RTC, USB HC/DC | Núcleos heredados R11–R16 conservados | No equivalen a validación extremo a extremo de impresión, NEO Manager o IrDA |

## Validación de la ejecución integral R17
1. `000_neo_diag_r17_hardware_integral_20261117.job.sh`: **7/7 pruebas independientes PASS**. MAME código nuevo compiló pero falló **en el enlazador de sonido** `speaker_sound_device` porque el subtarget mínimo no había regenerado dependencias. Estado FAIL 25. [Log](https://drive.google.com/file/d/1F5JHoA6hI85ZX0DAFNnB-VhoPLDJOCex/view).
2. `000_neo_diag_r17_hardware_linkfix_20261117.job.sh`: el mismo código y mismo ejecutable destino, `make ... REGENIE=1`. **Compilación y enlace MAME PASS**, siete pruebas C++ PASS, **BOOT/WRITER ROM PASS**. [Evidencia ROM](https://drive.google.com/file/d/10J5ywjBoIliZ_D5MYmUi0arRQMDkGKf0/view).
3. La prueba de presencia MAME detuvo erróneamente el trabajo con FAIL 27: el script esperaba los tags `uart1` y `uart2`, pero el inventario auténtico [listdevices](https://drive.google.com/file/d/13MUNHdfO57wkgnnQny0lWMDYcEXWc7dS/view) incluye `uart0`, `uart1`, `usbcomputer`, `usbprinter`, `pwm1speaker`, `serial_peer0`, `serial_peer1`. Se verifican todos los dispositivos, sin hardware ausente por este error. **El test de AlphaWord no se ejecutó y R17 NO fue instalado.**
4. Intervención remota: **tres llamadas a Commander** (lanzamiento inicial, lectura de salida del lanzador, lanzamiento correctivo); se respeta el límite. No hay trabajo en segundo plano ni instalación pendiente autorizada.

## Qué sigue faltando para decir hardware completo
- Puertos USB: impresora con bytes reales de salida desde firmware, dispositivo hacia PC con enumeración y transferencias reales.
- UART2/IrDA: transferencia bidireccional con un peer real calificada.
- NOR: comportamiento físico de todas las revisiones y programación/borrado calificados.
- GPIO externos, selectores/batería, energía y estados de sueño/RTC: mapeo de placa y pruebas funcionales.
- SPI/PWM: prueban registro/temporalidad, pero no equipo real conectado ni todas las combinaciones del firmware.
- Identificación de placa NEO2 desde firmware: el selector MAME existe, sin RF ni puente de señal ya demostrado.

**No convertir CI, lista de dispositivos o compilación en afirmación de equipo terminado.** La prioridad en una intervención autorizada posterior sería corregir la validación errónea `uart2`, usar el candidato R17 ya enlazado sin recompilar, ejecutar la prueba funcional única y sustituir el binario `neo-diag` si no hay regresiones. Ningún trabajo adicional ha sido despachado.