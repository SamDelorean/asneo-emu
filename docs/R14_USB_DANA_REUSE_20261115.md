# R14 — Hardware USB común: semántica Dana, prueba real Print y límite ISP1763
**15 de noviembre de 2026.** Destino único: `neo-diag` en T640, MAME 0.289, sin modificar ROM, datos de usuario ni `DANA-emu`.

## Código y ejecutable entregados
- Código fuente de hardware: commit `acd6709e3f3f723d9a2b9ca82843a559b9834d80`, CI GCC/Clang PASS: https://github.com/SamDelorean/asneo-emu/actions/runs/43729037985
- Único ejecutable instalado: `/home/spc/Projects/alphasmart/neo-diag/neo-diag` SHA256 **`6a0298daa28c5937b7bb1202d7e2067b1a39198bdcc2f345a2bbd4b391cc7a05`**.
- ROM WRITER intacta SHA256 `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626`.
- Trabajo exclusivo NEO R14: **PASS, STAGE=END, EXIT_CODE=0**. [Estado](https://drive.google.com/file/d/1uHaLVYfqDf-0NHSfPJfdXW_z3vPz9Mv6/view), [resumen](https://drive.google.com/file/d/1Xel0w_W3v8GIucmAujGG0VL-i8IeYYHC/view), [SHA](https://drive.google.com/file/d/1BxtqjoLiqnF26ofQi7jyCAUduOuOXhG7/view).

## Reutilización precisa de Dana
| Bloque | Origen Dana | R14 en ISP1763 NEO/NEO2 | Evidencia |
|---|---|---|---|
| Puerto host/reset | `isp1161_root_update` y `isp1161_root_write` en `DANA-emu/src/mame/skeleton/isp1161_dana.h` | `PORTSC +0xA0`; espera de 10 ms en reloj virtual de 1 ms, cambio de habilitación después del reset, desconexión invalida espera | C++ GCC/Clang y T PASS |
| Servicio periódico | `isp1161_usb_service` | `m_usb.tick_1ms()` en timer USB MAME existente, sin nuevo loop ni hilo externo | MAME nativo PASS |
| Device SET_ADDRESS | `pending_address` y `active_address` de Dana, activación tras IN/ZLP status ACK | NEO DC `+0x00` lee dirección solicitada, dirección de cable cambia únicamente al ACK de estado del EP0; bus reset cancela pendiente | C++ GCC/Clang y T PASS |
| Printer backend | `printer_image_device` y paquete de clase Printer reutilizados R13 | Printer MAME se monta con **`-prin`** en NEO (alias real `printout (prin)`) | MAME media y conexión física virtual PASS |

**No** se copian los registros ISP1161A1 `0x1F000000..7`, PTD 8 B ni las IRQ2/IRQ3 de placa Dana. El firmware NEO13 accede a ISP1763 vía `0x02000000..FF` y PTD ATL 32 B. El chip tiene una salida física INT; **no está demostrado a cuál GPIO/IRQ VZ conecta en la placa NEO**. El motor HC/DC es común; la radio particular NEO2 queda fuera de R14.

## Prueba funcional original de impresión, no sintética
Se ejecutó la ROM WRITER original, se entró a AlphaWord mediante **Y→Enter**, se escribió TEST por matriz real y se presionó la tecla física **Print** (fila 9, bit 4 / raw `0x49`), con una impresora virtual MAME montada por `-prin` y NVRAM de ensayo aislada. El dispositivo conectado cambió el registro host:
- Sin impresora: `PORTSC=0x1000`.
- Con impresora: `PORTSC=0x1001`; `HC_INTERRUPT=0x0050`, `HC_INTERRUPT_ENABLE=0x0000` durante la prueba.
- Pantalla genuina (frame 2140): **Printing File 1. Looking for printer.** [Captura](https://drive.google.com/file/d/1_pxxttSi_VFMVJp0UImUEejpZEOfNXR5/view).
- Frames 2600 y 3450: error del firmware **`Usb1763.c`, número `2671`**. [Captura](https://drive.google.com/file/d/1bDvT3Lc2QvGdmKbnuHUlvTmIgqzjiT9t/view).
- Archivo `.prn`: **0 bytes**; la función **NO** se califica como impresión operativa. [Resultado](https://drive.google.com/file/d/1Sz8VkZu99fv3hfYfCRIMYB9oq-v268_R/view), [traza](https://drive.google.com/file/d/10mpB-HrboIZGfd4lBJNYwyDMAs81eGzv/view).

Este error **no demuestra por sí solo** que la causa sea el pin INT, un offset, un protocolo o el código de print. Se conserva la firma exacta como punto de partida de ingeniería inversa, sin inventar cableado ni un servicio del SO. La prueba confirmó que la ruta original de OS3K intenta acceder al controlador y que el puerto físico virtual detecta el periférico.

## Resultado delimitado / siguiente frontera REAL
**Cerrados:** compilación MAME, 5 pruebas C++ nativas, 2 ROM audit, arranque WRITER/AlphaWord, MMIO CHIPID, 10 ms reset, cambio de dirección tras ACK, detección de impresora por PORTSC. El test original de impresión corre pero termina en `Usb1763.c:2671`.

**Abiertos:** identificar la condición de esa excepción con fuente/disassembly NEO13, completar la inicialización HC de firmware real, verificar el pin INT frente al MCU mediante evidencia NEO, producir bytes de impresión desde AlphaWord y USB Send/Manager bidireccional. Las pruebas de paquetes R13 siguen siendo válidas pero **no cierran estos puntos**. No repetir gates R11/R12 ni crear otra versión del emulador.

Metodología: una cola NEO en Drive, un binario, **tres llamadas a Commander en total** (R14 compile/install, Print dirigida, copia de pantallas), dentro del límite universal. Se entregan todos los logs en outbox y se archiva el script de prueba, sin pruebas sobre archivos de usuario.
