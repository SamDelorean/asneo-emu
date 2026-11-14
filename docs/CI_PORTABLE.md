# Pruebas automáticas C++ (sin T ni ROM privadas)

- `.github/workflows/component-ci.yml` compila y ejecuta `tests/neo_components.cpp` y `tests/neo_vz_core.cpp` en runners Linux de GitHub con GCC y Clang, C++17, `-Werror`.
- Ejecuta sólo código fuente publicado, sin firmware, paquetes propietarios, Ghidra o periféricos.
- El CI mide consistencia de componentes autónomos, **no** sintaxis del driver `alphasneo.cpp` frente al MAME fijado ni compatibilidad con hardware NEO; éstos requieren una compilación MAME real.
- No se declarará PASS hasta que exista una ejecución de GitHub Actions con resultado comprobado.
