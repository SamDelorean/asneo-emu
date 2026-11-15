# R16 — USB host / impresión NEO: depuración dirigida y estado nativo
**Fecha:** 15 de noviembre de 2026. **Alcance:** única instancia `neo-diag`, firmware NEO original, reutilización de Dana sólo donde la semántica coincide. Se respetaron **10 de 10 llamadas** autorizadas a Commander. No se editaron las ROM privadas ni la NVRAM del usuario.

## Entrega y separación de estados

| Elemento | Estado comprobado |
|---|---|
| Ejecutable productivo único `/home/spc/Projects/alphasmart/neo-diag/neo-diag` | **R14 conservado** |
| SHA-256 R14 | `6a0298daa28c5937b7bb1202d7e2067b1a39198bdcc2f345a2bbd4b391cc7a05` |
| Fuente R16 | [commit `7e0b5f3d`](https://github.com/SamDelorean/asneo-emu/commit/7e0b5f3d90496fcaaf631e0e483be76e22414462), con INT5 (`5b451bd`) y PTD ZLP |
| CI gcc/clang | [PASS](https://github.com/SamDelorean/asneo-emu/actions/runs/43737204696) |
| Compilación completa MAME de candidato R16 | **PASS**, sin instalación |
| SHA-256 del último candidato T `neodiag` | `33a0d65b053ea55e2633c944ea624ffff17c173b4c215b6d041b8965ccc4594c` |
| Firmware AlphaWord, tecla Print | Ejecutado; regresa al editor sin la pantalla de excepción 806 |
| Salida real `.prn` | **0 bytes — impresión NO terminada** |
| Último trabajo | `NEO_DIAG_R16_ZLP_PRINT_20261115`: **PARTIAL / END_PARTIAL / exit 0**, `INSTALL=NO` |

## Ingeniería inversa original: error 806, no 886

La lectura inicial de pantalla como «886» fue incorrecta. La detención mediante breakpoint en la ROM WRITER real dio:

`PC=00425564 SP=0007FE2A RETURN=004508E6 FILE=004458E2 LINE=00000326 D0=00002711 A0=020000B2`.

`0x326=806`. En `0x004508CA–0x004508F0` el firmware repite una espera hasta más de 10000 iteraciones sin que el indicador de operación pendiente salga del estado 1, y llama al error `Usb1763.c:806`. Esta información procede de la ROM privada original; **no se distribuye la ROM**.

## INT5 — reutilización puntual de la propagación de Dana

Con `Print`, el firmware fijó el registro IMR VZ en `0x00EFFFDD`, habilitando la interrupción externa **INT5**, bit `0x00100000`. El ISP1763 tenía `HCINT=0x0150` y `HCINTEN=0x0100`, con un PTD que había marcado `DONE=0x0001`. En R16 se conectó `m_usb.controller_interrupt()` a `m_vz.set_pending(0x00100000U,...)`, propagada desde el timer USB y los accesos MMIO, con reconocimiento de eventos de hardware. La implementación reutiliza el patrón de interrupción de Dana, **sin copiar las IRQ2/IRQ3 de su PCB**. Código `8481c148`, regresión `5b451bd`, CI PASS.

## PTD ATL de longitud cero — diferencia descubierta en firmware NEO13

La traza de escrituras auténticas al HC SRAM mostró:

- Primer ATL PTD: `DW0=0x21000041`, `DW1=0x00000800`, `DW2=0x00018000`, `DW3=0x81800000`; el firmware envía 8 bytes de `SET_ADDRESS` y registra su finalización.
- Segundo ATL PTD: **`DW0=0x21000001`, `DW1=0x00000000`, `DW2=0x00000000`, `DW3=0x82000000`**, con `ATL_SKIP=0xFFFE` y `LAST=0x8000`.
- El motor R15 rechazaba cualquier PTD cuyo puntero de payload fuese menor a `0x1000`, aun con `total=0`. Se limitó esta condición a **transferencias con longitud positiva**, manteniendo todos los controles de límites y maxpacket en paquetes con datos; no se introduce ACK sintético ni se modifica la ROM.
- Prueba C++ reproduce explícitamente el segundo PTD con `DW2=0`, comprueba su finalización, mapa `DONE` y reconocimiento. [Commit fuente/prueba `7e0b5f3d`](https://github.com/SamDelorean/asneo-emu/commit/7e0b5f3d90496fcaaf631e0e483be76e22414462).

## Última prueba real en T: avance, pero sin impresión

Trabajo `NEO_DIAG_R16_ZLP_PRINT_20261115`: **PARTIAL**, preservó R14. Al pulsar Print en AlphaWord, después del ZLP:
- `PORTSC=0x1885`, `HCINTEN=0x0180`; el host ya ha avanzado en su inicialización.
- `ATL_SKIP=0x0001`, frente a `0xFFFE` antes de corregir ZLP. El firmware ya no enseña `Usb1763.c:806`.
- Capturas de frames 2300, 2600 y 4200 muestran el editor original con `test`. No se obtuvo archivo impreso: `PRN_BYTES=0`.
- No se sostiene que el controlador/impresora esté terminado, ni que exista enumeración con PC real. Puede ser otra divergencia de configuración, política de endpoint, interrupción o transferencia posterior; requiere evidencia adicional.

Evidencias:
- [Resultado final R16](https://drive.google.com/file/d/1ED-dCUbKRpd2HQ0_JFZzXt-lg648B-tN/view)
- [Traza final del firmware](https://drive.google.com/file/d/182ReOsWgr57PEVp5wfyxSjLVKUmD-vr7/view)
- [Hashes instalado/candidato](https://drive.google.com/file/d/1UycqQ69B22Y0gB9kZy4bIzStHTRkYj3F/view)
- [Error 806: retorno y argumentos capturados](https://drive.google.com/file/d/14BVw3L7ik5QWIkpymlx4cc4nZB4RJCZo/view)
- [Traza de PTD del firmware](https://drive.google.com/file/d/1pgBaF1fotSB-WHXOq5ZW58iy27uW8hNy/view)
- [Último estado del trabajo: PARTIAL](https://drive.google.com/file/d/1I13qMhPj8Wm-AnNLWZRJP4516yy_gh8N/view)

**Pendiente acotado para la siguiente intervención:** capturar la siguiente transacción de host/estado de salida tras superar ZLP, contrastar con la clase impresora reutilizada de Dana y lograr `PRN_BYTES>0` desde AlphaWord. No repetir R11–R15 ni restudiar el error 2671/806. **El entregable sigue siendo el único ejecutable `neo-diag`, no pruebas o variantes adicionales.**
