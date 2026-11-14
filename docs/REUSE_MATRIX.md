# Matriz de reutilización comprobada — NEO MAME R0 (2026-11-14)

## Criterio general
No se implementa hardware sin **fuente existente, evidencia del NEO y prueba dirigida**. El destino es `SamDelorean/asneo-emu` (MAME, un único driver y un binario `neo-diag`). Esta matriz es documental: **no equivale a pruebas compiladas/ejecutadas en T**. Los nombres de fuentes y valores deben verificarse nuevamente contra los commits exactos al importarlos.

| Subsistema | Fuente previa prioritaria | Estado de partida observado | Diferencia clave / acción mínima | Validación obligatoria |
|---|---|---|---|---|
| CPU M68k/DragonBall VZ | `SamDelorean/DANA-emu/src/mame/skeleton/alphasdana.cpp`, `alphasdana_h2.h`; MAME `src/devices/machine/mc68328.*` | Dana dispone de M68000 con 32-bit addr MMIO, PLL, CLK32, timers, GPIO, IRQ, RTC, UART; ejecutó ROM Dana auténtica según bitácoras | Extraer/generalizar **sin arrastrar** dispositivo Dana ni su estado específico. NEO es VZ, AS3K es EZ | Reset vectors y registros de firmware NEO; trazas CPU/MMIO; validación contra datasheet MC68VZ328 |
| MAME driver e infraestructura de build | `SamDelorean/mame-as3k/src/mame/skeleton/alphasma3k.cpp`; `isotherm/mame` rama `alphasmart`; MAME oficial | MAME oficial contiene skeleton de AS3000; isotherm contiene cargador de applets, RAM/NVRAM, teclado, Flash | Reutilizar patrones de dispositivo e integración, **no** reutilizar el mapa EZ y LCD 40×4 | Compilación mínima NEO sin afectar AS3K/Dana |
| Firmware/arranque | `ioma8/neo-re/alpha-emu` y Ghidra existente | SmallROM, main OS y registro de entrada documentados; binarios oficiales `.os3kos` en Drive | Reconstruir verdadero boot de CPU y transición SmallROM/OS sin fabricar PC/stack/pantallas | Trace PC/SSP, MMIO, paso real por SmallROM, menú del SO y applets |
| RAM, Flash, estado persistente | `DANA-emu`, `ioma8/neo-re/alpha-emu/src/memory.rs` | `neo-re` documenta reset mirror, Flash, RAM baja, applet storage | NEO Flash Group A, RAM Group B, LCD Group C; revisar modelo físico, escritura/CFI y layout instalable. `.os3kos` no es dump completo | Hashes, reset repetible, ciclo recuperación; datos persistentes sin corrupción |
| LCD | `ioma8/neo-re/alpha-emu/src/lcd.rs` + investigación Ghidra/NEO | Modelo de dos controladores; framebuffer 320×128 y ventana visible 264×64; MMIO `0x01000000` y `0x01008000` | Crear dispositivo MAME mínimo según comandos realmente usados; validar identificación ST7675 contra datasheet/placa | Escritura de píxeles por CPU, frame capturado, cursor/menu reales |
| Teclado | `ioma8/neo-re/alpha-emu/src/keyboard.rs`; workpaper NEO 2013 | Matriz 16×8; Port D entradas activas-bajas; filas en GPIO con PB5 como fila 9 | Adaptar conectividad NEO, no ST-keyboard de Dana ni el teclado AS3K | Barrido real de filas/columnas, varias teclas, boot chords |
| SmartApplets y trampas | `SamDelorean/betawise`, `ioma8/neo-re`, fork `isotherm/mame` | Formatos `c0ffeead/cafefeed`, herramientas de carga e investigación profunda ABI | Presentar SmartApplets como Flash auténtica; no usar shims permanentes que reemplacen el SO | Menú SmartApplets y lanzamiento CPU del original AlphaWord/Calculator/ControlPanel |
| USB/printer/IR | `DANA-emu`, `mame-as3k`, `ioma8/neo-re/real-check` | USB NEO real con modos 081e:bd04/081e:bd01; modelos previos para DMA/serial/USB | Primero trazar controlador externo concreto del NEO; no afirmar chip Dana = chip NEO | Enumeración/transferencias/capturas nativas; no simular respuestas sin base |
| SD / radio NEO2 | Expediente Drive NEO SD; Dana SD | NEO SPI1 activo, uso real comprobado; semántica no cerrada; PB5 confirmado para teclado | No reutilizar Dana SD sin prueba de netlist NEO; no asociar SPI1 automáticamente a SD ni radio | GPIO, pinout físico o evidencia firmware independiente |

## Anclas técnicas NEO (revalidar sobre bytes)
- `smallos3kneorom.os3kos`: 22554 bytes observados en Drive, base candidata `0x00400000`, PC reset `0x0040042A`. SHA256 documental: `6402c40878ba28747642ce474361b1596555b2a1f137db79c7ea8bb5b35a9523`.
- `os3kneorom.os3kos`: 395264 bytes observados en Drive, paquete main base `0x00410000`, entrada normal `0x00410082`, trap handler `0x00426768`. SHA256 documental: `32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0`.
- Cadenas de applets persistentes desde `0x00470000`, registros MMIO VZ `0xFFFFFxxx`, LCD izquierdo `0x01008000`, derecho `0x01000000`, ASIC `0x02000000` bajo estudio.
- Tamaños/offsets anteriores son del inventario y del emulador externo; validar diferencias 2005/2013 y actualizadores, no soldarlas como defaults físicos no comprobados.
- **NO** tomar notas históricas de SD/radio como conclusiones nuevas; el expediente maestro contiene rectificaciones explícitas.

## Bibliografía técnica y rutas existentes
### Repositorios
- https://github.com/SamDelorean/asneo-emu — destino único.
- https://github.com/SamDelorean/DANA-emu — Driver VZ/MAME y docs `GENERAL_WORK_PLAN.md`.
- https://github.com/SamDelorean/mame-as3k — infraestructura MAME.
- https://github.com/SamDelorean/AS2K-Emulator — metodología DIAG/boot, no CPU compatible.
- https://github.com/SamDelorean/ASPRO-emu — metodología DIAG/bootstrap, no hardware NEO compatible.
- https://github.com/SamDelorean/betawise — ABI OS3K de referencia.
- https://github.com/SamDelorean/Dana-OS3K — documentos/datasheets VZ y fuentes de terceros.
- https://github.com/isotherm/mame/tree/alphasmart — adaptación AS3K MAME con SmartApplets.
- https://github.com/mamedev/mame/blob/master/src/mame/skeleton/alphasma3k.cpp — skeleton oficial.
- https://github.com/ioma8/neo-re — referencia independiente NEO; `alpha-emu/src/{firmware,memory,lcd,keyboard}.rs`.
- https://github.com/lykahb/neotools — protocolos USB y formato de SmartApplets.

### Drive privado
- ROMs NEO: https://drive.google.com/drive/folders/1c_Km85zoBOjQBN0DDdeiQGqpcBHEu8rq
- Applets NEO: https://drive.google.com/drive/folders/1EQ-Bai1wl1tGUm8rrNI4WmES9iPCfkIT
- Ingeniería inversa NEO SD: https://drive.google.com/drive/folders/15ZSF-lRJ5KiaL4TKjE0cQcmWE0OabmrR
- Ghidra previo (`neomanager_2024_01_01.gar`): https://drive.google.com/drive/folders/16D-m5VcfmXpQ5IhMV5mhMIzblRIxi-xI
- Fuente clásica AS3000: https://drive.google.com/drive/folders/1wcuT3vrnlBBkevxKPJClTSOPyHQcT9p5
- Trabajo local ioma8 registrado previamente en T: `/home/spc/Projects/alphasmart/ioma8-neo-re` con modificaciones locales; **no reset/clean/pull ciego**.

## GATE R0 — criterio de entrega a Codex
1. Inventario actual con ruta, hash, commit, licencias, diferencias y qué puede copiarse legalmente; distinguir análisis externo de código transferido.
2. Único plano de memoria y hardware NEO con evidencia y clasificación de certeza.
3. Cadena de boot real y pruebas dirigidas ya disponibles (de `neo-re`, Ghidra previo y fuentes privadas).
4. Plan integral de adaptación de driver VZ; sólo crear código específico de NEO donde el contrato lo exige.
5. Ninguna prueba nativa reclamada PASS hasta corrida con exit code, trazas, binario y evidencia conservada. Sin lanzamiento remoto no autorizado.
