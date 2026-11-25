# Práctica 2: ADC, temporizadores y PWM

Cada archivo `.c` es un programa completo. Copia el ejercicio que quieras ejecutar a `main/main.c`, en la raíz del proyecto. **Solo se compila ese `main.c`**; los otros ejercicios se conservan como material de estudio.

Los enunciados siguientes son resúmenes del PDF de la práctica 2 del curso 26-27, con su numeración original. Las respuestas y cálculos están en [Teoria.md](Teoria.md).

## Índice de ejercicios

| Ejercicio | Qué pide | Archivo | Página del PDF |
|---|---|---|---|
| 2.1 | Leer un potenciómetro en GPIO7, promediar dos muestras y mostrar resultado con timestamp cada 100 ms. | [ejercicio_2_1.c](ejercicio_2_1.c) | 7 |
| 3.1 | Generar 1 Hz en GPIO4 y 120 Hz en GPIO5 usando GPTimer y callbacks; comprobar con osciloscopio. | [ejercicio_3_1.c](ejercicio_3_1.c) | 11 |
| 3.2 | Indicar cuántos temporizadores de propósito general independientes tiene el ESP32-S3. | [Teoria.md](Teoria.md#ejercicio-32-cuantos-temporizadores-hay) | 11 |
| 3.3 | Medir el intervalo entre pulsaciones válidas consecutivas de BOOT, en milisegundos; empezar a mostrar intervalos con la segunda pulsación. | [ejercicio_3_3.c](ejercicio_3_3.c) | 12 |
| 4.1 | Generar PWM de 100 Hz en GPIO5 y recorrer 0, 20, 40, 60, 80 y 100 % al pulsar BOOT; la siguiente pulsación vuelve a 0 %. | [ejercicio_4_1.c](ejercicio_4_1.c) | 15 |
| Ampliación 4 | Investigar periodo y anchuras de pulso del servomotor concreto que se utilice. Es voluntaria. | [Teoria.md](Teoria.md#ampliacion-voluntaria-servomotor-rc) | 15 |
| Apartado 5 | Reflexionar sobre alternativas al DAC, que el ESP32-S3 no incorpora. | [Teoria.md](Teoria.md#apartado-5-dac-y-alternativas) | 16 |

## Configuración de ESP-IDF para esta práctica

El PDF, página 3, pide tick de FreeRTOS a **1000 Hz**, CPU a **80 MHz** y deshabilitar el watchdog. La base común del repositorio ya fija los dos primeros valores. Revisa la [guía de configuración](../docs/configuracion.md) si has reutilizado un `sdkconfig` anterior.

| Ajuste | Valor de la práctica | Dónde localizarlo en `idf.py menuconfig` |
|---|---|---|
| `CONFIG_FREERTOS_HZ` | `1000` | Component config → FreeRTOS → Kernel → FreeRTOS tick rate (Hz). |
| `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ` | `80` | Component config → ESP System Settings → CPU frequency. |
| `CONFIG_ESP_TASK_WDT_EN` | Desactivado para P2. | Component config → ESP System Settings → Enable Task Watchdog Timer. |
| Consola | UART0, 115200 baudios. | Component config → ESP System Settings → Channel for console output / UART console baud rate. |

La ubicación visual de una opción puede variar entre versiones: usa `/` en `menuconfig` para buscar el nombre del símbolo. La versión de referencia del proyecto y sus componentes se explican en el README de la raíz.

**Interpretación del watchdog:** el PDF dice «deshabilita el Watchdog», sin identificar cuál ni mostrar una captura. El perfil de P2 desactiva el **watchdog de tareas (TWDT)** y conserva el de interrupciones (IWDT). Esto es una elección explícita para el laboratorio, no una afirmación sobre una configuración que el PDF no concreta. Estos programas no necesitan bucles que monopolicen la CPU. Antes de hacer P3, vuelve a activar el TWDT como explica su README.

### Forma habitual: el mismo proyecto de VS Code

Ejecuta desde la raíz, en una terminal con el entorno ESP-IDF cargado. Ejemplo en PowerShell:

```powershell
Copy-Item .\practica2\ejercicio_2_1.c .\main\main.c
idf.py menuconfig
idf.py build
idf.py -p COM8 flash monitor
```

Cambia `COM8` por tu puerto real. En `menuconfig` comprueba los valores anteriores. Cambiar de ejercicio solo requiere copiar otro `.c`; el `CMakeLists.txt` común ya incluye los componentes necesarios.

### Opción: conservar la configuración de P2 separada

Para generar por primera vez un `sdkconfig.p2` independiente, la raíz incluye `config/sdkconfig.practica2.defaults`:

```powershell
idf.py -B build_p2 -D SDKCONFIG=sdkconfig.p2 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.practica2.defaults" build
idf.py -B build_p2 -D SDKCONFIG=sdkconfig.p2 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.practica2.defaults" -p COM8 flash monitor
```

Los archivos `defaults` inicializan configuraciones nuevas; **no sobreescriben automáticamente** un `sdkconfig.p2` ya creado. Si existe y quieres cambiarlo, conserva los mismos argumentos y sustituye `build` por `menuconfig`. El código sigue tomándose de `main/main.c`. Al usar esta opción, conserva también esa selección al compilar y flashear; la compilación normal de VS Code usa `build`.

## Conexiones y componentes

Conecta y modifica el montaje con la placa desconectada de la alimentación.

| Elemento | Conexión |
|---|---|
| Potenciómetro, extremo 1 | 3V3. |
| Potenciómetro, cursor central | GPIO7, entrada `ADC1` / `ADC_CHANNEL_6`. |
| Potenciómetro, extremo 3 | GND. |
| LED de GPIO5 | GPIO5 → resistencia limitadora → ánodo; cátodo → GND. Activo a nivel alto. |
| LED de GPIO4, si se mantiene el montaje de P1 | 3V3 → resistencia → ánodo; cátodo → GPIO4. Activo a nivel bajo. |
| BOOT integrado | GPIO0, entrada con pull-up; pulsado equivale a 0. |
| Osciloscopio | Punta a GPIO4 o GPIO5 según el ejercicio; masa a GND de la placa. |

Usa las resistencias calculadas en P1: con sus datos, el valor comercial de 330 Ω es adecuado. La entrada ADC se alimenta desde **3V3**, nunca desde 5 V. No confundas el rango útil de medida del ADC con los límites eléctricos del pin.

| Archivo | Componentes utilizados |
|---|---|
| `ejercicio_2_1.c` | `esp_adc`, `esp_timer`, FreeRTOS. |
| `ejercicio_3_1.c` | `esp_driver_gpio`, `esp_driver_gptimer`, FreeRTOS. |
| `ejercicio_3_3.c` | `esp_driver_gpio`, `esp_timer`, FreeRTOS. |
| `ejercicio_4_1.c` | `esp_driver_gpio`, `esp_driver_ledc`, `esp_timer`, FreeRTOS. |

Son componentes oficiales de ESP-IDF y los drivers tratados en clase. No hay componentes externos en esta práctica. `ESP_ERROR_CHECK` comprueba errores de inicialización y ejecución; no sustituye la comprobación del montaje físico.

## Ejercicio 2.1: potenciómetro y ADC

**Enunciado resumido.** Conecta el cursor del potenciómetro a GPIO7, configura `ADC_BITWIDTH_DEFAULT` y `ADC_ATTEN_DB_12`, promedia dos muestras y muestra el resultado y un timestamp cada 100 ms. [Fuente: PDF, p. 7.]

La solución conserva el comportamiento del C original: toma una muestra inicial y después una cada 100 ms. Cada resultado es la media de la muestra anterior y la actual. Por tanto, es una **media móvil de dos muestras separadas aproximadamente 100 ms**. El README antiguo describía dos conversiones consecutivas; aquí la documentación coincide con el C. Ambas interpretaciones son posibles porque el PDF no exige una separación concreta entre las dos muestras.

`xTaskDelayUntil` mantiene la cadencia respecto a un instante anterior, evitando sumar deliberadamente el tiempo de `printf` al periodo. Sigue existiendo variación por el planificador. La marca `esp_timer_get_time()` se captura tras la conversión actual y está en microsegundos. La media se imprime con un decimal para conservar valores como `2040.5`.

**Comprobación:** gira lentamente el potenciómetro. Los códigos deben cambiar y las marcas temporales de resultados consecutivos deben separarse alrededor de 100 000 µs. La salida es **ADC crudo**, no una tensión calibrada. Promediar reduce ciertas fluctuaciones pero no elimina errores sistemáticos; el extremo alto puede saturar antes de representar exactamente 3,3 V.

## Ejercicio 3.1: dos señales mediante GPTimer

**Enunciado resumido.** Genera 1 Hz en GPIO4 y 120 Hz en GPIO5 con callbacks de GPTimer. El `while (1)` no puede generar las señales: solo debe incluir una espera. Comprueba especialmente GPIO5 con el osciloscopio. [Fuente: PDF, p. 11.]

La solución utiliza dos temporizadores a 1 MHz. Cada callback invierte su salida, por lo que una onda completa requiere **dos alarmas**.

| Salida | Alarma | Periodo de la onda | Frecuencia nominal resultante |
|---|---:|---:|---:|
| GPIO4 | 500 000 ticks = 500 ms. | 1 s. | 1 Hz. |
| GPIO5 | 4 167 ticks = 4,167 ms. | 8,334 ms. | 119,9904 Hz. |

El semiperiodo ideal de 120 Hz es 4 166,666… µs. Redondearlo a 4 167 µs introduce aproximadamente −0,008 % de error; es la misma resolución del código original. La tolerancia del reloj y la latencia de interrupción también afectan a la medida física. Ningún cálculo de este documento sustituye una medida con osciloscopio.

**Comprobación:** mide un periodo completo entre dos flancos de subida, no entre un flanco de subida y el siguiente de bajada. Debes observar aproximadamente 1 s y 8,334 ms, con un ciclo de trabajo cercano al 50 %. Los callbacks no hacen `printf` ni `vTaskDelay`. `return false` indica que no se ha despertado una tarea de mayor prioridad.

## Ejercicio 3.3: intervalo entre pulsaciones

**Enunciado resumido.** Muestra en milisegundos el tiempo entre dos pulsaciones válidas consecutivas de BOOT. No muestres un intervalo hasta la segunda pulsación. [Fuente: PDF, p. 12.]

La solución consulta BOOT cada 5 ms y solo acepta un cambio después de 30 ms estables. Filtra tanto pulsación como liberación. Guarda el instante del flanco observado y calcula la diferencia con la pulsación anterior. Mantener el botón pulsado no genera pulsaciones nuevas; hay que soltarlo de forma estable.

Se ha simplificado la solución original de interrupción, cola y tarea adicional porque el enunciado no exige interrupciones. **La resolución en microsegundos del reloj no implica precisión de microsegundos en la pulsación:** el sondeo añade incertidumbre del orden de su periodo, además de la planificación y el rebote. Para estudiar timestamps capturados en ISR, revisa P1 6.1.

**Comprobación:** con la primera pulsación aparece un mensaje sin intervalo. Pulsa otra vez después de aproximadamente un segundo: debe aparecer un valor cercano a 1000 ms. Mantén BOOT varios segundos y comprueba que solo cuenta una pulsación. Los tres decimales de la impresión son un formato, no una garantía de exactitud física.

## Ejercicio 4.1: PWM de 100 Hz

**Enunciado resumido.** En GPIO5 genera PWM de 100 Hz, inicialmente al 0 %. Cada pulsación válida de BOOT incrementa 20 puntos porcentuales hasta 100 %; después vuelve a 0 %. Mide el resultado con el osciloscopio. [Fuente: PDF, p. 15.]

Se configura primero el temporizador LEDC y después su canal. El ESP32-S3 utiliza `LEDC_LOW_SPEED_MODE`. La resolución es de 10 bits: un periodo tiene 1024 posiciones de cuenta. Los valores intermedios se redondean al entero más cercano.

| Pulsaciones desde el arranque | Porcentaje solicitado | Salida nominal |
|---:|---:|---|
| 0 | 0 % | Nivel bajo. |
| 1 | 20 % | Duty 205/1024; aproximadamente 2 ms alto de cada 10 ms. |
| 2 | 40 % | Duty 410/1024; aproximadamente 4 ms alto. |
| 3 | 60 % | Duty 614/1024; aproximadamente 6 ms alto. |
| 4 | 80 % | Duty 819/1024; aproximadamente 8 ms alto. |
| 5 | 100 % | Nivel alto permanente. |
| 6 | 0 % | Vuelve a nivel bajo. |

Para el 100 % se usa `ledc_stop(..., 1)`: detiene la conmutación dejando el nivel alto. Es una función del mismo driver LEDC y se explica en el C. Al volver a 0 %, `ledc_set_duty` y `ledc_update_duty` reactivan la salida. Esto evita llamar «100 % exacto» a `1023/1024`. El valor `1024` sí puede representar el 100 % en algunas resoluciones admitidas; no debe extrapolarse sin comprobar los límites del chip. [Referencia: documentación LEDC de Espressif enlazada abajo.]

**Comprobación:** a 20-80 % el periodo debe ser aproximadamente 10 ms. Al 0 y 100 % la señal es constante: el osciloscopio puede dejar de mostrar una frecuencia medible, lo cual es normal. Una pulsación mantenida no debe aumentar repetidamente el duty.

## Cambios respecto al repositorio original

| Ejercicio | Qué se conserva | Qué se revisa |
|---|---|---|
| 2.1 | GPIO7, ADC1 canal 6, atenuación 12 dB, media móvil de dos muestras. | Comprobación de errores, temporización periódica y README coherente con el algoritmo real. |
| 3.1 | Dos GPTimer a 1 MHz, alarmas 500 000 y 4 167 ticks. | Inicialización de GPIO comprobada, argumentos de callback explícitamente no usados y frecuencia de redondeo documentada. |
| 3.3 | BOOT, `esp_timer_get_time`, diferencia entre pulsaciones desde la segunda. | Sondeo sencillo con antirrebote estable; se elimina el timestamp tardío tomado al procesar una cola. |
| 4.1 | LEDC 100 Hz, GPIO5, 10 bits, secuencia de seis estados. | Antirrebote en ambas transiciones, redondeo del duty y 100 % alto permanente explícito. |

No se han mezclado varios `app_main` en una misma compilación. La verificación del proyecto unificado y sus limitaciones están documentadas en la raíz del repositorio.

## Material para entregar o practicar

El PDF exige un vídeo por ejercicio con el nombre de los integrantes. En 3.1 y 4.1 debe verse también el osciloscopio. Para estudiar, explica en voz alta el cableado, el orden de inicialización, las unidades de cada constante y el resultado esperado antes de flashear.

## Fuentes

- Enunciado aportado: **Práctica 2, Sistemas Empotrados, Francisco Javier Burgoa, curso 26-27**, páginas 3-16. Los PDF no se incluyen en este repositorio.
- Soluciones originales revisadas: [Alvorak/SistemasEmpotrados, commit `0215954584839e431d32918761647f8f2685cafa`, carpeta Practica2](https://github.com/Alvorak/SistemasEmpotrados/tree/0215954584839e431d32918761647f8f2685cafa/Practica2).
- Espressif: [ADC oneshot](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/adc_oneshot.html), [GPTimer](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/gptimer.html) y [LEDC](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/ledc.html).
- [Fuente del driver LEDC, ESP-IDF v5.5.1](https://github.com/espressif/esp-idf/blob/v5.5.1/components/esp_driver_ledc/src/ledc.c), revisada para comprobar la reactivación del canal tras `ledc_stop`.
