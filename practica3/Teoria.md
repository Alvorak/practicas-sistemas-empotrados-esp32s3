# Práctica 3: teoría y preparación del examen

El **ejercicio 2.1 es teórico**. El PDF también pide consultar RMT como trabajo previo al LED. El resto de este documento explica los conceptos que necesitas para defender los programas; no presenta mediciones de una placa que no se han realizado.

Fuentes de partida: enunciado **Práctica 3, curso 26/27** y soluciones previas revisadas para este proyecto. Las páginas indicadas se refieren a ese enunciado. Los enlaces oficiales al final aclaran los detalles de las APIs.

## Ejercicio 2.1: encoder incremental

**Pregunta:** ¿por qué hay dos señales A y B y qué información da su desfase? [Enunciado, p. 4.]

Un encoder incremental genera pulsos al desplazarse. Contar los pulsos de una sola señal permite medir movimiento relativo y, al relacionar el número con el tiempo, velocidad. Sin otra información, esa señal por sí sola no distingue los dos sentidos.

Las señales A y B suelen estar en **cuadratura**, desfasadas un cuarto de periodo eléctrico, es decir, 90°. Si A se adelanta a B se recorre una secuencia de estados; al invertir el movimiento se recorre la secuencia contraria. El sentido que se denomina «horario» depende del montaje y de qué cable se llame A: no debe memorizarse como una convención universal.

| Orden de estados `(A,B)` | Interpretación |
| --- | --- |
| `00 → 10 → 11 → 01 → 00` | A se adelanta a B; un sentido de movimiento. |
| `00 → 01 → 11 → 10 → 00` | B se adelanta a A; el sentido contrario. |

Si se cuentan solo las subidas de A, se obtiene una cuenta por ciclo de A (*x1*). Si se cuentan sus dos flancos, dos (*x2*). Contando ambos flancos de A y B, cuatro (*x4*). Por eso debes aclarar si el fabricante expresa pulsos por vuelta o cuentas por vuelta y qué modo de decodificación estás utilizando.

Si `P` significa ciclos de A por vuelta, `k` es 1, 2 o 4, `N` son las cuentas durante un intervalo de `Δt` segundos:

```text
vueltas relativas = cuentas acumuladas / (k · P)
rpm = 60 · N / (k · P · Δt)
```

El signo de la cuenta puede representar el sentido. Un encoder incremental no da por sí solo la posición absoluta tras perder alimentación; hay que referenciar la posición o conservar/recuperar una referencia. Algunos incorporan una señal Z de índice, pero la existencia de A y B no implica que tengan Z.

## PCNT: lo que debes poder explicar

El ESP32-S3 dispone de **cuatro unidades PCNT y dos canales por unidad**, según el apartado 2 del laboratorio. Una unidad tiene un contador; sus dos canales actúan sobre ese mismo contador. Dos canales no significan dos contadores independientes. [Enunciado, pp. 4–7.]

En nuestro ejercicio 2.2 solo hay una entrada útil, GPIO4. El canal incrementa en cada bajada y mantiene la cuenta en las subidas. `level_gpio_num = -1` indica que no usamos un segundo pin de control. Un encoder A/B usaría además el nivel de la otra señal para decidir cómo modificar la cuenta.

### ¿Por qué la frecuencia se mide contando?

Una señal periódica tiene un número de ciclos por segundo. Si contamos una bajada por ciclo:

```text
f [Hz] = N / Δt [s]
```

Por ejemplo, 1000 pulsos durante 1,010 s equivalen aproximadamente a 990,1 Hz, no a 1000 Hz. Por eso el programa mide la duración efectiva de la ventana. Si se cuentan subidas y bajadas, hay dos cuentas por periodo y se debe dividir por dos.

El error de una cuenta es proporcional a `1/Δt`: una ventana más larga mejora la resolución a costa de actualizar la medida más despacio. Con una frecuencia muy baja puede resultar más útil medir el periodo entre flancos. Estas son decisiones de medida, no cambios en la definición de Hz.

### ¿Qué hacen los límites y el filtro?

El contador hardware vuelve a cero cuando alcanza un límite. Los límites ±100 del ejemplo del PDF no son una solución general para frecuencias de varios kHz. En este proyecto, `accum_count` y los *watch points* de ambos límites permiten al driver sumar esas vueltas. `pcnt_unit_clear_count()` también borra el acumulador. El resultado sigue teniendo capacidad finita. [API PCNT, enlace al final.]

El filtro de 1000 ns descarta impulsos muy cortos; no comprueba si una frecuencia es «correcta». Si el pulso útil es demasiado estrecho, también puede ser rechazado. Tampoco sustituye un antirrebote de botón de varios milisegundos.

### ¿Por qué detener antes de borrar?

En esta solución cada ventana se detiene, se lee y se informa antes de abrir la siguiente. Así el borrado no compite con una cuenta activa. Hay un intervalo entre ventanas sin adquisición, documentado en el README. Para contar todos los eventos de un proceso sin huecos se necesitaría otra estrategia, por ejemplo lecturas por diferencias de un contador continuo con su desbordamiento bien tratado.

## Trabajo previo: RMT y el LED WS2812

RMT significa **Remote Control Transceiver**. Permite producir o capturar secuencias de niveles con duraciones precisas. El laboratorio lo utiliza para enviar el protocolo del WS2812, cuyas anchuras de pulso codifican datos. El componente `led_strip` prepara esa secuencia; nuestro programa solo especifica colores. [Enunciado, pp. 8–10; documentación RMT.]

Debes distinguir estas tres operaciones:

| Operación | Qué significa |
| --- | --- |
| `led_strip_new_rmt_device(...)` | Crear/configurar el dispositivo LED usando RMT. |
| `led_strip_set_pixel(led, 0, R, G, B)` | Escribir el color en el buffer del componente; índice 0 para el único LED. |
| `led_strip_refresh(led)` | Transmitir el buffer al LED. |

No sirve dejar GPIO48 a 1 para conseguir «blanco»; ese pin transporta datos, no tres alimentaciones de color. El valor 10 MHz de `resolution_hz` indica una resolución temporal de 0,1 µs para RMT; no es la frecuencia de cambio entre los cuatro colores ni la frecuencia de la CPU.

El formato GRB describe el orden de bytes enviado al chip. La función sigue recibiendo sus argumentos en orden R, G, B. Blanco significa activar simultáneamente los tres componentes; rojo significa R distinto de cero y G/B a cero. El tiempo de dos segundos lo decide el programa mediante `vTaskDelay()`, no el protocolo del LED.

## Watchdog: inicializar, registrar y alimentar

El watchdog es un temporizador de vigilancia: exige que se produzca una señal de actividad dentro de un plazo. En el ejercicio se usa BOOT como condición artificial de actividad. En un sistema real normalmente se alimentaría al verificar progreso correcto de tareas o funciones críticas. [Enunciado, pp. 11–12.]

| Llamada | Papel en el ejercicio |
| --- | --- |
| `esp_task_wdt_init(&cfg)` | Crear/configurar el TWDT si todavía no está inicializado. |
| `esp_task_wdt_reconfigure(&cfg)` | Cambiar la configuración de uno ya inicializado. |
| `esp_task_wdt_add(NULL)` | Registrar la tarea actual para que deba alimentar el TWDT. |
| `esp_task_wdt_reset()` | Comunicar que esa tarea sigue cumpliendo su condición de actividad. |

`reset()` aquí significa **alimentar el temporizador**, no reiniciar el ESP32. El reinicio ocurre cuando falta esa alimentación durante el plazo y se ejecuta la respuesta configurada. La máscara `idle_core_mask` indica qué tareas idle se vigilan además; el bucle les permite ejecutarse al ceder CPU.

### Preguntas habituales

**¿Por qué no alimentar el watchdog en todas las vueltas?** Porque el requisito exige una pulsación. Alimentarlo continuamente impediría demostrar que la ausencia de BOOT provoca el timeout.

**¿Por qué detectar un flanco y no `gpio_get_level(BOOT) == 0`?** Con una condición por nivel, un botón mantenido alimentaría indefinidamente. Con un flanco estable solo alimenta una vez hasta que se suelta y se vuelve a pulsar.

**¿Por qué podría fallar una llamada a `init()`?** ESP-IDF puede haber inicializado el TWDT durante el arranque. El error `ESP_ERR_INVALID_STATE` debe llevar en este caso a `reconfigure()`, no a abortar como si fuera un fallo de hardware.

**¿Por qué un timeout no siempre reinicia?** La reacción debe incluir pánico y el manejador de pánico debe estar configurado para reiniciar. Un mensaje de advertencia no demuestra un reinicio. El README indica las opciones necesarias.

**¿Qué diferencia hay entre TWDT e IWDT?** TWDT vigila las tareas o usuarios registrados; IWDT detecta que las interrupciones no pueden atenderse a tiempo. Desactivar uno no equivale a desactivar el otro. No se debe resolver un fallo de código desactivando indiscriminadamente la vigilancia.

**¿Por qué no probarlo con el depurador detenido?** OpenOCD modifica el funcionamiento de los watchdogs. Cierra la sesión y reinicia antes de verificar el timeout. [API Watchdogs, enlace al final.]

## Deep sleep y memoria RTC

En deep sleep se detiene la ejecución normal y se desconectan dominios de alimentación. Al despertar se vuelve a arrancar el programa. La memoria RTC que se conserva permite recuperar variables declaradas con el atributo adecuado. En esta práctica se usa `RTC_SLOW_ATTR`. [Enunciado, pp. 13–15.]

| Concepto | Respuesta que debes defender |
| --- | --- |
| `esp_sleep_enable_timer_wakeup(10000000ULL)` | Preparar un despertar por RTC tras diez segundos nominales; la unidad del argumento es µs. |
| `esp_deep_sleep_start()` | Entrar en deep sleep; el programa no retoma después de esta llamada al despertar. |
| `esp_sleep_get_wakeup_cause()` | Consultar si el último arranque procede de una fuente de despertar configurada. |
| `RTC_SLOW_ATTR` | Situar la variable en una zona RTC que se mantiene en esta configuración de sueño. |
| Reinicio o pérdida de alimentación | No deben tratarse automáticamente como continuación de un sueño; aquí el contador comienza en cero. |

La memoria RTC no es una EEPROM. Retener durante deep sleep no garantiza conservar el dato al quitar la alimentación. Si el requisito fuese conservarlo sin energía, habría que estudiar almacenamiento no volátil y su desgaste.

El código suma 100 décimas al despertar para contabilizar los diez segundos nominales de sueño, como en la solución previa. No obtiene diez segundos exactos de un reloj externo ni incluye todo el tiempo de arranque. La variante `CONTAR_TIEMPO_DORMIDO = 0` conserva la cuenta pero la pausa durante el sueño. Explica cuál has elegido al mostrar el ejercicio.

**¿Por qué la pinza no marca el mínimo de consumo anunciado para el chip?** Porque mide el circuito completo en el punto elegido: regulador, convertidor USB-UART, LEDs y componentes auxiliares pueden seguir consumiendo. Lo exigido es observar y explicar la caída real; no inventar una cifra de microamperios.

### EXT0: aclaración de una contradicción del PDF

En la p. 15, el texto confunde el nivel de reposo del pull-down y el nivel que provoca el despertar. El ejemplo de código usa `esp_sleep_enable_ext0_wakeup(GPIO7, 1)` junto con pull-down: eso significa **reposo bajo y despertar cuando llega un nivel alto**. No significa despertar por nivel bajo.

Para reposo alto y despertar bajo se usaría pull-up y nivel de activación 0. EXT0 responde a un **nivel**, no a un flanco de software. Si ese nivel ya está activo al dormir, el sistema puede despertar enseguida. Además, el pin debe ser RTC GPIO. [Enunciado, p. 15; API Sleep Modes.]

El ejercicio 5.1 no activa EXT0. BOOT provoca la entrada en sueño mientras se ejecuta el programa; el temporizador RTC provoca la salida diez segundos después.

## UART: de bytes a líneas

UART transmite una secuencia de bytes sin un paquete de longitud arbitraria incorporado. Una lectura puede devolver media línea, una línea o varias líneas. El programa debe guardar bytes y reconocer el delimitador ENTER antes de actuar. [Enunciado, pp. 16–18.]

**8N1** significa ocho bits de datos, ninguna paridad y un bit de parada. Con el bit de inicio, un carácter ocupa diez bits de transmisión. A 115200 baudios el máximo teórico de ese formato es `115200 / 10 = 11520` caracteres de ocho bits por segundo, antes de añadir pausas y otras limitaciones.

Las funciones tienen papeles distintos:

- `uart_driver_install()`: reservar los recursos y buffers del driver.
- `uart_param_config()`: fijar velocidad y formato.
- `uart_set_pin()`: asignar TX/RX y, si se usaran, RTS/CTS.
- `uart_read_bytes()`: extraer bytes recibidos, con tamaño máximo y tiempo de espera.
- `uart_write_bytes()`: enviar una cantidad de bytes.

El tamaño solicitado a `uart_read_bytes()` nunca debe superar el array destino, aunque el driver anuncie que hay más bytes pendientes. Reservar `char linea[128]` deja espacio para **127 bytes de texto más el terminador NUL**. NUL es la marca de fin de una cadena C; no se envía como parte de la línea al PC.

CR es `\r` y LF es `\n`. CRLF son dos bytes que representan un único ENTER en muchos terminales. El programa los reconoce sin ejecutar dos órdenes. Las cadenas largas se rechazan completas, porque actuar sobre una cadena truncada puede cambiar su interpretación.

**¿Qué hace `GRIS`?** Contiene G y R: dos clases de color, así que conserva el color actual. `RRR` contiene una sola clase, R, y pone rojo. Las comparaciones del programa son sensibles a mayúsculas. La regla es presencia de letras de color, no comprensión del nombre de un color.

**¿Por qué no recibe nada por el otro USB de la placa?** El transporte USB-Serial/JTAG no es automáticamente el periférico UART0. Este ejercicio espera los datos a través del puente USB-UART conectado a UART0.

## Archivos y fragmentos que conviene saber reconstruir

| Archivo | Qué debes poder explicar y escribir |
| --- | --- |
| `ejercicio_2_2.c` | Unidad/canal PCNT; acciones por flanco; filtro; límites; fórmula de frecuencia y ventana real. |
| `ejercicio_3_1.c` | Crear `led_strip`, diferencia `set_pixel`/`refresh`, orden de colores y retardo. |
| `ejercicio_4_1.c` | Configurar TWDT, `init` frente a `reconfigure`, registro de tarea, alimentar solo por flanco. |
| `ejercicio_5_1.c` | Variable RTC, causa de despertar, timer de 10 s, entrada en sueño y nuevo arranque. |
| `ejercicio_6_1.c` | Configurar UART; acumular una línea con límites; ENTER; regla de color. |
| `main/idf_component.yml` | Por qué existe la dependencia `espressif/led_strip`. |
| `main/CMakeLists.txt` | Por qué se compila solo `main.c` y dónde se declaran los componentes. |

## Referencias para verificar detalles

- **Práctica 3, Sistemas Empotrados, curso 26/27**, documento proporcionado por el usuario; páginas citadas en cada bloque.
- Soluciones previas de P3, adaptadas como base de la implementación y de algunas decisiones de comportamiento.
- [Espressif: PCNT](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/pcnt.html).
- [Espressif: RMT](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/rmt.html), referencia indicada por el laboratorio para el trabajo previo.
- [Espressif: Watchdogs](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/system/wdts.html).
- [Espressif: Sleep Modes](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/system/sleep_modes.html).
- [Espressif: UART](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/uart.html).

**Aclaración transversal:** `vTaskDelay()` bloquea la tarea que lo llama y permite ejecutar otras tareas. No mantiene necesariamente toda la CPU «sin hacer nada», como sugiere alguna explicación del laboratorio. Tampoco es equivalente a deep sleep.
