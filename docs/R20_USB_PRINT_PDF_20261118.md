# R20 — Impresión USB NEO/NEO2: INT PTD y estado de control; PDF pendiente
Fecha: 18 de noviembre de 2026. Producto: **único emulador `neo-diag`**, no un documento PDF. El formato de salida requerido es un PDF generado por el flujo Print real de AlphaWord.

## Binarios y resultados
- Ejecución nativa `NEO_DIAG_R20_PRINT_PDF_FINAL_20261118`: **PARTIAL / END_PARTIAL / exit 0**. [Estado](https://drive.google.com/file/d/13bqmhwHNYNvaYyS96nvMj__LudQ25YcI/view), [resultado](https://drive.google.com/file/d/1_LYCVy61eSfABAn9G9BZn-Cz7znHw9CM/view).
- MAME integral GCC: **PASS**. ROM BOOT/WRITER: **PASS**. Nueve pruebas independientes (GCC/Clang): **PASS**. CI <https://github.com/SamDelorean/asneo-emu/actions/runs/44350739724>.
- Código de la ejecución nativa: `5bd7dcd817a7f0e5a6c203f760e3f361b4fd4b91`.
- Instalación canónica en T640 **conservada R17**: `/home/spc/Projects/alphasmart/neo-diag/neo-diag`, SHA-256 `d06a96325657112354172fa82ac96c8fcdd8828e9beaf5286ea23562901eff35`.
- Candidato R20 compilado como `neodiag`; **NO instalado**. Pruebas de impresión de los perfiles 1 (hub genérico), 3 (hub PCL) y 0 (impresora directa) produjeron **0 bytes de datos de impresora**. **No existe un PDF de salida**, y no debe fabricarse uno.
- Existen en GitHub `scripts/neo-diag-pdf.sh` y un proceso de conversión Ghostscript/PDF dentro de la validación. Sólo convierten bytes realmente recibidos; su instalación y aceptación queda condicionada a obtener PostScript/PCL/PDF del firmware.

## Hechos nuevos: protocolo original
El trabajo `NEO_DIAG_R18_USB_PACKET_FORENSICS_20261118` recogió el registro nativo de paquetes MAME con `-log` para las topologías directa y hub, antes ausente. 
- [Resultado forense](https://drive.google.com/file/d/1Trop_25MrvuEZMoKhtMGnPmqeikZRvLJ/view).
- [Paquetes R18 hub](https://drive.google.com/file/d/1Fy-05XnN2hSN8-JQDZShls_RgYEG3uz8/view).
- [Paquetes R19/R20 nativos](https://drive.google.com/file/d/1_LYCVy61eSfABAn9G9BZn-Cz7znHw9CM/view).

El firmware NEO13 configuró el hub con `00 09 01 00 00 00 00 00` (**SET_CONFIGURATION**) y acto seguido utilizó **PTD de estado OUT de longitud cero**, no IN. La función USB de Dana sólo confirmaba configuración con IN: `configuration` permanecía en cero. Se corrigió la aceptación de la secuencia **observada** sin confirmar durante SETUP ni alterar ROM, y se agregó prueba de regresión. En R20 el hub devuelve ACK con un byte en su endpoint de interrupción (evento de puerto) donde R19 devolvía NAK.

También se detectó que `neo::isp1763` sólo ejecutaba PTD ATL (`0x0c00`), aunque NEO configura también PTD INT (`0x0800`). Se incorporó motor de paquetes periódicos `service_interrupt`, registros `INT_DONE=0xaa`, `INT_SKIP=0xac`, `INT_LAST=0xae`, interrupción HCINT `0x0080`, reintento tras NAK, transferencia de SRAM y W1C. Regresión de hub: `tests/neo_usb_int.cpp`.

## Bloqueo exacto tras los cambios
En R20 hub/generic y hub/PCL, el firmware llega a:
1. Detectar y direccionar el hub como dispositivo 2.
2. Leer descriptores y ejecutar `SET_CONFIGURATION`.
3. Alimentar los dos puertos, consultar estado del puerto 2, y **recibir ACK/byte por la interrupción del hub (endpoint 1)**.
4. Limpiar el cambio y emitir **PORT_RESET del puerto 2**, con el hub informando `port2=0x0111`.

**Después de la fase de reset, no inicia enumeración del dispositivo de impresión downstream**. Las trazas R20 muestran **49 paquetes** (sin solicitudes adicionales a dirección 0 del dispositivo hijo), y `PRN_BYTES=0`. El motor USB del hub completa el reset tras 10 ms y su estado cambia a `port2=0x0103`, pero la aplicación no inicia las solicitudes `GET_DESCRIPTOR` de la impresora. La siguiente frontera real es el **ciclo de interrupción periódico/postreset y la notificación de cambio del puerto 2**, no configurar ficticiamente una impresora ni editar OS3K.

## Contraindicaciones y trabajo que NO debe rehacerse
- No reconstruir CPU/VZ, teclado, LCD, NOR, RAM, R11–R17; no repetir los 4 perfiles sin una corrección dirigida.
- No usar un PDF informativo como sustituto de la impresión de AlphaWord.
- No implementar Wi-Fi/RF; el NEO2 sólo monitorea ese hardware por alcance.
- No instalar R20 sin el flujo original `PRINT→bytes de impresora→PDF validado`.
- Última intervención respetó exactamente **3 llamadas a Commander** para dos campañas: traza, prueba INT y prueba del estado de configuración. Sin trabajos en segundo plano prometidos ni nuevos lanzamientos.
