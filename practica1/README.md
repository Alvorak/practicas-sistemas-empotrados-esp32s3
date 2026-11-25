# Práctica 1: GPIO, interrupciones y depuración

Resumen de los ejercicios del enunciado **Práctica 1, curso 26-27**, y adaptación de las soluciones de `Alvorak/SistemasEmpotrados`. La numeración coincide con el PDF. Las respuestas escritas están en [Teoria.md](Teoria.md).

## Cómo ejecutar

Abre la **raíz del repositorio** en VS Code. Copia **todo** el archivo del ejercicio elegido a `main/main.c`, sustituyendo su contenido. Compila y flashea desde ese proyecto. No añadas todos los ejercicios a `SRCS`: cada uno tiene su propio `app_main()` y solo debe compilarse uno.

Ejemplo desde la raíz, en una terminal ESP-IDF de PowerShell:

```powershell
Copy-Item .\practica1\ejercicio_4_6.c .\main\main.c
idf.py build
idf.py -p COM7 flash monitor
```

Sustituye `COM7` por tu puerto. La configuración inicial del proyecto se explica en el [README principal](../README.md).

## Inventario completo

| Apartado | Qué se pide | Archivo o guía | Página del PDF |
| --- | --- | --- | --- |
| 2.1 | Características del ESP32-S3 y sus memorias | [Teoria.md](Teoria.md#21-características-del-esp32-s3) | 4 |
| 2.2 | Nombrar al menos cinco periféricos y explicar su función | [Teoria.md](Teoria.md#22-periféricos) | 5 |
| 2.3 | Identificar BOOT, reset, LED RGB, otros LEDs, circuitos y tensiones | [Teoria.md](Teoria.md#23-elementos-de-la-placa) | 5 |
| 2.4 | Identificar y explicar los puentes de soldadura | [Teoria.md](Teoria.md#24-jumpers-o-puentes-de-soldadura) | 5 |
| 3 | Preparar ESP-IDF y probar el programa inicial, sin número de ejercicio | [ejemplo_3_arranque.c](ejemplo_3_arranque.c) | 6 |
| 4.1 | Nivel de una entrada con pull-up sin pulsar | [Teoria.md](Teoria.md#41-entrada-con-pull-up) | 8 |
| 4.2 | Calcular las resistencias de los LEDs | [Teoria.md](Teoria.md#42-resistencias-de-los-leds) | 11 |
| 4.3 | Elegir la configuración de BOOT | [Teoria.md](Teoria.md#43-configuración-de-boot) | 11 |
| 4.4 | Parpadeo de GPIO4 cada 100 ms y GPIO5 según BOOT | [ejercicio_4_4.c](ejercicio_4_4.c) | 12 |
| 4.5 | Aumentar la espera a 3 s y estudiar el retardo al leer BOOT | [ejercicio_4_5.c](ejercicio_4_5.c) y [respuesta](Teoria.md#45-por-qué-falla-la-respuesta-del-botón) | 12 |
| 4.6 | Detectar flanco de bajada por polling y alternar GPIO5 | [ejercicio_4_6.c](ejercicio_4_6.c) | 12 |
| 5.1 | GPIO4 cada 3 s y respuesta de GPIO5 por interrupción | [ejercicio_5_1.c](ejercicio_5_1.c) | 13 |
| 6.1 | Añadir número de pulsación y timestamp en microsegundos | [ejercicio_6_1.c](ejercicio_6_1.c) | 14 |
| 6.2 | Depurar un ejercicio anterior mediante USB Serial/JTAG | Instrucciones en esta página; reutiliza 4.6 o 6.1 | 18 |

El PDF menciona un apartado 8 en su introducción, pero las 18 páginas aportadas terminan en el ejercicio 6.2. No se inventa un ejercicio 8.

## Montaje común

| Elemento | Pin y conexión | Nivel que lo activa |
| --- | --- | --- |
| LED rojo del PDF, blanco en tu repositorio original | `3V3 -> resistencia -> ánodo LED`; cátodo a GPIO4 | `0` |
| LED verde del PDF, azul en tu repositorio original | `GPIO5 -> resistencia -> ánodo LED`; cátodo a GND | `1` |
| BOOT integrado | GPIO0 como entrada con pull-up; el pulsador conecta a GND | `0` |
| Masa | Todos los elementos comparten GND | — |

Cada LED lleva su propia resistencia en serie. Para los valores de diseño del PDF, el cálculo da **320 Ω** y se escoge **330 Ω**. El color físico no cambia la lógica del programa, pero sí puede cambiar la caída de tensión del LED y, por tanto, el cálculo real de corriente. Usa los valores de tu componente si recalculas el montaje.

Monta sin alimentación y revisa polaridades. Las resistencias de pull-up internas de las entradas **no sustituyen** las resistencias limitadoras de los LEDs. Mantén BOOT suelto durante un reinicio normal: GPIO0 también interviene en la selección del modo de arranque.

## Configuración adicional

Para ejecutar los ejercicios 3 a 6.1 no hace falta desactivar el watchdog ni activar PSRAM. El proyecto raíz aporta las dependencias. Esta tabla sirve para entender qué utiliza cada archivo:

| Archivos | Componentes de ESP-IDF usados |
| --- | --- |
| Ejemplo 3 | FreeRTOS y biblioteca estándar de C |
| 4.4, 4.5 y 4.6 | `esp_driver_gpio`, FreeRTOS |
| 5.1 y 6.1 | `esp_driver_gpio`, `esp_timer`, FreeRTOS |

Trabaja con `CONFIG_FREERTOS_HZ=1000`, incluido en la configuración base. Los tiempos se expresan mediante `pdMS_TO_TICKS()`, no mediante un número de ticks supuesto. El terminal serie usa **115200 baudios**. En los programas que imprimen mensajes, selecciona el puerto que corresponde al conector UART de tu placa.

El antirrebote de 5.1 incorpora una tarea y notificaciones de FreeRTOS; 6.1 conserva la cola y la tarea de terminal que ya usaba tu solución original. Son mecanismos del sistema operativo incluido en ESP-IDF, no componentes externos. Están comentados en los archivos.

## Ejercicio 4.4: salida y entrada digitales

**Objetivo:** hacer que GPIO4 alterne entre encendido y apagado, permaneciendo 100 ms en cada estado, mientras GPIO5 refleja si BOOT está pulsado. Se utiliza `vTaskDelay()` como pide el enunciado.

**Qué observar:** GPIO4 parpadea; GPIO5 se enciende al mantener BOOT y se apaga al soltarlo. Como la lectura se realiza cada 100 ms, una pulsación más corta puede no verse. El periodo completo es aproximadamente 200 ms, es decir, 5 Hz.

**Qué estudiar:** dirección de los GPIO, pull-up de BOOT, polaridad de los dos LEDs y conversión de milisegundos a ticks.

## Ejercicio 4.5: espera de tres segundos

**Objetivo:** repetir 4.4 con 3000 ms entre cambios de GPIO4 y comprobar lo que ocurre con BOOT.

**Qué observar:** GPIO4 permanece unos 3 s en cada estado. Pulsa y suelta BOOT durante una de esas esperas: GPIO5 puede no reaccionar. Si mantienes BOOT, GPIO5 lo detectará en la siguiente lectura.

El retraso se conserva deliberadamente: es lo que pide estudiar el ejercicio. El PDF escribe `pdMS_TO_TICKS(1000)` junto a «3 segundos»; el código usa **3000**. Las posibles mejoras se explican en [Teoria.md](Teoria.md#45-por-qué-falla-la-respuesta-del-botón).

## Ejercicio 4.6: detección del flanco por polling

**Objetivo:** alternar GPIO5 una sola vez por pulsación, detectando el paso de `1` a `0`. Soltar BOOT no cambia el LED.

**Qué observar:** pulsa, mantén durante varios segundos, suelta y vuelve a pulsar. El LED cambia al pulsar y conserva el estado mientras mantienes o sueltas el botón. GPIO4 no forma parte de este ejercicio.

La versión original compara directamente la lectura actual con la anterior. Aquí se añade un filtro sencillo: una transición debe mantenerse 30 ms para considerarse estable, tanto al pulsar como al soltar. El bucle sigue teniendo la pausa de 10 ms indicada en el PDF. Para el examen, primero comprende `anterior == 1 && actual == 0`; después explica por qué se filtran los contactos mecánicos.

## Ejercicio 5.1: interrupciones

**Objetivo:** mantener el parpadeo de GPIO4 cada 3 s con `vTaskDelay()` y cambiar GPIO5 cuando BOOT produce una interrupción de flanco de bajada.

**Qué observar:** pulsa BOOT en diferentes momentos de los 3 s. GPIO5 cambia sin esperar a que termine esa pausa. Mantener el botón no produce cambios adicionales.

La ISR hace tres acciones breves: deshabilita temporalmente la interrupción de BOOT, cambia GPIO5 y avisa a una tarea. Esa tarea espera a que BOOT lleve 30 ms suelto y vuelve a habilitar la interrupción. Así se evitan rebotes al pulsar y al soltar, sin introducir una espera dentro de la ISR ni retrasar la primera respuesta del LED. Esta es una política de **una pulsación por ciclo pulsar-soltar**; no pretende contar pulsos eléctricos rápidos.

`IRAM_ATTR` sitúa la función en RAM de instrucciones, siguiendo la notación del laboratorio. Se instala el servicio con `gpio_install_isr_service(0)`, como en el PDF; no se solicita `ESP_INTR_FLAG_IRAM`. Marcar solo la función no convierte todas sus llamadas en una cadena completa segura con la caché deshabilitada.

## Ejercicio 6.1: contador y timestamp por UART

**Objetivo:** añadir a 5.1 un mensaje por pulsación con su número y el tiempo en microsegundos desde la inicialización de ESP Timer en ese arranque, que es la referencia de tiempo usada en la práctica.

**Qué observar:** los números aumentan de uno en uno, el timestamp crece y GPIO4 sigue parpadeando. Una línea esperada tiene este formato; los números concretos dependen de cuándo pulses:

```text
BOOT # 1 | t = 2345678 us | GPIO5 = 1
BOOT # 2 | t = 5210000 us | GPIO5 = 0
```

La ISR obtiene el timestamp con `esp_timer_get_time()` y deposita una copia del evento en una cola mediante `xQueueSendFromISR()`. La tarea recibe el evento y llama a `printf()`. El dato impreso corresponde al instante atendido por la ISR, no al momento posterior en que el terminal dibuja la línea. Contador y estado del LED solo se modifican en la ISR; la cola evita leer un timestamp de 64 bits mientras está cambiando.

## Ejercicio 6.2: depuración con USB Serial/JTAG

El enunciado pide depurar **un programa de los anteriores**, por lo que no se necesita otro `app_main()` ni un archivo C duplicado.

1. Copia, por ejemplo, `ejercicio_4_6.c` a `main/main.c`.
2. Selecciona el objetivo `esp32s3` y compila. Para facilitar la depuración, en `idf.py menuconfig` elige **Compiler options -> Optimization Level -> Debug (-Og)** cuando esa opción esté disponible; su símbolo es `CONFIG_COMPILER_OPTIMIZATION_DEBUG`.
3. Flashea por el puerto UART, como en los demás ejercicios. Conecta un cable USB de datos al conector nativo **USB Serial/JTAG** de la placa. Identifica los conectores por su serigrafía, no por izquierda/derecha: la orientación y la revisión cambian.
4. En la extensión ESP-IDF de VS Code configura el dispositivo ESP32-S3 y la opción de depuración **Built-in USB-JTAG**. Si Windows no reconoce la interfaz JTAG, instala el controlador indicado por la guía de depuración de Espressif; no cambies a ciegas el controlador de tu puente UART.
5. Inicia el servidor OpenOCD desde la extensión y después la sesión de depuración. Usa la configuración generada para tu instalación; no copies rutas de otro ordenador.
6. Pon un breakpoint en el bloque que modifica `estado_led`. Pulsa BOOT y comprueba `lectura`, `estado_estable` y `estado_led`. Avanza una línea, reanuda y prueba otra pulsación.
7. Como ampliación, crea un breakpoint condicional para detenerte solo cuando `estado_led == 1`.

En 6.1 las variables del evento están en la tarea de terminal: también puedes poner ahí un breakpoint. Detener una ISR durante mucho tiempo altera la temporización; para aprender a inspeccionar variables es más cómodo empezar por 4.6. Al terminar de depurar, vuelve a compilar con la configuración que requiera el ejercicio siguiente.

**Restricción de pines:** no uses GPIO19 ni GPIO20 para otros periféricos mientras depuras por el USB nativo. En este montaje son las líneas USB. Véanse el PDF, páginas 15-18, y la [guía oficial de JTAG para ESP32-S3](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-guides/jtag-debugging/index.html).

## Qué se ha conservado y qué se ha revisado

Se mantienen las funciones GPIO de clase, la progresión polling -> interrupciones -> cola para el terminal y las polaridades del repositorio original. Se han reducido repeticiones, uniformado nombres, añadido comprobaciones de error y antirrebote, y separado las respuestas teóricas.

Los LEDs blanco/azul de tu montaje se llaman rojo/verde en estos archivos para coincidir con el enunciado. No es necesario cambiar los LEDs físicos. El cálculo de 330 Ω responde a los datos del PDF, no a una medición inventada de los LEDs blanco y azul.

Fuentes de las soluciones previas: `Practicas/Practica1/ejercicio-4-gpio/4.4-led-y-boot/main.c`, `4.5-delay-3-segundos/main.c`, `4.6-flanco-bajada/main.c`, `ejercicio-5-interrupciones/main.c` y `Ejercicio6/Ejercicio6.1.c`, además de sus README y resultados, en el [repositorio original](https://github.com/Alvorak/SistemasEmpotrados/tree/0215954584839e431d32918761647f8f2685cafa/Practicas/Practica1).
