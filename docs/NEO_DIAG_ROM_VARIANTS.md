# `neo-diag` — Dos ROM privadas en un solo emulador

Fecha: 2026-11-14. La intención del usuario es usar ambas variantes de ROM igual que AS3K, **no** crear dos versiones ni dos binarios.

## Archivos privados verificados
| Variante | Flash | CRC32 | SHA-1 | SHA-256 |
|---|---:|---|---|---|
| `OS3K_NEO_MIN_BOOT.bin` | 4 MiB | `7321c03f` | `8b556e1dda4588b9380fbf895ece11c52c7f18f8` | `e0e962dfaf7cc8f914c17708c8f9da924e13e44fb0bf8d7fbe015bd8c5c87765` |
| `OS3K_NEO_MIN_WRITER.bin` | 4 MiB | `7099ffa9` | `334b867183acf6f537823ddd4aa6743fe3d2dd8e` | `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626` |

Ambas están en `ALPHASMART/private/neo/os3k_minimal` y la copia local autorizada debe quedar en `/home/spc/Projects/alphasmart/private/roms/neo/os3k_minimal`.

## AlphaWord original NEO
Origen `ALPHASMART/SmartAppletsNEO/alphawordplus.os3kapp`, SHA256 `a8ae516734a0e0b2e19d51ffb7dff3893074ca733fa40207cbff7ead1433f29e`; 106684 bytes (`0x1a0bc`), magic `c0ffeead`, trailer `cafefeed`, header `AlphaWord Plus`, entry offset `0x94`. Cargado sin alteraciones en `0x00470000..0x0048a0bb`. Coincide con el punto de inicio de almacenamiento de applets NEO 2013 documentado en `ioma8/neo-re/alpha-emu/src/memory.rs`, `STOCK_APPLET_BASE=0x00470000`.

El constructor privado `build_neo_min_writer.py` usa exclusivamente la BOOT ya validada y el contenedor original, comprueba hashes, cabecera, tráiler, longitud, entry y zona borrada. La regresión de comparación verifica que **el resto de los 4 MiB permanece idéntico** a BOOT. Las imágenes de actualización originales no se modifican.

## Selección en el único MAME `neo-diag`
Se conserva `asneo` para paquetes `.os3kos` individuales, pero se habilitan dos sets de datos **en el mismo driver, la misma configuración de máquina y el único ejecutable**:
- `neo-diag asneob`: carga `OS3K_NEO_MIN_BOOT.bin` en región `flashrom`.
- `neo-diag asneow`: carga `OS3K_NEO_MIN_WRITER.bin` en región `flashrom`.

En las definiciones MAME se incluyen CRC32/SHA1 reales. No hay firmware nuevo en Git, ni otro driver, ni build separado. La región `flashrom` sustituye el proceso de importación `smallrom/system` para los dos sets y alimenta el NVRAM virtual sólo cuando no existe imagen anterior del set.

### Cuidados de pruebas
- MAME persiste NVRAM por nombre de sistema: cada set tiene medio virtual distinto.
- Ante un NVRAM previamente guardado, MAME mantiene el estado persistido y no vuelca de nuevo la ROM de base. Para pruebas de arranque limpio usar un directorio NVRAM de pruebas separado, no borrar archivos de usuario.
- **PASS de construcción y comparación de imagen es sólo estático.** El driver completo, carga MAME, salto al sistema y ejecución de AlphaWord quedan pendientes de compilación y evidencia real; `MACHINE_NOT_WORKING` permanece.
- Si una compilación nativa `neo-diag` está activa, no modificar su árbol a mitad de compilación. Integrar este commit en una futura compilación incremental tras obtener resultado final del trabajo previo.

## Evidencia nativa T640 — PASS (2026-11-14)

Commit integrado y compilado: `a09d1479c5adbc242026302e20b3400c8c68bd6f`.

- Árbol y **binario únicos**: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`; SHA256 `667a0014fd5809b3ff669df3ec9522d5215caed63ed794fb629ea3a6543db788`.
- Flash privada verificada por SHA256 en `/home/spc/Projects/alphasmart/private/roms/neo/os3k_minimal`, ambas imágenes de 4 MiB, intactas.
- El driver compiló y enlazó incrementalmente con GCC 15.2, utilizando dependencias anteriores; `asneo`, `asneob` y `asneow` registrados.
- Auditoría MAME real: `romset asneob [asneo] is good`, `romset asneow [asneo] is good`; ambos `1 romsets found, 1 were OK`.
- Prueba sin GUI `-seconds_to_run 3`, NVRAM de ensayo independiente: ambos sets llegaron a salida `MAME_EXIT_CODE=0`; no hubo errores de validación de etiquetas de ROM tras corregir el uso obligatorio de minúsculas en `ROM_LOAD`. Las etiquetas MAME son `os3k_neo_min_boot.bin` y `os3k_neo_min_writer.bin`; son enlaces a los archivos canónicos privados `OS3K_NEO_MIN_*.bin`, no copias.
- En el entorno headless aparecen avisos del sistema gráfico `XDG_RUNTIME_DIR` y ALSA (no se evaluó audio); no son demostración de fallo de CPU.
- **NO** se declara `FULL_OS_BOOT` ni `WRITER_VISIBLE`: no se verificó PC alcanzando System 3, interfaz LCD con texto ni pulsaciones host->Writer.

Resultados en Drive (`AS2K_TEMP/NEO_DIAG_PIPELINE/outbox`):
- [Estado PASS](https://drive.google.com/file/d/1awxErCClx_iuP6AOKGjKoIVv9kE6XgiK/view)
- [Comprobación de ROMs](https://drive.google.com/file/d/1-MH57LGYlnrHnk9mP0ORW6nbXeDdg5At/view)
- [Ejecución BOOT](https://drive.google.com/file/d/1mgjPBQH2ufNnzYu6R0RtY3PEsb2djCXN/view)
- [Ejecución WRITER](https://drive.google.com/file/d/1Sof9hGZRYbdMuluHqguS2IXlDMbIn7PT/view)
- [Hashes de imágenes y ejecutable](https://drive.google.com/file/d/1gAgDk7ulEEEP9p55ffNMytNj4JiaDFFR/view)
- [Estado funcional desglosado](https://drive.google.com/file/d/1HUyCTLz76s5THqdW35B3MgOyYs2iEybz/view)

La primera validación `46be3fc` sí compiló y comprobó ROM, pero MAME detectó etiquetas uppercase inválidas en los dos sets durante la prueba del proceso, aunque éste retornó 0. Se corrigió en `a09d147` y sólo este último se considera el resultado válido y cerrado de integración.

## Rutas operativas permanentes en T

Los dos sets MAME fueron registrados también en la ruta **privada permanente** `/home/spc/Projects/alphasmart/private/roms/neo/mame_sets`, utilizando únicamente enlaces simbólicos a las dos imágenes originales de `os3k_minimal`, sin duplicarlas ni reescribirlas.

Comandos reales del mismo ejecutable:
```bash
/home/spc/Projects/alphasmart/neo-diag/neo-diag asneob \
  -rompath /home/spc/Projects/alphasmart/private/roms/neo/mame_sets
/home/spc/Projects/alphasmart/neo-diag/neo-diag asneow \
  -rompath /home/spc/Projects/alphasmart/private/roms/neo/mame_sets
```

Ambos `-verifyroms` con la ruta permanente: **PASS**, un ROM set correcto por variante. [Recibo generado en T](https://drive.google.com/file/d/1V3EBOCYHP0xncNqWrNhBpgHho2E-d4HF/view). Boot real System 3 y funcionamiento del editor siguen **NO ACREDITADOS**.

## Diagnóstico de arranque real R4
Las dos imágenes ejecutan SmallROM, pero no tienen transición a System 3 acreditada incluso en 24 s simulados. La UART VZ no está integrada en el driver. Evidencia y límites en [R4_BOOT_TRACE_20261114.md](R4_BOOT_TRACE_20261114.md). No confundir la prueba previa de proceso con un sistema operativo funcional.

## R5 UART VZ — Integración nativa cerrada
La UART universal de Dana ya está incorporada al único `neo-diag` en T con `ROM_AUDIT=PASS`, ejecución de BOOT y WRITER de 24 segundos, y binario SHA256 `98270ba2c257ccbac6a3ca52d7c6c0c6c991ab5fb536d5fc5ba7a4a96d1ca4d6`. Mejoró la progresión dentro de SmallROM (115→235 PC distintos), pero System 3/AlphaWord todavía no presentan arranque visible o PC en sus regiones. Ver [R5_VZ_UART_INTEGRATION_20261114.md](R5_VZ_UART_INTEGRATION_20261114.md).

## R6 — Corrección de las dos ROM activas (2026-11-14)
El ensamblador de firmware se corrigió al mapa real de bloques de 0x400 bytes; System3 ahora ejecuta sus rutinas y presenta el diálogo auténtico de batería. La tecla Y se verificó, y el siguiente paso físico de interfaz pide Enter para reiniciar. El único binario en T es SHA256 `386635d9f9e9eef1d48662d6ea0834b03fa37445de175c306352d28ba4490534`. Los SHA de la tabla inicial son los **activos vigentes**; las pruebas R4/R5 documentadas más arriba son históricas y usaron los viejos medios R3 inválidos. Ver [R6](R6_FLASH_SEGMENT_ALIGNMENT_20261114.md).

## R6 — Confirmación Y y Enter
Teclas verificadas: Y `ROW15/0x01` y Enter `ROW9/0x40`. En ambas ROM el mensaje de batería desaparece y después de Enter se muestra **Updater Mode**, no el escritorio ni Writer. La secuencia de entrada pasó, pero el reinicio normal/AlphaWord sigue **NO ACREDITADO**. [Detalles y evidencia](R6_FLASH_SEGMENT_ALIGNMENT_20261114.md).
