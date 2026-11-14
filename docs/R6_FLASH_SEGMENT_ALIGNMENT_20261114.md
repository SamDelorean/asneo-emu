# R6 — Corrección del empaquetado NEO OS3K y regresión de arranque

Fecha: 2026-11-14. Se conserva **un solo** emulador `neo-diag` y **dos** imágenes privadas BOOT/WRITER.

## Hallazgo de la comparación con firmware canónico
El parser anterior iniciaba el cursor del paquete System3 en `0x70` y concatenaba los segmentos según sus longitudes **lógicas**. Esto provocaba desplazamiento de `0x70` en todo el bloque principal y colocaba ceros en el segmento secundario de identidad `0x00406000`. En el original NEO13 el `0x70` inicial **forma parte de la transferencia** y los rangos secundarios están redondeados a bloques `0x400`.

La evidencia histórica privada del mapper de SmallROM NEO13 ya había calificado estos tres rangos: `file[0:0x60000]→flash[0x410000:0x470000]`, `file[0x60000:0x60400]→flash[0x406000:0x406400]`, `file[0x60400:0x60800]→flash[0x5ffc00:0x600000]`. Referencias: expediente de revisión SmallROM y `verify_neo_smallrom_segment_mapper_2026-09-08.py` (33/33 PASS), contenidos en Drive privado. La antigua traducción `data_offset=0x70` fue trasladada de `ioma8/neo-re` sin verificar contra los offsets **físicos**, lo cual produjo ROMs de prueba incorrectas a pesar de pasar hashes de procedencia.

## Invariantes de firmware verificadas (fuentes originales sin cambios)
- SmallROM reset SSP `0x0007FFF0`, PC `0x0040042A`.
- `0x00410082` ahora empieza `2E7C0007FFF0...` y alcanza el trampoline original de System3; antes `2F0B...` (desplazado).
- `0x00406000` contiene `System 3 Neo      \0N` como en el archivo en `+0x60000`; antes todo cero.
- Tráiler principal en `0x0044FF80` idéntico a `os3kneorom.os3kos[0x3FF80:0x40000]`; secundario en `0x005FFF80` idéntico al original `[0x60780:0x60800]`.
- AlphaWord Plus original se copia sin alteraciones en `0x00470000` para WRITER. BOOT queda con zona borrada.
- Nunca se equipara una imagen ensamblada a un dump físico, y **no** se fuerza el salto del PC a la entrada de System 3.

## Artefactos privados corregidos (candidatos R6)
| Imagen | Bytes | SHA-256 | CRC32 | SHA1 |
|---|---:|---|---|---|
| BOOT | 4194304 | `e0e962dfaf7cc8f914c17708c8f9da924e13e44fb0bf8d7fbe015bd8c5c87765` | `7321c03f` | `8b556e1dda4588b9380fbf895ece11c52c7f18f8` |
| WRITER | 4194304 | `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626` | `7099ffa9` | `334b867183acf6f537823ddd4aa6743fe3d2dd8e` |

El constructor reproducible, el manifiesto de hashes y ambas imágenes **están solamente en private**, nunca en GitHub. Mantener las imágenes R3 anteriores como evidencia y restauración; no borrarlas.

## Estado de calificación
- Construcción independiente, comparación byte a byte y validación estática de tabla y contenido **PASS** en entorno de trabajo.
- `neo_firmware.h` corrige cursor físico y longitudes de transferencia, y `tests/neo_components.cpp` contiene prueba sintética para detectar regresión. Los `ROM_LOAD` actualizan los CRC/SHA1 de BOOT y WRITER.
- La compilación nativa y el arranque real en MAME requieren evidencias de T con NVRAM **aislada**; no declarar `SYSTEM3_BOOT_PASS` ni `ALPHAWORD_VISIBLE` antes de ellas.
- La condición de salida de SmallROM puede ser una **selección de modo actualizador** y no un fallo de CPU. El ROM original muestra mensajes “Small ROM Updater” / “Waiting for USB or AlphaHub connection”. El System 3 se analiza con sus verdaderas entradas y metadatos, sin parchear vectores.

## Criterios de aceptación en T
1. Solo instalar fuente en el árbol `/home/spc/Projects/alphasmart/neo-diag`.
2. Verificar SHA de dos ROM candidatas, pruebas sintéticas y compilación MAME incremental.
3. Validar ROM sets y trazas de 24s con NVRAM aislada y capturas LCD; comparar PC contra R5.
4. Mantener las ROMs R3 como archivadas y sustituir únicamente las dos imágenes BOOT/WRITER **después** de la validación.
5. Registrar `PASS` de integridad, compilación y ejecución por separado de `BOOT_PASS`.
