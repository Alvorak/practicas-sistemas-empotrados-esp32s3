# Prácticas unificadas de Sistemas Empotrados · ESP32-S3

Repositorio de estudio que reúne en un único proyecto ESP-IDF el material de las tres prácticas. Incluye teoría, montajes y ejercicios de programación para consultar, estudiar y probar en la placa. Cada ejercicio de programación tiene un archivo C completo, con su propio `app_main()`, que puedes **copiar entero a `main/main.c`, compilar y flashear**. No hay un menú de selección ni dependencias entre los archivos de ejercicios.


## 1. Abrir y ejecutar

1. Abre **la carpeta raíz de este proyecto** en VS Code; es la que contiene este README y el `CMakeLists.txt` principal.
2. Si el proyecto está en una ruta larga o con espacios, muévelo a una ruta corta, por ejemplo `C:\esp\practicas-sistemas-empotrados-esp32s3`.
3. Usa la extensión **Espressif IDF** y abre una terminal del entorno ESP-IDF que ya utilizas en clase. No hace falta crear otro proyecto encima.
4. Conecta la placa mediante el conector **USB-UART** para el monitor serie y el ejercicio de recepción UART. Selecciona tu puerto real.
5. Ejecuta, desde la raíz:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

`COM5` es un ejemplo: sustitúyelo por el puerto de tu placa. En Linux puede ser `/dev/ttyUSB0` y en macOS `/dev/cu.usbserial-...`. Para salir del monitor: `Ctrl+]`.

**El `main.c` inicial es la práctica 3, ejercicio 3.1:** el LED RGB integrado cambia rojo → verde → azul → blanco cada dos segundos. Se presupone el LED del montaje de clase en GPIO48. Si tu revisión de placa lo conecta a otro GPIO, modifica `LED_RGB_GPIO` en el ejemplo.

La primera compilación descarga el componente oficial `espressif/led_strip` **3.0.0**, declarado en `main/idf_component.yml`. Necesita conexión a Internet y el Component Manager del entorno ESP-IDF. No uses `pip` de otro Python para resolver problemas de esta instalación.

La API utilizada corresponde a ESP-IDF 5.3 o posterior; el manifiesto admite versiones anteriores a 7.0. Consulta [la validación realizada](docs/VALIDACION.md) para conocer la versión realmente compilada: el rango del manifiesto no equivale a una prueba en todas esas versiones.

## 2. Cambiar de ejercicio

Por ejemplo, para hacer el ejercicio 4.4 de la práctica 1:

1. Abre `practica1/ejercicio_4_4.c`.
2. Copia **todo** su contenido y reemplaza el contenido de `main/main.c`.
3. Guarda, revisa el montaje y la configuración del README de esa práctica.
4. Ejecuta `idf.py build` y `idf.py -p COM5 flash monitor`.

También puedes copiar el archivo desde PowerShell:

```powershell
Copy-Item .\practica1\ejercicio_4_4.c .\main\main.c
idf.py build
idf.py -p COM5 flash monitor
```

O desde Linux/macOS:

```bash
cp practica1/ejercicio_4_4.c main/main.c
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

El destino se sustituye: si has hecho cambios que quieras conservar, guárdalos antes en el ejercicio correspondiente o en Git. No añadas todos los `.c` a CMake: eso produciría varias definiciones de `app_main`. `main/CMakeLists.txt` ya incluye los componentes necesarios para todas las prácticas.

## 3. Dónde está cada cosa

| Ruta | Contenido |
|---|---|
| `main/main.c` | Único programa que compila el proyecto; inicialmente RGB de P3.3.1. |
| `main/CMakeLists.txt` | Selección explícita de `main.c` y componentes ESP-IDF. |
| `main/idf_component.yml` | Versión de ESP-IDF admitida y componente oficial `led_strip`. |
| [practica1/README.md](practica1/README.md) | Enunciados resumidos, GPIO, flancos, interrupciones, serie y JTAG. |
| [practica1/Teoria.md](practica1/Teoria.md) | Características, montaje, cálculos, respuestas y cuestiones teóricas. |
| [practica2/README.md](practica2/README.md) | ADC, GPTimer, medida de intervalos y PWM. |
| [practica2/Teoria.md](practica2/Teoria.md) | ADC, temporizadores, PWM, DAC y ampliación del servo. |
| [practica3/README.md](practica3/README.md) | PCNT, LED RGB, watchdog, deep-sleep y UART. |
| [practica3/Teoria.md](practica3/Teoria.md) | Encoders y explicación de los periféricos de la última práctica. |
| [docs/configuracion.md](docs/configuracion.md) | `menuconfig`, valores por defecto, consola y perfiles. |
| [docs/GUIA_EXAMEN.md](docs/GUIA_EXAMEN.md) | Guía complementaria de repaso y archivos importantes. |
| [docs/origen_y_cambios.md](docs/origen_y_cambios.md) | Correspondencia con las soluciones originales y decisiones. |
| [docs/VALIDACION.md](docs/VALIDACION.md) | Comprobaciones realizadas y pruebas pendientes en la placa. |

Los ejercicios teóricos conservan su numeración en cada README y se responden en `Teoria.md`. El ejercicio de depuración JTAG reutiliza el programa de la práctica 1; no necesita otro firmware distinto.

## 4. Montaje y ajustes que se repiten

| Elemento | GPIO / ajuste en estos ejemplos |
|---|---|
| BOOT | GPIO0, entrada con pull-up; pulsado = 0. |
| LED rojo externo de P1 | GPIO4, activo a nivel bajo, resistencia de 330 Ω. |
| LED verde externo de P1/P2 | GPIO5, activo a nivel alto, resistencia de 330 Ω. |
| Potenciómetro de P2 | Cursor a GPIO7 = ADC1, canal 6; extremos a 3,3 V y GND. |
| Señal del generador para PCNT de P3 | GPIO4; desconectar el LED externo de ese pin y compartir GND. |
| RGB integrado de P3 | GPIO48, WS2812, orden de componentes GRB. |
| UART de consola | UART0, 115200 baudios, 8N1; usar USB-UART. |
| FreeRTOS / CPU | Tick de 1000 Hz y CPU de 80 MHz. |

Los montajes son alternativos: revisa la sección de conexiones antes de cambiar de práctica. En los GPIO usa señales entre GND y 3,3 V; no apliques 5 V. Los LEDs externos llevan resistencia en serie.

**Al pasar de P2 a P3.4.1, reactiva el watchdog de tareas.** El PDF de P2 pide deshabilitar el watchdog, mientras que P3.4.1 lo necesita. El README de cada práctica y [configuración](docs/configuracion.md) explican el cambio. El archivo `sdkconfig` generado conserva tus ajustes aunque después cambies de ejercicio.