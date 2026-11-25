# Práctica 3: contadores, RGB, watchdog, sleep y UART

Resumen del enunciado **Práctica 3, curso 26/27**, y adaptación de las soluciones previas a este proyecto unificado. Cada `.c` contiene un programa completo: copia su contenido a `main/main.c` y compila **desde la raíz del proyecto**. No compiles todos los ejercicios a la vez, porque todos tienen `app_main()`.

Las respuestas de teoría están en [Teoria.md](Teoria.md). Los ajustes comunes se explican en [Configuración](../docs/configuracion.md).

## Índice y correspondencia con las fuentes

| Ejercicio del PDF | Qué pide | Archivo de este proyecto | Solución previa |
| --- | --- | --- | --- |
| 2.1, p. 4 | Explicar las señales A/B de un encoder incremental y su desfase. | [Teoria.md](Teoria.md#ejercicio-21-encoder-incremental) | No había programa: es teoría. |
| 2.2, p. 7 | Contar pulsos durante un segundo mediante PCNT y expresar la frecuencia en Hz. | [ejercicio_2_2.c](ejercicio_2_2.c) | `ejercicio_21.c`, renumerado para coincidir con el PDF. |
| 3.1, p. 10 | Repetir rojo, verde, azul y blanco; cambiar cada dos segundos. | [ejercicio_3_1.c](ejercicio_3_1.c) | `ejercicio_31.c`. |
| 4.1, p. 12 | Watchdog de seis segundos alimentado mediante BOOT; mostrar décimas y observar el reinicio. | [ejercicio_4_1.c](ejercicio_4_1.c) | `ejercicio_41.c`. |
| 5.1, p. 15 | Contar décimas, dormir al pulsar BOOT, despertar a los diez segundos y conservar la cuenta. | [ejercicio_5_1.c](ejercicio_5_1.c) | `ejercicio_51.c`. |
| 6.1, p. 18 | Recibir una línea por UART, devolverla al pulsar ENTER y seleccionar el RGB con R/G/B. | [ejercicio_6_1.c](ejercicio_6_1.c) | `ejercicio_61.c`. |

El `main.c` de la solución previa era una prueba de un color fijo; aquí se usa la secuencia cíclica que pide el ejercicio 3.1. El CMake anterior seleccionaba directamente `ejercicio_61.c`; en el proyecto unificado siempre se compila `main/main.c`.

## Hardware y dependencias

| Elemento | Conexión usada | Observación |
| --- | --- | --- |
| BOOT integrado | GPIO0 | Entrada con pull-up; pulsado equivale a 0. |
| WS2812B integrado | GPIO48 | Es el pin indicado en el enunciado y usado en la solución previa. Verificar el esquema si se usa otra revisión de placa. |
| Entrada de pulsos PCNT | GPIO4 | Se mantiene el pin de la solución previa. Quitar el LED rojo de la práctica 1 antes de conectar el generador. |
| UART0 hacia el puente USB-UART | TX GPIO43, RX GPIO44 | Conectar el USB de la placa identificado como UART. |
| Generador de señales | Señal a GPIO4, masa a GND | Señal entre 0 y 3,3 V, con masa común. Verificar niveles reales en el osciloscopio. |

Se usan las APIs de ESP-IDF vistas en clase: `pulse_cnt`, `gpio`, `esp_timer`, `esp_task_wdt`, `esp_sleep` y `uart`. La única dependencia descargable es **`espressif/led_strip` versión 3.0.0**, declarada en `main/idf_component.yml`. Es el componente oficial que ya aparece en la práctica: genera con RMT el protocolo del WS2812. No es Arduino ni una biblioteca privada del proyecto.

La primera configuración/compilación necesita descargar el componente. Su código quedará en `managed_components`; no hay que copiarlo a cada ejercicio. El CMake de la raíz ya incorpora los componentes necesarios.

## Ejercicio 2.1: encoder incremental

**Enunciado resumido:** investigar para qué se utilizan dos señales A/B y qué permite saber su desfase. No requiere un firmware. Respuesta desarrollada en [Teoria.md](Teoria.md#ejercicio-21-encoder-incremental).

## Ejercicio 2.2: frecuencímetro con PCNT

**Enunciado resumido:** contar los pulsos de una señal cuadrada durante un segundo, mostrar su frecuencia en el terminal y comprobar varias frecuencias con el generador de clase.

**Montaje:** generador a GPIO4 y GND común. Ajustar la señal a 0–3,3 V; una cuadrada simétrica alrededor de 0 V no sirve. Algunos generadores muestran amplitud según una carga de 50 Ω: comprobar la amplitud efectiva con la carga del montaje.

**Funcionamiento del código:**

1. Crear una unidad PCNT, su filtro de 1000 ns y un canal.
2. Contar únicamente flancos de bajada: una onda cuadrada aporta una cuenta por periodo.
3. Añadir los dos límites como *watch points* y activar `accum_count` para no perder la cuenta cuando el contador hardware vuelve a cero.
4. Borrar el contador estando parado, arrancar, esperar aproximadamente un segundo y detenerlo.
5. Leer la cuenta y dividirla entre la duración medida de la ventana.

La frecuencia se calcula como `pulsos × 1 000 000 / ventana_us`. No se supone que `vTaskDelay(1000 ms)` dure exactamente un segundo. Las llamadas de arranque/parada introducen un pequeño error adicional de delimitación; tampoco es una puerta hardware de precisión.

**Límites de esta solución:** las ventanas son independientes. Durante la lectura, la impresión y el borrado previo a la siguiente ventana no se cuentan pulsos. Ese intervalo no entra en la frecuencia calculada: es adecuado para medir señales periódicas estables, no para contabilizar todos los eventos de un proceso sin interrupción. El filtro de 1 µs puede descartar pulsos válidos demasiado estrechos. No hay un rango máximo de laboratorio validado en placa; empezar, por ejemplo, por 10 Hz, 100 Hz y 1 kHz y contrastar el resultado con el generador.

En una ventana de un segundo, un error de una cuenta representa aproximadamente 1 Hz. A frecuencias bajas es normal que medidas consecutivas discrepen en una cuenta según la fase de apertura de la ventana. La compensación de los límites mantiene un resultado `int`; no convierte el contador en una capacidad ilimitada.

**Comprobación:** observar `Pulsos`, `ventana` y `frecuencia` en el monitor a 115200 baudios. La lectura debe seguir los cambios del generador. Si aparece el doble, revisar si se están contando ambos flancos; si aparece cero, revisar masa común, pin, niveles y señal.

## Ejercicio 3.1: secuencia RGB

**Enunciado resumido:** usar el ejemplo del LED rojo como base y repetir **rojo → verde → azul → blanco**, con dos segundos por color.

No necesita cableado adicional en la placa indicada. Configura un LED WS2812 en GPIO48 y un canal RMT de 10 MHz. `led_strip_set_pixel()` prepara el color; `led_strip_refresh()` lo transmite. Aunque el protocolo del LED tenga orden GRB, los argumentos de `set_pixel()` siguen siendo rojo, verde y azul.

Se mantiene la intensidad 150 de la solución previa para reducir brillo y consumo. No significa que un valor 255 sea una orden para «quemar» el LED: la alimentación, el componente y las condiciones eléctricas determinan sus límites.

**Comprobación:** un ciclo completo dura aproximadamente ocho segundos. Si los colores están intercambiados, revisar el modelo/orden del LED de la placa. Si no luce, confirmar GPIO48 y la instalación del componente antes de modificar el programa.

## Ejercicio 4.1: TWDT de seis segundos

**Enunciado resumido:** alimentar el watchdog con cada pulsación de BOOT. Si pasan seis segundos sin una pulsación válida, provocar un reinicio. Mostrar tiempo de ejecución con una cifra decimal y evidenciar que el reinicio procede del watchdog.

### Configuración necesaria

En `idf.py menuconfig`, usar `/` para buscar el símbolo cuando el nombre o la posición del menú cambien entre versiones. En ESP-IDF 5.5 están en **Component config → ESP System Settings**:

| Símbolo | Valor requerido o recomendado |
| --- | --- |
| `CONFIG_ESP_TASK_WDT_EN` | `y`: imprescindible, permite usar TWDT. |
| `CONFIG_ESP_TASK_WDT_INIT` | `y` recomendado. El código también sabe inicializarlo si se deja desactivado. |
| `CONFIG_ESP_TASK_WDT_TIMEOUT_S` | `6`. El programa también fija 6000 ms en ejecución. |
| `CONFIG_ESP_TASK_WDT_PANIC` | `y`. El programa solicita igualmente `trigger_panic = true`. |
| `CONFIG_ESP_SYSTEM_PANIC_PRINT_REBOOT` | `y`: imprimir información y reiniciar después del pánico. |
| `CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0/CPU1` | Se pueden mantener activos; el bucle cede CPU. |

Si venías del perfil de la práctica 2, **volver a activar TWDT**. Desactivarlo al compilar elimina su disponibilidad; llamar a `init()` no lo recupera. El archivo genera un error claro de compilación en ese caso. Los valores de `sdkconfig.defaults` no sobrescriben automáticamente un `sdkconfig` ya existente.

El código distingue inicialización nueva y TWDT ya inicializado: en el segundo caso utiliza `esp_task_wdt_reconfigure()`. Se registra la tarea de `app_main()` con `esp_task_wdt_add(NULL)`. Las tareas idle también quedan vigiladas. Una transición estable durante 20 ms de 1 a 0 alimenta el watchdog; mantener BOOT pulsado no genera nuevas alimentaciones.

### Qué demostrar

1. Arrancar sin tocar BOOT y observar el timeout aproximadamente a los seis segundos, su diagnóstico y un nuevo arranque.
2. Pulsar y soltar BOOT antes del timeout varias veces; el programa continúa.
3. Mantenerlo pulsado; tras la única pulsación aceptada, acabarán transcurriendo seis segundos y se reiniciará.
4. Mostrar tanto el mensaje de TWDT anterior al reset como la causa de arranque impresa después. No interpretar cualquier reset como una prueba de watchdog.

**Realizar esta prueba sin una sesión OpenOCD/JTAG activa.** El depurador puede desactivar los watchdogs y no reactivarlos al continuar. Cerrar la sesión y reiniciar la placa antes de comprobar el timeout. `vTaskDelay()` permite ejecutar otras tareas, pero no alimenta la suscripción de `app_main()`; para eso hace falta `esp_task_wdt_reset()`.

## Ejercicio 5.1: deep sleep y memoria RTC

**Enunciado resumido:** contar décimas desde el inicio, detectar BOOT, entrar en deep sleep, despertar diez segundos después y continuar la cuenta conservada. Registrar la caída de consumo con el instrumento del laboratorio.

**Funcionamiento:** el contador `decimas` tiene el atributo `RTC_SLOW_ATTR`. Al despertar por temporizador se conserva; ante un arranque normal se pone a cero. BOOT tiene antirrebote de 20 ms y se compara con el estado observado al arrancar para no interpretar una pulsación mantenida como una nueva pulsación.

El programa conserva el criterio de la solución previa: **suma 100 décimas por los diez segundos nominales dormidos**. Si se duerme aproximadamente en 3,2 s, al despertar mostrará aproximadamente 13,2 s. La constante `CONTAR_TIEMPO_DORMIDO = 0` permite la variante en la que se retoma en 3,2 s y se cuenta solo el tiempo despierto.

Esta cuenta no es un cronómetro absoluto: se suman diez segundos nominales, no una medición del sueño; también se omiten el arranque y la fracción inferior a una décima al dormir. Mientras está despierto se usa `esp_timer_get_time()` para que imprimir no convierta cada vuelta de bucle en una falsa décima exacta.

Al comienzo se apaga el RGB con el componente ya estudiado: un WS2812 puede mantener un color del firmware anterior. Después se libera el dispositivo RMT. Al entrar en deep sleep, `fflush()` vacía el buffer de C y la función de sueño vacía la salida UART antes de dormir. El regreso no ocurre en la instrucción siguiente: se inicia de nuevo `app_main()`.

**Configuración y comprobación:**

- Usar el USB-UART para observar el terminal; el USB nativo puede desconectarse durante deep sleep.
- No dejar una suscripción explícita de `app_main()` al TWDT del ejercicio anterior: se copia el programa completo, no se mezclan los dos `app_main()`.
- Soltar BOOT después de pulsarlo y comprobar un silencio de unos diez segundos, el mensaje «Despertar por temporizador RTC» y una cuenta que continúa.
- Comparar consumo activo y dormido en el mismo punto de alimentación. El regulador, el puente USB-UART, LEDs y otros componentes de la placa siguen contribuyendo al consumo; el consumo de la placa completa no es el mínimo del chip aislado.

EXT0 se explica en la teoría porque aparece en el PDF, pero **no se necesita en este ejercicio**: BOOT ordena dormir y el temporizador RTC ordena despertar.

## Ejercicio 6.1: UART por líneas y colores

**Enunciado resumido:** recibir una cadena desde el terminal, devolverla al pulsar ENTER y cambiar el LED según R, G o B; con una combinación como `GRIS`, conservar el color.

**Conexión/configuración:** usar el conector USB-UART y el puerto serie correspondiente; **115200, 8N1, sin control de flujo**. El código configura UART0 en TX GPIO43 y RX GPIO44. El USB nativo USB-Serial/JTAG es otro transporte: escribir por ese terminal no entrega automáticamente los bytes a `UART_NUM_0`.

En un terminal serie compatible, escribir y pulsar ENTER. Si la configuración no hace eco local, es normal que no se vea lo escrito hasta que la placa devuelva la línea. Solo un programa debe tener abierto el puerto: cerrar el monitor de VS Code antes de usar otro terminal.

### Regla de interpretación conservada

Se buscan **clases distintas de letras mayúsculas R/G/B** en toda la línea. Se cambia de color si hay exactamente una clase. Otras letras no son colores. Esta interpretación conserva la solución aportada y explica el ejemplo `GRIS`, que contiene G y R.

| Línea recibida | Resultado |
| --- | --- |
| `R` o `RRR` o `ROJO` | Rojo. |
| `G` o `GATO` | Verde. |
| `B` | Azul. |
| `GRIS`, `RG`, `RGB` | Mantiene el color anterior. |
| `HOLA`, `r` o línea vacía | Mantiene el color anterior. |

No es un intérprete de nombres de colores: las palabras no se traducen. Si el profesor exige únicamente comandos exactos de un carácter, habría que cambiar la condición a `strlen(linea) == 1` y comparar ese carácter; esa no es la regla de la solución previa.

**Recepción:** admite ENTER como CR (`\r`), LF (`\n`) o CRLF sin procesar CRLF dos veces. Soporta Backspace/Delete. Acepta un máximo de 127 bytes por línea; si se supera, descarta **toda** la línea hasta ENTER y conserva el color. Así no ejecuta una orden incompleta que ha perdido, por ejemplo, una G situada fuera del buffer. Los caracteres NUL se ignoran porque la entrada es texto.

**Pruebas:** enviar `R`, `G`, `B`, `GRIS`, una línea vacía y varias líneas pegadas juntas; confirmar eco y color. Probar una línea de más de 127 bytes para comprobar el aviso sin cambio de color. Una línea se procesa cuando llega ENTER, nunca por cada bloque parcial de la UART.

## Adaptaciones de las soluciones previas

- Se corrigió la numeración de PCNT y se hicieron todos los archivos copiables directamente a `main/main.c`.
- Se añadieron comprobaciones de errores a las llamadas que devuelven `esp_err_t`.
- PCNT compensa los límites del contador, mide la duración de cada ventana y documenta los huecos entre ellas.
- TWDT mantiene la inicialización compatible con un arranque que ya lo haya creado, añade antirrebote y muestra causa de reset.
- Sleep conserva la suma de diez segundos del original, mide el avance despierto con un reloj y apaga el RGB antes de medir consumo.
- UART conserva el criterio de colores, unifica los finales de línea y rechaza las líneas demasiado largas en lugar de actuar sobre un prefijo truncado.

## Fuentes

- Enunciado aportado: **Práctica 3, Sistemas Empotrados, curso 26/27**, pp. 4–18.
- Soluciones previas de P3, adaptadas y organizadas junto con los enunciados y el material de clase; la tabla inicial indica la correspondencia de archivos.
- [Espressif: PCNT y compensación de desbordamientos](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/peripherals/pcnt.html#compensate-overflow-loss).
- [Espressif: watchdogs y depuración JTAG](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/system/wdts.html).
- [Espressif: modos de sueño y tratamiento de la UART](https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32s3/api-reference/system/sleep_modes.html).
