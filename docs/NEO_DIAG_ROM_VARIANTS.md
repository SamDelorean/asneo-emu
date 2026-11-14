# `neo-diag` — Dos ROM privadas en un solo emulador

Fecha: 2026-11-14. La intención del usuario es usar ambas variantes de ROM igual que AS3K, **no** crear dos versiones ni dos binarios.

## Archivos privados verificados
| Variante | Flash | CRC32 | SHA-1 | SHA-256 |
|---|---:|---|---|---|
| `OS3K_NEO_MIN_BOOT.bin` | 4 MiB | `0bb36e55` | `5930ee13122d26d690748206407baeec5e33be06` | `d9ca602b1253e727e39f53ab113e00c4f87a5835a0843845622ca50dea3cf4bc` |
| `OS3K_NEO_MIN_WRITER.bin` | 4 MiB | `080b51c3` | `7bf9476ff6eb3384da05cacea8bb20a641043c3a` | `7fbc224f050d648011e1e50c7dc1ba3c5eda109e3b02c817e0ae03ec042b9820` |

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
