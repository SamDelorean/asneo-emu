> **REGISTRO HISTÓRICO SUPERADO POR R6:** las imágenes R3 descritas abajo estaban mal ensambladas (cursor inicial +0x70 y bloques secundarios sin alineación). **No utilizarlas como ROM activa.** Las dos imágenes corregidas y los SHA vigentes están en [R6_FLASH_SEGMENT_ALIGNMENT_20261114.md](R6_FLASH_SEGMENT_ALIGNMENT_20261114.md). Se archivan R3 únicamente como evidencia de fallo reproducible.

# NEO R3 — ROM mínima de arranque (14-nov-2026)

## Producto real
**`OS3K_NEO_MIN_BOOT.bin` — 4,194,304 bytes**. Construido y verificado usando los paquetes privados originales, NO con instrucciones sintéticas ni con el firmware del AS3000.

- SHA-256: `d9ca602b1253e727e39f53ab113e00c4f87a5835a0843845622ca50dea3cf4bc`
- Ubicación principal T (copia verificada por SHA): `/home/spc/Projects/alphasmart/private/roms/neo/os3k_minimal/OS3K_NEO_MIN_BOOT.bin`
- Drive: `ALPHASMART/private/neo/os3k_minimal/OS3K_NEO_MIN_BOOT.bin`, [carpeta privada](https://drive.google.com/drive/folders/1449pEFS001vYmoSZutxB9ER1iV7q3kgn)
- También están el código fuente de ensamblaje reproducible `build_neo_min_boot.cpp` y `OS3K_NEO_MIN_BOOT.manifest.txt` (privados).
- Evidencia de copia verificable en T: [NEO_MIN_BOOT_T640_20261114.status.txt](https://drive.google.com/file/d/17dC6MzXD7dszxXSldWFwpmO-oaJ87bwE/view) = `STATUS=PASS`, 4MiB, hash correcto.

## Procedencia auténtica, sin ROMs en Git
- SmallROM NEO 2013, `smallos3kneorom.os3kos`: 22,554 bytes; SHA-256 `6402c40878ba28747642ce474361b1596555b2a1f137db79c7ea8bb5b35a9523`.
- System 3 NEO 2013, `os3kneorom.os3kos`: 395,264 bytes; SHA-256 `32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0`.
- Origen: `ALPHASMART/private/firmware/neo` (Google Drive); bytes de entrada comprobados contra los hashes rectores de `AGENTS.md`.
- Constructor: C++17 reutiliza `neo_firmware.h` del mismo emulador; no existe un segundo parser de firmware dentro del driver.
- Analogía con el AS3K: variante BOOT sin instalar AlphaWord Plus ni SmartApplets; Flash no vaciada ni reinstalada en un equipo físico.

## Mapa exacto de ROM virtual
| Direcciones físicas | Bytes | Procedencia |
|---|---:|---|
| `0x00400000..0x00405819` | `0x581A` | SmallROM original y vectores de reset |
| `0x00406000..0x00406013` | `0x14` | Segmento 2 original OS3K NEO |
| `0x00410000..0x0046FFFF` | `0x60000` | Segmento principal System 3 Neo |
| `0x005FFC00..0x005FFFFF` | `0x400` | Segmento 3 de datos/tráiler System 3 |
| Otras celdas `0x00400000..0x007FFFFF` | Complemento | `0xFF`, estado borrado de **medio NOR virtual nuevo** |

Vectores originales `SSP=0x0007FFF0` y `PC=0x0040042A`; SmartApplet storage comienza en `0x00470000` y permanece sin applets en esta variante.

## Pruebas efectuadas
- C++17, GCC14 `-Wall -Wextra -Werror -pedantic`: **compilación de constructor PASS**.
- Ejecución del cargador real `neo::firmware::install_small` y `install_system` con ambos archivos originales: **PASS**.
- Prueba Python independiente: vectores, tres segmentos comparados byte por byte, límites, tamaño exacto y todas las celdas complementarias `0xFF`: **PASS**.
- Verificación SHA-256 y tamaño al copiar desde Drive a la carpeta `private/roms/neo` de T: **PASS**.

## Límites importantes
- Este archivo es una **imagen virtual ensamblada** para pruebas; no es un dump de memoria Flash física ni demuestra su estado original.
- El driver MAME NEO publicado hasta hoy carga **SmallROM y System como dos paquetes originales** y los instala en Flash virtual; ya **tiene una selección nativa mediante los sets asneob y asneow** para cargar directamente el `OS3K_NEO_MIN_BOOT.bin`. La integración se validó en T640 con compilación, auditoría de ambas ROM y ejecución breve de proceso. El arranque de System 3 y la visibilidad de Writer siguen sin acreditar; ver [NEO_DIAG_ROM_VARIANTS.md](NEO_DIAG_ROM_VARIANTS.md).
- **Se ejecutó un smoke test MAME sin GUI con este archivo, pero NO hay BOOT PASS del System 3**; el trabajo de compilación `neo-diag` se sigue acreditando por separado.
- No publicar la imagen, paquetes originales ni SmartApplets en GitHub abierto.

## Ejecución reproducible
En entorno privado con el mismo `neo_firmware.h`:
```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
  -Isrc/mame/skeleton \
  build_neo_min_boot.cpp -o build_neo_min_boot
./build_neo_min_boot \
  smallos3kneorom.os3kos os3kneorom.os3kos \
  OS3K_NEO_MIN_BOOT.bin
sha256sum OS3K_NEO_MIN_BOOT.bin
```
El programa rechaza sobrescribir un destino existente, vectores distintos o una geometría de segmentos no reconocida.
