# NEO-DIAG — despacho nativo T640 / 2026-11-14

## Identidad e invariantes
- Único ejecutable operativo: `neo-diag`; un solo árbol: `/home/spc/Projects/alphasmart/neo-diag`.
- Repo overlay: `SamDelorean/asneo-emu`, commit **`a1e52d5ced30f8f98dbe2a7633d42f5b51820c7c`**.
- Fuente MAME reutilizada localmente (sin modificar): `/home/spc/Projects/alphasmart/mame-as3k-dev`.
- Cola privada única: `googledrive:AS2K_TEMP/NEO_DIAG_PIPELINE`; mantener intacto el job AS3K en la cola común.
- Launcher T: `DCJOB_REMOTE=googledrive:AS2K_TEMP/NEO_DIAG_PIPELINE /home/spc/.local/bin/dcjob launch`.
- [Carpeta outbox](https://drive.google.com/drive/folders/1PlU2MJLhyV4V2Tkg5PRo5UofpycWLIxr).

## Ejecuciones reales
| Nombre | Resultado | Causa y acción |
|---|---|---|
| `000_neo_diag_native_build_20261114.job.sh` | FAIL rc=21, 10s | Precondición falsa: exigir que `.git` sea directorio. Se corrigió para comprobar el worktree con `git -C ... rev-parse`. |
| `..._retry1.job.sh` | FAIL rc=2 | MAME: `None of the specified source files contain system drivers`; faltaba `@source:skeleton/alphasneo.cpp\nasneo` en `src/mame/mame.lst` **del árbol NEO solamente**. No error de CPU/firmware. |
| `..._retry2.job.sh` | **RUNNING / BUILD_STARTED confirmado** | Registro MMIO/CPU y paquetes NEO se mantienen. Después de más de dos minutos desde su lanzamiento el compilador seguía trabajando: evidencia en `NEO_DIAG_NATIVE_R0_20261114_RETRY2.build_progress.log`. |

## Estado de validación
- El despacho y el inicio real de compilación **están acreditados**. La compilación completa/link/creación del ejecutable, la inclusión del driver `asneo` en `-listfull` y el arranque de firmware **no** están acreditados aquí.
- El script integra fuentes canónicas en el único árbol NEO y no copia ROMs privadas al Git público. Si el binario se produce, validará driver con `-listfull asneo` y ejecutará ROMs solamente si las encuentra en ubicaciones privadas conocidas.
- Outbox del segundo retry emite status en `BUILD_STARTED` y progreso MAME real (dependencias 3rdparty/flac compilando; advertencias no fatales). No confundir RUNNING con PASS.
- Uso de Commander sólo para disparo y verificaciones correctivas puntuales; sin polling permanente. Corridas previas preservadas en `failed`/outbox.
- Referencia de reglas: `AS2K_T640_DCJOB_OPERATING_RULES_20261009.md` en Drive. 
