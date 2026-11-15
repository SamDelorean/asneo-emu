# R9 — Conjunto mínimo de arranque NEO13 y alcance de AlphaWord (14 nov 2026)

## Criterio
Un arranque aceptado no es sólo ejecutar el MC68VZ328, cargar System3 ni visitar el entrypoint de AlphaWord. El criterio exigido por el usuario es completar **Y → Enter → editor de texto**, aceptar texto y conservarlo en una NVRAM de ensayo al reiniciar. Mantener un solo `neo-diag` con BOOT/WRITER. Imágenes privadas no modificadas.

## Conjunto mínimo por nivel de evidencia

| Componente | Clasificación | Evidencia |
|---|---|---|
| SmallROM NEO 2013 | Requerida por el camino de reset implementado | Vectores originales SSP 0x0007FFF0, PC 0x0040042A; ejecuta hasta System3 |
| System 3 NEO 2013 (OS) | Requerido | Encabezado System interno, slot0 de la tabla `0xE0A` = `0x00449602` |
| AlphaWord Plus (id `0xA000`, 106684 B) | **Mínimo applet de usuario para abrir escritor** | Contenedor válido en `0x470000`; R9 tras CFI: slot1 `0xE0E` = `0x00470000`, PC entra en `0x00472C28` |
| Neo Font Small/Medium/Large/Very Large/Extra Large | Recursos adicionales, obligatoriedad individual NO demostrada | Instalables separadamente en recuperación de NEO físico, después de AlphaWord |
| Calculator / Control Panel / Beamer / AlphaQuiz / KeyWords / SpellCheck / Thesaurus | **No necesarios para registrar ni iniciar código de AlphaWord** | AlphaWord oficial incluye rutas explícitas para ausencia de Calculator, SpellCheck y Thesaurus. ControlPanel/Beamer son applets independientes. No hay prueba de interfaz completa sin ningún otro recurso |

No confundir **mínimo demostrado para alcanzar código de AlphaWord** con **mínimo probado para editor funcional**: el segundo todavía NO está acreditado.

El README del emulador de referencia `ioma8/neo-re/alpha-emu` utiliza AlphaWord, Calculator y Control Panel como un **paquete de referencia**, no demuestra que los tres sean esenciales. Su [recovery runbook en NEO físico](https://github.com/ioma8/neo-re/blob/main/docs/2026-04-18-neo-recovery-runbook.md) restablece los applets individualmente en orden **AlphaWord → fuentes → otros**, con comprobación tras cada instalación. Esto tampoco prueba que **todas** las fuentes sean indispensables. En la ROM AlphaWord original hay textos `Spell Check SmartApplet not found.`, `Calculator SmartApplet not found.` y `Thesaurus SmartApplet not found.`; se han verificado directamente en el binario privado a offsets locales `0x15839`, `0x158E2`, `0x18AA8`.

## R9 — Corrección NOR CFI verificada de forma nativa (T640)
- GitHub código: commit `0b8f100748b5cf1bf3892ac1baefa2e9f5432e2f`, modificación acotada de `neo_nor.h` y `tests/neo_nor.cpp`, permitiendo únicamente `MOVE.B #0x98,$4000AA` y salida de consulta CFI, con el perfil ST provisional seleccionado **explícitamente**.
- Job `NEO_DIAG_R9_CFI_NATIVE_20261114`: **STATUS=PASS / EXIT_CODE=0**, tests nativos, compilación/enlace y auditoría ROM PASS; [status](https://drive.google.com/file/d/16q7qTG3U7SmpHtQu-JChCsRPJiPKwY_7/view), [resultados](https://drive.google.com/file/d/13s_iYLA8gtdVexgL9paJ3hSiqQm8wSgJ/view), [CFI trace](https://drive.google.com/file/d/1peoSrIKFl-lPOCRu3_DKEQDb2mCMSecm/view).
- Firmware original ve secuencia CFI `QRY` (0x51/0x52/0x59), comando set 0x02, geometría de 4 MiB.
- Los límites runtime pasan de erróneos `0x00470000–0x0040000C` a válidos **`0x00470000–0x00800000`**.
- El slot1 real de `AppletHeader_t*` pasa de cero a `0x00470000` tras Y → Enter. AlphaWord es **registrado sin crear manualmente punteros ni ejecutar instalación USB**, y el PC entra en su código (frame 600, PC `0x00472C28`). Esto demuestra que el defecto R7 de inventario provino de la lectura de geometría, no de una obligación de ejecutar el protocolo host `0x06/0x02/raw/0x0B/0x07`. Ese protocolo sirve a instalaciones en el hardware, no hace falta para leer la cadena preinstalada en esta imagen de prueba.
- Ejecutable único instalado: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA-256 **`8856f0dbab93da3961ebfec8bd413672ebc7d627eb3f3e42c3c79980cdd3b580`** (antes: `386635d9f9e9eef1d48662d6ea0834b03fa37445de175c306352d28ba4490534`). ROM WRITER privada no cambia: SHA256 `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626`.
- No cambiar NOR_PROFILE por defecto sin identificar físicamente el chip. R9 fue probado con **ST profile=1** y NVRAM temporal aislada.

## R9 — Prueba de pantalla y escritura
- Primer trabajo visual `NEO_DIAG_R9_WRITER_UI_20261114` falló por GC de callbacks Lua locales, sin afectar el firmware, ROM ni el binario.
- Corrección únicamente de la sonda, manteniendo callback global; segundo job `NEO_DIAG_R9_WRITER_UI_V2_20261114` **PASS / EXIT_CODE=0**. [Status](https://drive.google.com/file/d/1UwBK8gJ3AQaVbd3Gzzv4BAU4HSBIPSNJ/view); [resultados](https://drive.google.com/file/d/1Lipjugf7W3z2zvjH_8KEJaikBjvwTIkc/view); [traza](https://drive.google.com/file/d/1IbBj_YMuN9jLLdbeWM2outyj9TGdr2Sl/view).
- Secuencia Y (ROW15/0x01), Enter (ROW9/0x40), después teclas físicas T/E/S/T (ROW13/0x01, ROW10/0x08, ROW11/0x04 y ROW13/0x01) ejecutada sobre NVRAM aislada.
- Entró a AlphaWord (`first_applet=582`) y conserva slot1 `0x00470000` hasta el final; sin embargo desde frame720 hasta frame1900 el histograma de LCD permanece en **32 píxeles oscuros**, sin variación visible después de las teclas de texto. El PC muestreado al final se mantiene en `0x00426758` (rutina System 3). El histograma y el envío del evento por la sonda por sí solos **no acreditan caracteres aceptados**.
- MAME guardó 11 snapshots de ensayo en T; el primer upload de snapshots no recogió el subdirectorio del sistema, por lo cual falta inspección visual detallada. No representar imágenes no disponibles como evidencias. **`EDITOR_UI=NOT_ACREDITED`, `TEXT_INPUT=NOT_ACREDITED`, `TEXT_PERSISTENCE=NOT_TESTED`**.

## Siguiente frontera funcional, acotada
La ausencia de Calculator/ControlPanel/Thesaurus/SpellCheck no justifica aumentar artificialmente la ROM mínima. Antes de añadir un Neo Font, comparar capturas reales y estado del fichero AlphaWord/cursor con la documentación de la RAM NEO13 (`0x0FDA` y tablas de archivos), y comprobar el lector de teclado en la ruta de edición. Si el firmware solicita ID de fuente, introducir **únicamente** el recurso real solicitado, no el lote completo. Un fallo de registro de applets ya no es el bloqueo: R9 lo cerró.

La secuencia final de aceptación no cambia: ROM audit PASS, CFI PASS, slot1 AlphaWord, PC applet, **LCD reconocible del editor**, escritura de prueba visible y persistente tras reinicio. No parchear RAM/PC para ocultar la deficiencia.