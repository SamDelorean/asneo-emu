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

## Resultados nativos en T640 — cierre R6
**Compilación y medios corregidos: PASS**. Job `NEO_DIAG_R6_CORRECT_MEDIA_20261114`, `STATUS=PASS`, `STAGE=END`, `EXIT_CODE=0`, PIN `6de5b9f`. [Recibo Drive](https://drive.google.com/file/d/1XxqUWuopwZiowtGt3Z__8Qp13QY2j_1n/view); [logs, hashes y resumen](https://drive.google.com/file/d/1_tLksTjWFHdbvU-1CpGw6tB24-subh-u/view); [auditoría de ambos sets](https://drive.google.com/file/d/1I3TcMakmhJD1l2jUF_Qvk6BaPvlP5c0E/view).

- Pruebas sintéticas `neo_components`, `neo_vz_core`, `neo_nor` y link MAME 0.289: **PASS**.
- Un solo ejecutable instalado: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA256 `386635d9f9e9eef1d48662d6ea0834b03fa37445de175c306352d28ba4490534`.
- ROM BOOT SHA256 `e0e962dfaf7cc8f914c17708c8f9da924e13e44fb0bf8d7fbe015bd8c5c87765` y WRITER `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626`. Ambos 4MiB, exactamente **dos** opciones del mismo emulador.
- Auditoría MAME de BOOT y WRITER: **PASS**, tanto con ruta candidata como canónica.
- Traces 24s con NVRAM aislada: ambos entraron a System 3 desde el frame 10; `PC` en `0x00423xxx` y `0x004264xx`, 129 PCs distintos, 1,431 cambios. Contrasta con R5: 0 muestras System 3 en 24s con medio mal ensamblado.
- LCD **renderizado, no hipotético**: [BOOT frame1380](https://drive.google.com/file/d/1SgBoqNWa1zspUm-qVdY1jt-_cv6UnJyy/view), [WRITER frame1380](https://drive.google.com/file/d/1SEQGLogkC-QnNT-jA1DS9PeV4y0ywbSe/view). Mensaje: "An unexpected data change occurred. Did you recently remove or replace the AlphaSmart's lithium backup battery? (Y for yes, N for no)". Es inicialización de memoria de respaldo de una NVRAM de ensayo, no error de importación ROM.
- [Traza BOOT](https://drive.google.com/file/d/10N-l-btuh4r2WJtanlggss_gd6ObJbF5/view), [WRITER](https://drive.google.com/file/d/1ZGiIA2O_U7Irrh3dDZNTJ6cyn1AACHey/view) confirman los primeros opcodes originales de las entradas y metadata de los dos tráileres.
- **Drive canónico corregido in-place** preservando los mismos IDs de archivo; se materializaron las dos imágenes y verificaron de nuevo los SHA256 y comparación exacta con los candidatos. Las versiones R3 se conservaron en [archivo privado](https://drive.google.com/drive/folders/1nwmWYqhGp8b_16qvRBLBT5Ez36AjrGrc) y en T, nunca en GitHub.
- El código fuente reproducible y el manifiesto R6 están en `ALPHASMART/private/neo/os3k_minimal`.

### Prueba de teclado Y real (NVRAM sólo de ensayo)
Job `NEO_DIAG_R6_KEYINIT_20261114`, **PASS / EXIT_CODE=0** ([recibo](https://drive.google.com/file/d/14RbQa5cWqc4kPIx-Xb-oqKD6FrXmR4R9/view)).
El input MAME `Y [0x0f]` en ROW15 pasó `FF→FE→FF` en ambos sets. Los píxeles activos del LCD pasaron 1763→1543 y la pantalla cambió a:

"Your AlphaSmart needs to be restarted. You should not see the previous message again. Press the enter key to restart..."

Captura real: [BOOT tras Y](https://drive.google.com/file/d/1E-aBhAE2Ay6UbmGfmZbyHL0f8PhmT-Nc/view). [Registro BOOT](https://drive.google.com/file/d/14nbdlnNMpc0Xe6iJy-2hKgiHkxphaPWq/view).

El teclado Y y el flujo de UI System3 están **verificados**; **aún no se ha pulsado Enter**, ni acreditado que AlphaWord haya aparecido como editor. Siguiente comprobación dirigida: Enter y reinicio sobre NVRAM de pruebas, conservando los dos medios BOOT/WRITER y el único ejecutable. NO forzar PC ni tocar Flash original.

### Secuencia Y → Enter (validación dirigida posterior)
- Se comprobó la ubicación del input físico MAME: `Y=ROW15/bit0x01` y `Enter=ROW9/bit0x40`, código matricial `0x69`.
- El primer intento `NEO_DIAG_R6_Y_ENTER_20261114` probó por error `ROW8/bit0x40`, que es `IPT_UNUSED`. El valor del puerto cambiaba, pero no había efecto en pantalla. **Esta primera prueba no valida Enter** y queda archivada como evidencia del fallo de sonda, no del firmware.
- Se corrigió **sólo la sonda** (no driver, ROM ni binario): `neo_r6_y_enter_corrected.lua`, privada en `ALPHASMART/private/neo/R6_validacion_ROM`. Job `NEO_DIAG_R6_Y_ENTER_V2_20261114`: **STATUS=PASS, STAGE=END, EXIT_CODE=0** en BOOT y WRITER, con 45s simulados y NVRAM aislada por set.
- Frames 180–195: `Y` activa/soltada `FF→FE→FF`. Frames 300–317: `Enter [0x69]` activa/soltada `FF→BF→FF`. No se tocó hardware externo.
- Pantalla previo a Enter (frame245): `Your AlphaSmart needs to be restarted ... Press the enter key to restart...`. Después de Enter aparece una pantalla transitoria (frame360) y desde frame600 hasta 2600 se observa **`Attempting to enter the Updater Mode. Attach an Updater cable. Start AlphaSmart Manager.`**
- Captura [BOOT después de Enter, frame600](https://drive.google.com/file/d/1sWh_kUgXfpPSs5iDfwUHunsf5w80rqc_/view), [WRITER final frame2600](https://drive.google.com/file/d/1Itik6oE2wd2mwYj4qi5kPNuytb7ROiDD/view). Ambas presentan el mismo mensaje de actualización. 
- Se ejecuta código real de System3 durante la prueba: BOOT 427 PC distintos, WRITER 429; `first_system=10`, `first_applet=nil` en ambas. La secuencia de teclado es **PASS**, pero **no hay reinicio funcional hasta el escritorio ni Writer acreditado**. La presencia de una solicitud de actualización tampoco demuestra por sí sola una transferencia USB/serial.
- Evidencia completa: [resultados ambas variantes](https://drive.google.com/file/d/1WMi4jFmrThvMz0woyqyP8SRSBfsQh22e/view), [traza BOOT](https://drive.google.com/file/d/1APPz-jncJ4R6KFcNzQNLf0LiqXeBd2fS/view), [traza WRITER](https://drive.google.com/file/d/17FxThApECNBmbc6QdM1kYAcSXRKY9fhy/view), [estado final PASS](https://drive.google.com/file/d/105u1AZuu70lp1itYwlfZtacwv7k4TNWp/view).
- **Siguiente bloqueo real**: identificar por qué el flujo del sistema después de la confirmación y Enter selecciona la pantalla `Updater Mode`. Investigar condición de reinicio/NVRAM/NOR y selección de modo a partir de ROM y fuente existente; no simular manualmente un salto de PC ni declarar arranque Writer.
