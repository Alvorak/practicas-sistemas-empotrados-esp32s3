# Práctica 1: respuestas teóricas y repaso

Estas respuestas siguen la numeración de **Práctica 1, curso 26-27**. El StarterKit completo no formaba parte de los archivos recibidos, pero se ha localizado y revisado el esquema exacto `YD-ESP32-S3-SCH-V1.4.pdf` en el repositorio de su fabricante, VCC-GND Studio. Los apartados 2.3 y 2.4 describen esa revisión V1.4, fechada el 23/09/2022; si tu placa física pertenece a otra revisión, compara sus referencias antes de trasladar la respuesta.

## 2.1. Características del ESP32-S3

Hay tres niveles diferentes: **chip ESP32-S3**, **módulo ESP32-S3-WROOM-1-N16R8** y **placa de desarrollo**. El módulo añade memorias, antena y otros componentes; la placa añade conectores, alimentación, pulsadores, indicadores y puente USB-UART.

| Característica | Respuesta |
| --- | --- |
| CPU | Dos núcleos Xtensa LX7 de 32 bits |
| Frecuencia máxima | 240 MHz; la práctica puede configurar una frecuencia inferior |
| ROM interna | 384 KB |
| SRAM interna | 512 KB |
| SRAM del dominio RTC | 16 KB |
| GPIO del chip | 45: GPIO0-21 y GPIO26-48 |
| GPIO del módulo WROOM-1 | Hasta 36; algunos se reservan según las memorias instaladas |
| Módulo N16R8 | 16 MB de flash y 8 MB de PSRAM |
| Conectividad inalámbrica | Wi-Fi de 2,4 GHz y Bluetooth LE |
| Conversión analógica | Dos ADC de 12 bits; no incluye DAC analógico integrado |

Fuentes: [datasheet del chip, características](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf), [datasheet del módulo, tabla de variantes](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) y [restricciones GPIO](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-reference/peripherals/gpio.html). El PDF de la práctica introduce esta distinción en la página 4.

**Idea de examen:** la flash conserva el programa sin alimentación; la RAM contiene datos de trabajo y se pierde normalmente al apagar. La PSRAM amplía la RAM, pero no es la SRAM interna del chip. La memoria RTC puede conservar determinados datos durante deep-sleep mientras permanezca alimentada. «45 GPIO» no significa «45 pines libres en mi placa»: pueden estar conectados a memoria, USB o funciones de arranque.

## 2.2. Periféricos

El ejercicio pide al menos cinco. Puedes explicar los siguientes usando ejemplos de tus tres prácticas:

| Periférico | Para qué sirve | Ejemplo |
| --- | --- | --- |
| GPIO | Leer o escribir niveles digitales | BOOT y LEDs externos |
| ADC | Convertir una tensión analógica en un número | Potenciómetro de la práctica 2 |
| GPTimer | Contar tiempo y generar alarmas | Señales periódicas de la práctica 2 |
| LEDC/PWM | Generar una señal digital con ciclo de trabajo ajustable | Brillo del LED |
| PCNT | Contar flancos mediante hardware | Medición de frecuencia de la práctica 3 |
| UART | Enviar y recibir datos serie asíncronos | Terminal del ordenador |
| I2C | Comunicar varios dispositivos en un bus de datos y reloj | Sensor o pantalla compatible |
| SPI | Transferir datos serie con reloj, normalmente a mayor velocidad | Memoria o pantalla SPI |
| RMT | Generar o recibir secuencias de pulsos temporizados | Protocolo del LED WS2812 |
| Watchdog | Detectar falta de progreso o de alimentación periódica | Reinicio de la práctica 3 |

Estos periféricos aparecen en las prácticas y en el diagrama funcional del [datasheet del ESP32-S3](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf). La explicación de uso es un resumen para estudiar, no una lista de componentes que haya que conectar a la vez.

## 2.3. Elementos de la placa

**BOOT:** está conectado a **GPIO0** y a GND al pulsarlo; la aplicación lo configura con pull-up. GPIO0 también selecciona el modo de arranque: mantener BOOT durante un reset puede hacer entrar en modo de descarga. El esquema muestra el condensador C8 asociado al botón. El dibujo parcial también aparece en la página 11 del enunciado.

**RESET/RST/EN:** el otro pulsador pone a GND la señal `CHIP_PU`/EN y reinicia el chip. No es un segundo botón GPIO normal.

**LED inteligente:** U1 es un **XL-5050RGBC-WS2812B**. Su entrada de datos recibe **GPIO48** a través del puente `RGB`. Incorpora un controlador que recibe datos temporizados y memoriza los niveles de rojo, verde y azul; por eso no se maneja como un LED sencillo con `gpio_set_level(48, 1)`. La práctica 3 lo presenta en su página 8.

**Cantidad de LEDs:** el esquema V1.4 contiene **cuatro encapsulados luminosos**: uno RGB y tres indicadores. El RGB contiene varios emisores internos, pero se cuenta como un encapsulado.

| Indicador | Conexión / función en V1.4 |
| --- | --- |
| RGB, U1 | WS2812B controlado desde GPIO48; color programable. |
| PWR | Conectado al rail `VDD33` mediante R6; indica alimentación de 3,3 V. |
| TX | Asociado a `U0TXD`, GPIO43; indica actividad de transmisión UART0. |
| RX | Asociado a `U0RXD`, GPIO44; indica actividad de recepción UART0. |

**Otros circuitos y elementos identificados:**

| Referencia | Componente del esquema V1.4 | Función |
| --- | --- | --- |
| U2 | CH343P | Puente USB-UART: conecta el USB1 con las señales UART0 del ESP32-S3. |
| U3 | CJ6107A33GW | Regulador que obtiene `VDD33` desde el rail nominal de 5 V. |
| U4 | ESP32-S3-WROOM-1 | Módulo con microcontrolador y elementos de memoria/radio según su variante. |
| Q1 y Q2 | SS8050-G | Transistores del circuito de programación automática: relacionan DTR/RTS con EN y GPIO0. Son transistores discretos, no otros circuitos integrados. |

**Tensiones disponibles:** alimentación USB nominal de **5 V**, rail regulado de **3,3 V** y **GND**. El esquema incluye diodos entre algunas entradas y el rail interno; no todos los puntos etiquetados como alimentación son el mismo nodo. En particular, el uso del pin de 5 V de J1 como salida depende del puente `IN-OUT`. Los GPIO se usan con lógica de 3,3 V, aunque la placa tenga una alimentación de 5 V.

Fuentes: [esquema exacto V1.4 del fabricante, hoja única](https://github.com/vcc-gnd/YD-ESP32-S3/blob/b4ac4d60d1845a554bbbc7c97806ccb97cf08941/5-public-YD-ESP32-S3-Hardware%20info/YD-ESP32-S3-SCH-V1.4.pdf) y [descripción de componentes del fabricante](https://github.com/vcc-gnd/YD-ESP32-S3/blob/b4ac4d60d1845a554bbbc7c97806ccb97cf08941/README.md). Las referencias anteriores son las del esquema; en una placa de otra revisión deben contrastarse con su serigrafía y documentación.

## 2.4. Jumpers o puentes de soldadura

Un puente permite unir o separar eléctricamente dos puntos. No tiene por qué llevar una pieza extraíble: el esquema **V1.4 muestra cuatro**, representados como conexiones de **0 Ω en formato 0603**.

| Nombre | Conexión exacta en V1.4 | Función al cerrarlo |
| --- | --- | --- |
| `RGB` | Entre GPIO48 y `RGB_CTRL`, entrada DI del WS2812B. | Conecta la señal de datos al LED inteligente. |
| `IN-OUT` | En paralelo con D3, entre J1 pin 21 (5 V del conector de pines) y el rail interno `5V`. | Puentea el diodo y permite que ese pin se conecte directamente al rail, incluyendo su uso como salida de alimentación. Abierto, D3 permite alimentar el rail desde el pin y bloquea el sentido contrario. |
| `USB-OTG` | En paralelo con D2, entre VBUS del conector USB2 nativo y el rail interno `5V`. | Permite que el rail de 5 V alimente VBUS del USB nativo, como se necesita para suministrar energía a un dispositivo en un montaje host/OTG adecuado. No conmuta las líneas de datos. |
| `USB-JTAG` | Conecta GPIO3 al extremo de R11; el otro extremo de R11 (10 kΩ) va a `VDD33`. | Añade un pull-up externo a GPIO3, pin de selección de arranque relacionado con la fuente de JTAG. |

La interpretación de `USB-JTAG` tiene un matiz: por defecto, el ESP32-S3 usa el controlador USB Serial/JTAG. El nivel de GPIO3 selecciona entre JTAG por USB y por pines **cuando la configuración de eFuses permite esa selección** (`STRAP_JTAG_SEL`); en ese caso, nivel alto selecciona USB Serial/JTAG. Por tanto, el puente no es un interruptor universal que «active JTAG». Véase la [explicación oficial de la selección JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/jtag-debugging/configure-other-jtag.html).

**Respuesta para el examen sobre el esquema pedido: cuatro puentes, con las funciones anteriores.** La comprobación física consiste en confirmar que tu placa es V1.4 y observar cuáles están montados, abiertos o cerrados. El esquema no permite afirmar cómo están soldados en tu ejemplar. No hace falta soldar puentes ni modificar eFuses para ejecutar los programas de este repositorio con un montaje que ya funciona.

Fuente de los cuatro puentes y sus nodos: [YD-ESP32-S3-SCH-V1.4.pdf, fabricante VCC-GND Studio, revisión del repositorio `b4ac4d60d1845a554bbbc7c97806ccb97cf08941`](https://github.com/vcc-gnd/YD-ESP32-S3/blob/b4ac4d60d1845a554bbbc7c97806ccb97cf08941/5-public-YD-ESP32-S3-Hardware%20info/YD-ESP32-S3-SCH-V1.4.pdf). La función de los puentes de alimentación se deduce siguiendo sus conexiones y el sentido de los diodos del dibujo.

## 4.1. Entrada con pull-up

Con el pulsador abierto, la resistencia de pull-up mantiene la entrada cerca de VCC: se lee **1 lógico**. Al pulsar y conectar la entrada a GND, se lee **0 lógico**. La resistencia evita una entrada flotante y limita la corriente cuando se cierra el pulsador. Fuente: esquema de la práctica, páginas 7-8.

## 4.2. Resistencias de los LEDs

Datos de diseño del enunciado: alimentación de 3,3 V, caída en el LED de aproximadamente 1,7 V y corriente máxima de 5 mA.

```text
Vresistencia = 3,3 V - 1,7 V = 1,6 V
R = Vresistencia / I = 1,6 V / 0,005 A = 320 ohmios
```

Se elige **330 Ω**, valor comercial por encima del resultado:

```text
I = 1,6 V / 330 ohmios = 0,00485 A = 4,85 mA
P = I^2 x R = aproximadamente 0,0078 W
```

Una resistencia de 1/8 W o 1/4 W tiene margen suficiente para esa potencia. Cada LED necesita una resistencia propia. Este cálculo procede de los datos de la página 11; las pequeñas caídas del transistor de salida se han despreciado en el modelo de clase.

El repositorio original utiliza LEDs blanco y azul. Si su caída de tensión real difiere de 1,7 V, la corriente también será distinta. Para calcular el montaje real se usa la caída de esos LEDs, no se cambia el resultado teórico que pide el enunciado.

## 4.3. Configuración de BOOT

El esquema conecta BOOT entre GPIO0 y GND. Por tanto:

```c
gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
```

Suelto se lee `1`; pulsado se lee `0`. Para detectar la pulsación por interrupción se configura `GPIO_INTR_NEGEDGE`. El condensador que aparece en el esquema no sustituye la necesidad de un nivel de reposo definido. Fuente: PDF, página 11.

## 4.5. Por qué falla la respuesta del botón

Durante `vTaskDelay(pdMS_TO_TICKS(3000))`, **la tarea que llamó a la función está bloqueada**. Si esa misma tarea es la única que lee BOOT, no vuelve a consultarlo hasta terminar la espera. Una pulsación que empieza y acaba dentro de esos tres segundos puede perderse.

`vTaskDelay()` no mantiene la CPU ocupada ejecutando una espera activa: permite ejecutar otras tareas o la tarea idle y no impide atender interrupciones. Los ticks son los del planificador FreeRTOS, no ciclos de reloj de la CPU. Este matiz corrige la descripción simplificada del PDF; véase la [documentación oficial de FreeRTOS en ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-reference/system/freertos_idf.html).

Soluciones posibles:

1. Leer BOOT más a menudo y controlar el parpadeo con tiempo transcurrido en lugar de una espera de 3 s en ese mismo recorrido.
2. Utilizar una interrupción de GPIO, como en 5.1.
3. Repartir el trabajo entre tareas, cuando la estructura del programa lo justifique.
4. Generar la temporización periódica con un temporizador hardware, como se aprende en la práctica 2.

Para el examen debes relacionar cada solución con el problema: aumentar la frecuencia de lectura reduce la latencia por polling; una interrupción permite responder a un evento mientras otra tarea está esperando.

## Aclaraciones sobre el vocabulario del enunciado

**Push-pull y pull-up no son sinónimos.** Push-pull describe una etapa de salida que puede llevar el pin a nivel alto o bajo. Pull-up/pull-down describe una resistencia que fija un nivel de reposo. Cuando el PDF llama «LED en pull-up» a GPIO4, lo que el circuito muestra es un LED **activo en bajo** conectado hacia 3,3 V. GPIO5 usa un LED **activo en alto** conectado hacia GND.

**GPIO no es número de patilla del conector.** `GPIO_NUM_4` identifica la señal GPIO4; no significa «cuarta patilla contando desde la esquina».

**La UART de la placa no tiene niveles RS-232 clásicos.** El enlace entre el ESP32-S3 y el puente USB-UART utiliza señales lógicas del sistema. El término RS232 aparece de forma imprecisa en el PDF; no se deben conectar tensiones RS-232 directamente a sus GPIO.

**Un flanco y un nivel no son lo mismo.** Nivel bajo significa que BOOT está pulsado; flanco de bajada significa que acaba de pasar de suelto a pulsado. Si alternaras el LED en cada vuelta mientras el nivel es cero, cambiaría muchas veces durante una sola pulsación.

## Preguntas típicas de examen

### ¿Por qué el LED de GPIO4 se enciende escribiendo cero?

Su ánodo recibe alimentación por la resistencia y su cátodo llega al GPIO. Al poner GPIO4 a cero existe diferencia de potencial y circula corriente hacia el pin. En GPIO5 el conexionado es el contrario: la corriente sale del pin cuando se escribe uno.

### ¿Por qué usar `1ULL << BOTON_BOOT`?

`pin_bit_mask` es una máscara de bits que permite seleccionar uno o varios GPIO. `ULL` hace que el desplazamiento use un entero de al menos 64 bits; esto permite representar también GPIO por encima de 31. Desplazar un `int` de 32 bits por un valor grande no sirve para esos pines.

### ¿Qué diferencia hay entre polling e interrupciones?

Polling consulta periódicamente la entrada desde el programa. Una interrupción ejecuta una rutina cuando el hardware detecta el evento configurado. Polling es sencillo, pero su latencia depende de cada cuánto se consulta; las interrupciones evitan esperar al siguiente paso del bucle, aunque requieren cuidar qué se hace en la ISR.

### ¿Puedo llamar a `printf()` o `vTaskDelay()` dentro de la ISR?

No se utilizan en las ISR de estos ejercicios: imprimir puede bloquear y una ISR no puede dormirse como una tarea. Se mantiene el trabajo corto y, si hace falta procesamiento posterior, se avisa a una tarea. Las operaciones de FreeRTOS dentro de una ISR deben utilizar sus variantes `...FromISR` cuando existan y estén permitidas.

### ¿Por qué 6.1 utiliza una cola?

Para pasar de forma segura un evento compuesto por número y timestamp a otra tarea. La ISR envía una copia y vuelve rápidamente; la tarea recibe y escribe por el terminal. Un único indicador `hay_evento` podría fusionar varias pulsaciones, y compartir varios campos sin coordinación podría producir una lectura inconsistente.

### ¿`volatile` evita todos los problemas entre ISR y tareas?

No. `volatile` no convierte una operación compuesta en atómica ni proporciona exclusión mutua. En 6.1 los datos se copian mediante la cola de FreeRTOS, por lo que no se necesita leer directamente desde la tarea un contador de 64 bits que pueda estar cambiando.

### ¿Por qué hay antirrebote al soltar además de al pulsar?

Los contactos pueden oscilar en ambas transiciones. Tras una pulsación larga, un rebote al soltar podría generar otro flanco de bajada y parecer una segunda pulsación. En 5.1 y 6.1 se acepta el primer flanco, se bloquean los siguientes y solo se rearma después de una liberación estable de 30 ms.

### ¿Qué mide `esp_timer_get_time()`?

Devuelve un `int64_t` con microsegundos desde la inicialización temprana de ESP Timer en ese arranque. Es la referencia de tiempo desde el arranque usada por el laboratorio; no es fecha/hora de calendario y vuelve a empezar tras un reinicio o un despertar de deep-sleep. Puede obtenerse desde una ISR. Fuente: [ESP Timer, obtención del tiempo](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32s3/api-reference/system/esp_timer.html#obtaining-current-time).

### ¿Por qué usar `PRId64` o `PRIu32` en `printf()`?

Las macros de `<inttypes.h>` proporcionan el formato correspondiente a los tipos de ancho fijo. Evitan asumir que un `int64_t` usa el mismo formato que `int` o `long` en cualquier plataforma.

### ¿Por qué un LED que cambia cada tres segundos no tiene frecuencia de 1/3 Hz?

Para completar un ciclo necesita 3 s encendido y 3 s apagado. Su periodo completo es aproximadamente 6 s, por lo que la frecuencia es aproximadamente `1/6 Hz`. Hay un cambio de nivel cada tres segundos, pero dos cambios por periodo.

### ¿Qué diferencia hay entre el terminal UART y JTAG?

El terminal muestra los mensajes que el programa decide enviar. JTAG permite detener la ejecución, avanzar instrucciones o líneas, inspeccionar registros y variables y establecer breakpoints. Un breakpoint altera la temporización del programa; no se usa para demostrar una frecuencia mientras la ejecución está detenida. El procedimiento de la práctica está en [README.md](README.md#ejercicio-62-depuración-con-usb-serialjtag).

## Qué debes poder reconstruir sin mirar

1. Configuración de una salida, entrada pull-up y lectura de BOOT: **4.4**.
2. Comparación entre estado anterior y actual, y motivo del antirrebote: **4.6**.
3. `gpio_config_t`, `gpio_install_isr_service()` y `gpio_isr_handler_add()`: **5.1**.
4. ISR corta, `esp_timer_get_time()`, envío a la cola y `printf()` desde la tarea: **6.1**.
5. Cálculo de 320 Ω -> 330 Ω, polaridades de ambos LEDs y significado real de `vTaskDelay()`.

No hace falta memorizar literalmente toda la tarea auxiliar. Sí debes poder explicar por qué existe, qué parte atiende el evento y dónde sería incorrecto bloquear o imprimir.
