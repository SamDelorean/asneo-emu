# AGENTS.md — Reglas rectoras de Codex para asneo-emu
Fecha: 2026-11-14. Alcance: TODO el repositorio `SamDelorean/asneo-emu`. Autoridad: instrucciones del usuario; cualquier instrucción más reciente y explícita prevalece.

## Objetivo único
Construir un **solo emulador minimalista AlphaSmart NEO / NEO2** sobre **MAME** con ejecución genuina de SmallROM, System 3 y SmartApplets, aprovechando al máximo código comprobado de **DANA-emu, mame-as3k, AS2K-Emulator, ASPRO-emu, isotherm/mame y ioma8/neo-re**. Binario de trabajo previsto: `neo-diag`. No crear emuladores alternativos ni un segundo motor de CPU, pantallas sintéticas o shims que reemplacen permanentemente servicios del firmware.

## Antes de cambiar código: lectura y comparación obligatoria
1. Leer este archivo y `docs/REUSE_MATRIX.md`; comprobar el estado y commit efectivo del repositorio NEO y de cada origen, en vez de asumir versiones anteriores.
2. Leer planes/cierres vigentes de `SamDelorean/DANA-emu/docs/GENERAL_WORK_PLAN.md`, su driver `src/mame/skeleton/alphasdana.cpp` y sus componentes VZ; revisar `SamDelorean/mame-as3k/src/mame/skeleton/alphasma3k.cpp`; comparar con `isotherm/mame` rama `alphasmart` y con MAME oficial.
3. Leer `ioma8/neo-re/alpha-emu/README.md`, `docs/2026-04-20-alpha-emu-memory-map.md`, y fuentes `alpha-emu/src/{memory,lcd,keyboard,firmware}.rs`. Es una **referencia conductual y de hardware**, NO plataforma final.
4. Consultar `SamDelorean/betawise` (ABI OS3K), `SamDelorean/Dana-OS3K` (comparativas y fuentes VZ), y el material de Drive del NEO. No volver a investigar contratos ya cerrados salvo evidencia contradictoria.
5. Elaborar una tabla por subsistema con: fuente/commit, evidencia, estado anterior, compatibilidad NEO, cambio mínimo, riesgo, prueba dirigida y criterio PASS. Si ya existe implementación equivalente, reutilizar y adaptar; justificar por escrito cualquier duplicación.

## Ghidra, firmware y evidencia
- Usar preferentemente la instalación **existente** de Ghidra en T; revisar proyecto privado previo `alphasmart-research-master/neomanager_2024_01_01.gar` y workpapers antes de crear proyecto nuevo.
- Firmware privado oficial en Drive: `smallos3kneorom.os3kos`, `os3kneorom.os3kos`, SmartApplets. Las dos imágenes `.os3kos` son paquetes de actualización; NO afirmar que constituyen un dump íntegro de Flash.
- Hash SHA256 de referencia documental para NEO 2013 main: `32fe55bb50c1b58326bd303f14973b8402ddf8bf6662b210e4d5e9f931c7e6c0`; SmallROM: `6402c40878ba28747642ce474361b1596555b2a1f137db79c7ea8bb5b35a9523`. Recalcular contra bytes reales antes de usarlos.
- Importación Ghidra sugerida (confirmar bytes y headers antes): SmallROM ejecutable base `0x00400000`, reset PC `0x0040042A`; main OS paquete base `0x00410000`, entrada normal `0x00410082`. Procesador `68000:BE:32:default`. Extraer vectores, inicialización RAM/Flash, GPIO, LCD, teclado, trampas, cadenas SmartApplet y transición real de arranque. No confundir offsets del paquete con direcciones de ejecución.
- Matriz documental: **CONFIRMADO / INFERENCIA FUERTE / CANDIDATO / NO RESUELTO**. El parecido con Dana no acredita conexiones de placa NEO. No atribuir SPI1 a SD sin pruebas: en NEO 2013 se demostró SPI1 activo, pero su periférico externo quedó abierto.

## Arquitectura y no regresión
- Emular MC68VZ328/DragonBall VZ (no MC68EZ328 del AS3000); conservar alias MMIO `0xFFFFFxxx` y ancho de dirección correcto. Ajustar reloj/PLL contra firmware real, no fijar 33 MHz como verificación física por intuición.
- NEO LCD gráfico, visible 264×64, con dos controladores; **no** copiar LCD de caracteres AS3000. Revisar naturaleza exacta del controlador antes de denominar ST7675 una certeza.
- NEO teclado 16×8 y selectores reales; conservar la semántica de GPIO y lectura activa-baja. No sustituir por inyección de texto.
- No copiar indiscriminadamente el driver completo de Dana: el hardware compartido se adapta; SD, pantalla táctil, ST keyboard, puertos y placa Dana NO son periféricos NEO por defecto.
- Firmware hace el trabajo: no fingir llamadas A-line ni inventar contenido de archivos. Las validaciones de Rust `neo-re` son oráculo de observación, no reemplazo de SO.
- Un solo árbol, una rama principal de trabajo y un binario operativo `neo-diag`; evitar clones, worktrees, forks y ramas aisladas injustificadas. Conservar todo lo ya cerrado.
- Proteger ROMs originales, datos personales, dispositivos, archivos de estado y evidencia privada. Nunca subir ROMs/SmartApplets propietarios ni grandes volcados de Ghidra a GitHub público. Referenciar con hashes y ubicaciones privadas.

## Metodología T / Drive / Commander
- T canónica verificada históricamente: `spc-HP-t640-Thin-Client`, deviceId `dee0b4b6-578a-41a5-88de-73dbeb808983`. No interpretar esta referencia como comprobación de estado online actual.
- Reglas fuente en Drive: `AS2K_T640_DCJOB_OPERATING_RULES_20261009.md` y `AS2K_COMMANDER_DEVICE_BINDING_20261007.md`. Launcher histórico `/home/spc/.local/bin/dcjob launch`; NO usar `dcjob-verified-launch` ni asumir scheduling automático.
- **Sin autorización explícita para intervención remota NO usar Commander ni lanzar compilaciones/trabajos en T**. Cuando haya autorización: máximo **3 llamadas Commander por intervención**, salvo excepción explícita del usuario; usar deviceId específico; nada de polling exploratorio; agrupar reparación/disparo/verificación; consultar resultados en Drive; al agotar presupuesto clasificar BLOCKED + STOP.
- Antes de despachar, reconciliar `inbox/outbox/archive/failed/hold` del pipeline; `dcjob launch` requiere **exactamente un** `*.job.sh` en la inbox; nunca ejecutar trabajos de otros proyectos ni borrar/renombrar sin verificar. Construir scripts completos y comprobables, revisar `bash -n`; no reactivar jobs históricos sin autorización. Guardar stdout/stderr/exit code/SHA/summary en Drive.
- Respetar restricciones vigentes de compilación por equipo; no asumir que compilaciones se hacen indistintamente en T/Surface/Vontar. Comprobar el entorno autorizado para la tarea concreta, sin afectar instalaciones existentes.
- Mantener GitHub y Drive sincronizados al completar cada intervención autorizada; bitácora y matriz **únicas**, con comparación PASS real vs PASS estático vs PENDIENTE. Sin claims PASS sin ejecución real, sin crear excels duplicados, sin repetir auditorías cerradas, sin fragmentar labores integrales.

## Primer entregable de implementación
Una **auditoría de reutilización por módulo**, con rutas de fuentes, commits, matriz confirmada, diferencias NEO/Dana, contratos de firmware y criterios de pruebas. Sólo después implementar un único driver MAME inicial en este repo; compilar/probar en equipo autorizado; exponer salida real de reset de CPU hasta SmallROM y System 3 según lo efectivamente observado. Nunca proclamar firmware boot PASS por haber construido exitosamente el binario.
