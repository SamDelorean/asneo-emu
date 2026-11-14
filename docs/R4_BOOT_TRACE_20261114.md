# R4 — Seguimiento real de arranque MC68VZ328 / 14-nov-2026

## Alcance y producto
Único emulador **\`neo-diag\`**; ROMs privadas **BOOT** (\`asneob\`) y **WRITER** (\`asneow\`). Sin edición de las imágenes de firmware, sin nuevas variantes del emulador, sin alteración de otros proyectos. MAME compilado 0.289, SHA-256 binario \`667a0014fd5809b3ff669df3ec9522d5215caed63ed794fb629ea3a6543db788\`.

## Evidencia verificada
- Despacho \`NEO_DIAG_R4_BOOT_PC_20261114\`: \`STATUS=PASS, STAGE=TRACE_COLLECTED\`. [Recibo](https://drive.google.com/file/d/1SSnRlDmNi6wySayU1KS4VARmfpO0ksrx/view).
- [BOOT, traza de 4 segundos](https://drive.google.com/file/d/1O6HQJmkLSIZRSJOE38TSeNi4PpSXQ3PB/view); [WRITER, traza de 4 segundos](https://drive.google.com/file/d/18YdVuLj72hX-hUf0nkEgOHxq_qKhtEu6/view). MAME Lua leyó los **registros reales** \`PC\`, \`CURPC\`, \`SR\`, \`SP\`, \`D0\`, etc., mediante \`manager.machine.devices[':maincpu'].state\`. Ambos sets proporcionaron idénticas 242 muestras y 101 PC distintos; ambas ROM tienen vectores \`SSP=0007FFF0\`, \`PC=0040042A\`, y la primera instrucción en Flash es \`46FC\`. En las 241 muestras posteriores al instante inicial, el PC estuvo dentro de Flash.
- **Cero muestras** con PC en el tramo System 3 \`0x00410000..0x0046FFFF\`, y cero en AlphaWord \`0x00470000..\`. No afirmar que no hubo tránsito entre muestras: instrumentación por frame, no traza instrucción a instrucción.
- [BOOT extendida 24 s simulados](https://drive.google.com/file/d/1TJHngnaeLiRep0YbLPGkrZKDekhdS4I-/view): \`STATUS=PASS / EXTENDED_TRACE_COLLECTED\` ([recibo](https://drive.google.com/file/d/1MTb2Jj7T06gq_fgVWF9XifuwMB2W0WP2/view)), **1,442 muestras / 115 PC distintos**, 0 muestras System 3, 0 AlphaWord, 1,441 muestras en Flash SmallROM. Las regiones más frecuentes fueron \`004026xx\` (770), \`004021xx\` (400); al finalizar PC \`00401Bxx/004026xx\` seguían activos. No se obtuvo \`BOOT_PASS\`.
- La región del sistema contiene bytes originales válidos: lectura durante emulación en \`0x00410082\` devolvió \`2F0B\`; no es una región sin programar.
- [Inspección de NVRAM y RAM de ensayo](https://drive.google.com/file/d/1-icWBEkJde10dY13ZifxTF5Ac8N6g2Fy/view): NVRAM Flash 4 MiB SHA-256 \`d9ca602b1253e727e39f53ab113e00c4f87a5835a0843845622ca50dea3cf4bc\`, **igual que la BOOT original**, sin modificaciones de Flash por la ejecución. SRAM 512 KiB con registros/estado de runtime; pila emulada en \`0x0007FFxx\`. La imagen ROM del fabricante permanece intacta.
- Ambas pruebas terminaron con salida del proceso 0. Esto **no** demuestra boot completo; sólo significa que MAME ejecutó el tiempo fijado y terminó normalmente.

## Diagnóstico acotado — hardware compartido real
Los accesos de la SmallROM original alrededor de \`0x00402680..0x004026F4\` incluyen registros VZ **Timer1** \`0xFFFFF604\` / \`0xFFFFF60B\` y **UART** \`0xFFFFF906/07\` (confirmado también por A0 en la traza). El driver actual \`alphasneo.cpp\` maneja Timer1 en \`neo_vz_core.h\`, pero su lectura \`vz_read8\` cae en \`NEO_VZ_UNIMPLEMENTED_READ\` para \`0xF906\` y devuelve \`0xFF\`; \`vz_write8\` rechaza las escrituras \`0xF907\`. Es una **deficiencia real del driver**, independientemente de que sea o no la única causa de no alcanzar System3.

Se localizó reutilización inmediata, no un subsistema nuevo: \`SamDelorean/DANA-emu/src/mame/skeleton/dana_vz_uart.h\`, clase \`dana_vz_uart_device\` (UART1/2 MC68VZ328 con FIFOs, framing, reloj, temporización y save-state) + registro en \`alphasdana.cpp\`. Debe trasladarse a NEO **únicamente la parte VZ universal y su conexión a los MMIO/IRQ originales**, no hardware USB/SD/SIR/teclado de Dana. No inventar un periférico serial conectado cuando no existe.

No se puede afirmar aún que la UART por sí sola resuelva la transición SmallROM → System3: puede haber una condición previa de arranque o un estado de recovery. Una vez integrado el componente, repetir la **misma traza con la misma ROM BOOT** y comparar PCs, y sólo entonces inferir el siguiente bloqueo. No parchear vectores a \`0x00410082\` ni emular artificialmente la función de OS.

## Clasificación
- [PASS] Integridad BOOT y WRITER, vectores y código de SmallROM, ejecución real del MC68VZ328, traza de registros, comprobación NVRAM.
- [PASS] Determinación de bloqueo **antes de System 3 observado** para ambos sets y 24 s adicionales BOOT.
- [CONFIRMADO] El backend MMIO NEO carece de UART VZ en \`0xFFFFF900..0xFFFFF90D\`; Dana ya tiene un dispositivo reutilizable.
- [NO ACREDITADO] Arranque completo System 3, visibilidad/uso de AlphaWord, causalidad exclusiva de la UART, RTC/Flash/interrupts físicos.
- [RESTRICCIÓN] Se utilizaron **3 llamadas Commander** esta intervención (dos lanzamientos via cola NEO y una inspección privada); no más acciones en T bajo el límite vigente. No hay trabajo pendiente en la inbox después del cierre de ambas trazas.
