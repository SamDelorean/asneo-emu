# R15 — ISP1763 PORTSC: recuperación de conexión, error 2671 superado y nueva frontera 886
Fecha: 2026-11-15. **No sustituye el ejecutable R14 instalado.** Un solo emulador `neo-diag`, rama `main`, sin binarios alternativos operativos, sin modificar ROM ni NVRAM de usuario.

## Estado del producto y del candidato
- Instalado y conservado (R14): `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA256 **`6a0298daa28c5937b7bb1202d7e2067b1a39198bdcc2f345a2bbd4b391cc7a05`**.
- Fuente R15: `src/mame/skeleton/neo_isp1763.h`, `tests/neo_usb_ports.cpp`, commit **`cfa5d191e4ac7b008f888ce66308ceeae89e733a`**. [CI GCC+Clang PASS](https://github.com/SamDelorean/asneo-emu/actions/runs/43734137206).
- MAME nativo R15 compilado con GCC 15.2: PASS. Candidato no instalado `/home/spc/Projects/alphasmart/neo-diag/neodiag`, SHA256 **`7f2db60b13cc6b8e6f96d2d1dab6eab1a55a7f9b3b5bfdc1a40c46b120d3c14d`**.
- El primer intento con D+ físico pero ECSC precoz `NEO_DIAG_R15_PORTSC_FS_20261115` terminó **PARTIAL, exit 0**, sin instalar: el firmware seguía viendo PORTSC `0x1801`, error `Usb1763.c:2671`.
- El segundo intento `NEO_DIAG_R15_POWER_EVENT_20261115` terminó **PARTIAL, exit 0**, sin instalar: **2671 superado**, sigue sin imprimir y aparece **`Usb1763.c:886`**. Ninguna intervención posterior en T: se alcanzaron las tres llamadas universales a Commander (inspección ROM, lanzamiento R15 original, lanzamiento R15 corregido).

## Ingeniería inversa dirigida: condición 2671, no inferencia de IRQ
ROM privada original WRITER ensamblada (4MiB, SHA256 `a14ccd938e1fccb62b839870dbb1942a9bfafb6d51f9dfd42d9ad82399665626`):
- String `Usb1763.c` en ROM offset `0x00045946`, VMA `0x00445946`.
- En VMA `0x004520B4`: `CMPI.L #0x00001803,(A7)`; rama `BEQ` omite el error. Si no coincide, se pasan línea `0x0A6F = 2671` y el puntero de filename a la rutina `0x00425562`. La llamada anterior solicita lectura de **PORTSC offset `0xA0`** y almacena el valor en pila.
- R14 sin impresora: `PORTSC=0x1000`; con `-prin`, `0x1001`; errores.
- R15 intento 1: el firmware ve `0x1801`. D+ está correctamente representado, pero ECSC no (ya quedó borrado prematuramente).
- R15 intento 2: **`PORTSC=0x1803`**, con el enlace eléctricamente alimentado y estable; firmware continúa su inicialización, habilita el puerto `0x1805`, `HcInterruptEnable=0x0100`, `HcInterrupt=0x0150`; el error 2671 desaparece. **No implica que la línea INT al VZ esté mapeada**.
- Tabla de la ficha ISP1763A Rev04 §10.1.6 (PORTSC1): `0x1000=PP`, `0x0800=LS(D+)`, `0x0002=ECSC`, `0x0001=ECCS`, `0x0004=PED`, `0x0100=PR`; escritura 1 en ECSC reconoce evento. [Ficha del fabricante](https://www.st.com/resource/en/datasheet/isp1763a.pdf).

## Qué se reutilizó de Dana y qué se mantuvo diferente
- De Dana: contrato del puerto raíz **power → cambio de conexión → reset/enable**, W1C del evento y temporizador virtual 1ms, con cualificación explícita de impresora montada.
- NEO: controlador **ISP1763**, registro `PORTSC +0xA0`, espera documentada de estabilización de alimentación (hasta 20ms), conexión sólo al completarse esa fase, indicación física D+ para periférico Full Speed. No hay `PORTSC` fijado artificialmente en `0x1803`: los bits resultan del estado eléctrico y del latch; tras W1C se observa `0x1801`.
- Dana: **ISP1161A1** MMIO y 8B PTD, no se copia el bloque de registros ni se asignan las IRQ2/IRQ3 de su PCB a NEO.
- Se preservan los endpoints, buffers y el printer image del único driver MAME. No hay USB físico nativo con el PC ni respuestas OS3K fabricadas.

## Nueva frontera: Usb1763.c:886
La captura del firmware del segundo intento ya **no** enseña 2671. Presenta error original **`Usb1763.c:886`**, y el archivo virtual `.prn` sigue con **0 bytes**. No atribuir el 886 a una función o componente sin desensamblar su callsite y registrar los PTD/register transfers reales; tampoco añadir ACK, texto de impresión o IRQ no cualificada.

Evidencias:
- [R15 primer intento, traza de lectura PORTSC](https://drive.google.com/file/d/167_f3x5ZUf_Txesj0OmfkFLAH4rLDmh6/view)
- [R15 segundo intento, estado PARTIAL](https://drive.google.com/file/d/1kGu_irgq3aDiveoniIAu8RXNSiTuy97r/view)
- [R15 segundo intento, traza de lectura/escritura PORTSC](https://drive.google.com/file/d/1B1_1Pa0iY1J4-DIofcqpz-jCIfPPQEX-/view)
- [R15 segundo intento, captura de error 886](https://drive.google.com/file/d/1LGmo1IaBoTDf7_aXn5sj4LmmNbW2t0uG/view)
- [SHA del ejecutable protegido y candidato](https://drive.google.com/file/d/1AeVq4InaqtdqaDTjoNDHp4QyRcUBThQ4/view)

**Criterio del siguiente paso dentro del mismo trabajo:** localizar la rama de error 886 a partir del firmware auténtico, revisar el PTD/HLT del host frente al ISP1763 y corregir sólo el contrato divergente; después comprobar producción de bytes `.prn` desde AlphaWord. No repetir R11–R14 ni la investigación de 2671, ya cerrada como causa concreta.