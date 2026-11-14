# R7 — NEO: condición Updater tras Y → Enter (2026-11-14)

## Comportamiento correcto y fallo
En primer arranque con AlphaWord instalado, Y (confirma cambio de batería de respaldo) y después Enter deben conducir a la inicialización normal y al editor. La pantalla `Attempting to enter the Updater Mode` es **fallo funcional del emulador WRITER**, no PASS de arranque. El mensaje sí corresponde a una condición conocida en NEO reales cuando no hay SmartApplets instalados/reconocidos: `lykahb/neotools` README: https://github.com/lykahb/neotools#attempting-to-enter-the-updater-mode. Referencia del flujo Y/Enter en dispositivo: https://saudette.net/neo/screens.php y experiencia de sustitución de batería en https://blog.tinycities.net/alphasmart/replacing-the-backup-battery.

Se conserva **un solo** `neo-diag` y ambas ROM BOOT/WRITER. No se modificaron binarios ni las ROM durante este diagnóstico.

## Evidencia estática de WRITER
- Flash canónica 4 MiB `OS3K_NEO_MIN_WRITER.bin`, SHA-256 `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626`.
- `0x00449602`: encabezado `C0FFEEAD`, SmartApplet interno `System`.
- `0x00470000`: encabezado `C0FFEEAD`, longitud `0x0001A0BC` (106,684 bytes), identificador `AlphaWord Plus`, entry relativo `0x94`. Los bytes de AlphaWord están completos, pero eso **no implica instalación reconocida**.
- **BOOT** no contiene applets de usuario por diseño; su acceso al actualizador no demuestra fallo de CPU/teclado. La variante para criterio de editor es **WRITER**.

## Comparación controlada en T: perfil Flash / NVRAM
Job `NEO_DIAG_R7_APPLET_PROFILE_20261114` PASS, sólo lectura diagnóstica, **mismo ejecutable y ROM**, cuatro casos de 32 s simulados:
- Perfil 0: NOR desconocida/solo lectura, primer arranque Y→Enter y arranque posterior sin entradas con la NVRAM creada.
- Perfil 1: ST M29W320EB (perfil experimental ya disponible), la misma secuencia y arranque posterior.
- En ambos perfiles, `R7_END` acreditó ejecución System 3, **0 muestras PC en área AlphaWord**.
- Los cuatro archivos de RAM persistida resultaron byte por byte idénticos al comparar a la misma fase entre perfiles (primer arranque y segundo arranque); perfiles 0 y 1 **no cambian el fallo observado**.
- El registro `NOR_REJECTED` no apareció durante la ventana probada en el flujo de diagnóstico. Esto no prueba ausencia global de escrituras ni identifica el chip físico; sólo descarta que cambiar el perfil sea solución en esta prueba.

### Inventario runtime original NEO 2013
La ingeniería inversa ABI ya archivada del usuario ubica en NEO13 la **tabla de 32 punteros a `AppletHeader_t` en SRAM `0x00000E0A..0x00000E89`**; servicios A238/A23C leen esa tabla. Es un origen más directo que inferir presencia del applet a partir de sus bytes en NOR.

Se inspeccionó la RAM NVRAM **persistida de T** después de cada caso, sin tocar el hardware ni manipular PC:

| Campo | Valor en cuatro pruebas |
|---|---|
| Slot 0 `0x00000E0A` | `0x00449602` → `System`, encabezado C0FFEEAD |
| Slot 1 `0x00000E0E` | `0x00000000` |
| Slots 2–31 | Cero |
| Flash `0x00470000` | `C0FFEEAD` AlphaWord, intacta |
| RAM `0x00000E8A` | `0x00470000` |
| RAM `0x00000E8E` | `0x0040000C` |

**Resultado inequívoco:** System3 no registra AlphaWord en ninguno de los 31 slots instalables, aunque su contenedor esté presente y válido en NOR. Tampoco lo hace tras reiniciar el emulador usando la NVRAM persistida. Ese déficit explica funcionalmente el `Updater Mode` acorde con neotools, sin atribuir todavía por qué el instalador/arranque no lo registra.

**Precisión sobre `0x0E8E`:** en `ioma8/neo-re` aparecen `0x0E8A` y `0x0E8E` como límites de almacenamiento de applets; el valor observado `0x0040000C` es incompatible con ese modelo. Sin embargo, dado que el proyecto de ingeniería inversa original contiene estructuras contiguas a la tabla de 32 punteros, no debe tratarse aisladamente ese valor como causa demostrada sin seguir los escritores/lectores del firmware. El hecho primario demostrado es **slot1 cero** y no instalación reconocida.

[Evidencia completa de 32 slots, cuatro casos, fuente T](https://drive.google.com/file/d/12Dnzh2KY2r-xlELYZimpZqTNsREh1HV6/view). [Resumen perfil 0](https://drive.google.com/file/d/1NEOQ5NWqgAZL8YgieVN_mzI23JL31rOd/view) / [perfil 1](https://drive.google.com/file/d/1EU6cfYHcJFhEfRtpdwGY8PHwU4oyEXtv/view). [Estado final R7](https://drive.google.com/file/d/1CwH7hFhrO2kTtTYPJ03LBo61e-T8X2O2/view).

## Trabajo siguiente y restricciones
1. Rastrear en la **ROM System3 NEO13 original** la rutina que construye la tabla `0xE0A`/registra applets, su contrato con Flash y la condición que habilita slot1, usando las bitácoras/ingeniería inversa ya existentes del usuario. No inventar rutas ni código de bootstrap ajenos.
2. Determinar si `.os3kapp` requiere registro persistente o una cadena de instalación adicional más allá de copiar bytes en `0x00470000`. **No basta un SHA válido**.
3. Preparar única corrección justificada, dentro del **mismo** `neo-diag` o el ensamblador privado según corresponda, sin parchear PC y sin simular que AlphaWord abrió. Repetir Y→Enter con NVRAM aislada; criterio de aceptación: slot1 apuntando a cabecera real AlphaWord, PC de applet y LCD de editor, más prueba de escritura y persistencia de texto.
4. No cambiar el perfil NOR por defecto sin identificación física cualificada; no sobrescribir imagen privada sin prueba previa; conservar las versiones históricas y NVRAM de ensayo.
