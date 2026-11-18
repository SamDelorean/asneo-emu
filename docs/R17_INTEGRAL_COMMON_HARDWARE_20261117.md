# R17 — Integración única de hardware NEO/NEO2 sin RF
**Estado vigente: INSTALADO, 18 de noviembre de 2026.** El entregable es el emulador único `neo-diag`; no son ROMs nuevas ni un parche de OS3K.

## Binario instalado y evidencia de validación
- Ruta T640: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`.
- SHA-256: **`d06a96325657112354172fa82ac96c8fcdd8828e9beaf5286ea23562901eff35`**.
- Código fuente de hardware fijado en `766f23b2693c11a50f0853a70b8f02ba087678cc`.
- Fuente íntegra única: `https://github.com/SamDelorean/asneo-emu`.
- [Estado instalación PASS / END / EXIT_CODE=0](https://drive.google.com/file/d/1ueuDlSC2uJJk_v14OX4tw5Ovi2KyPv6e/view).
- [Resultado detallado](https://drive.google.com/file/d/16gpxvk46l_UnXT9dLS9cwOXCYqXzeAbB/view).
- [Hashes instalado/ROM](https://drive.google.com/file/d/1loy5n3gF5iNw5lsgMS_jzCkxR6Ku97Eq/view).
- [ROM BOOT/WRITER OK](https://drive.google.com/file/d/1V3oiowuL_Wuq6hl3nrG6j9oWR6P3oFLv/view).
- [Dispositivos MAME](https://drive.google.com/file/d/1NNSFT4Xx9RU30PYvuMLVCJrPPrI4yFBW/view): `uart0` y `uart1`, `serial_peer0/1`, `usbcomputer`, `usbprinter`, `pwm1speaker`.
- [Traza AlphaWord](https://drive.google.com/file/d/1g4ZIcQzFpuqtoLunGKZfKfL6yh9-aEfj/view) y [segunda ejecución con la misma NVRAM](https://drive.google.com/file/d/1q3SkeS4p6vzeBEhCPdUoicWU9jcAUKTD/view).
- [GCC/Clang siete pruebas de componentes PASS](https://github.com/SamDelorean/asneo-emu/actions/runs/44247418956).

## Inventario integrado en un único driver
| Bloque | Implementación R17 | Evidencia/limitación |
|---|---|---|
| MC68VZ328, RAM 512 KiB, NOR 4 MiB | CPU 68k con bus VZ, registros, NVRAM, BOOT/WRITER y CFI | Arranque y reinicio con NVRAM PASS; identidad física NOR por PCB no completa |
| Teclado + LCD 264×64 | Matriz completa 16×8, dos controladores de pantalla | AlphaWord/Print/teclado probados |
| SPI1 F700–F70B | FIFO TX/RX, control, reloj y IRQ VZ, adaptación del `dana_sd::controller` | C++ y enlace MAME PASS; entrada RF desconectada a nivel alto |
| SPI2 F800–F803 | Registros, finalización temporizada y estatus | C++ PASS; sin dispositivo externo supuesto |
| PWM1/PWM2 | Cinco muestras, periodos, salida sound MAME, contador PWM2 | MAME enlaza; audio procedente de firmware aún no comprobado |
| GPIO B/C/D/E/F/G/J/K/M | Registro DIR/DATA/PUEN/SEL, observación de cambios, teclado preservado | Falta correlacionar señales físicas de placa y batería |
| UART1/2 + IrDA | UART VZ y peers bitbanger/SIR reutilizados de Dana | Dispositivos presentes; transferencia IR real no acreditada |
| ISP1763 host + DC | SRAM host/device, PTD ATL, IRQ INT5, USB printer y computer conectados | Hardware base y pruebas de paquetes PASS; ver limitaciones |
| NEO + NEO2 | Selector de variante en el mismo ejecutable | La ruta real `IS_NEO_2` no está cableada/validada con firmware |
| RF / Wi-Fi de NEO2 | **Excluidos por alcance** | Sólo observación de tráfico SPI/GPIO, sin simulación de radio |

## Límites que impiden declarar todos los subsistemas totalmente funcionales
- **Impresora USB host:** R17 arrancó e invocó la tecla Print desde la ROM original. El archivo `.prn` tiene **0 bytes**. No es válido calificar como completo el periférico.
- **USB hacia PC:** el controlador y el peer explícito de paquetes están implementados, pero no existe prueba de enumeración o transferencia con software de PC/Manager real.
- **IrDA:** el UART2 y peer óptico existen, pero falta probar recepción y transmisión de archivos extremos a extremos.
- **Placa NEO2:** menú de perfil implementado, todavía no se ha identificado/probado la línea física que detecta la variante desde el firmware.
- **Energía y señalización externa:** PLL, RTC, timer y watchdog básicos presentes, pero estados completos de suspensión, batería y líneas auxiliares no están cualificados.
- **SPI/PWM:** comportamiento de registros/timer comprobado; cableado eléctrico y funciones de firmware no certificados.

**Estas son limitaciones de hardware/validación del emulador**, no tareas de parcheo OS3K. La radio no bloquea el objetivo según la instrucción expresa del usuario.

## Incidencia y cierre, sin recompilación redundante
1. Primer intento R17: 7/7 C++ PASS; no enlazó `speaker_sound_device` por dependencias incompletas del subtarget mínimo MAME.
2. Segundo intento R17: `REGENIE=1`, MAME enlazó, BOOT/WRITER ROM PASS; script erróneo esperaba un dispositivo `uart2`, pero los nombres reales MAME son `uart0` y `uart1`. La prueba se detuvo **antes de usar firmware**.
3. **Última ejecución `NEO_DIAG_R17_FINISH_INTEGRAL_20261118`:** comprobación corregida, reutilizó el candidato previamente enlazado **sin compilar de nuevo**, ejecutó firmware original, teclado Print, captura del estado SPI/PWM y reinicio con la misma NVRAM. **PASS**; instaló el único `neo-diag`, SHA indicado. El archivo PRN siguió con cero bytes y el resultado lo registra explícitamente.
4. Se preservó el R14 anterior hasta completar y verificar el reemplazo. Se usaron dos llamadas a Commander en esta intervención (lanzador y su confirmación), dentro de las tres autorizadas.

**No crear emuladores paralelos, reinterpretar la radio como requisito ni declarar COMPLETE antes de cerrar las transferencias externas.** La próxima tarea funcional debe actuar directamente sobre el hardware USB host/DC e IrDA con evidencia de la ROM y los periféricos; no repetir gates BOOT, teclado o compilación.
