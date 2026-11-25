# Práctica 2: teoría, cálculos y preguntas de examen

La única pregunta numerada exclusivamente teórica del enunciado es **3.2**. También se recogen las ampliaciones voluntarias y los conceptos necesarios para defender los programas. Los apartados se basan en el PDF del curso y en las fuentes indicadas al final; los ejemplos numéricos son cálculos, no mediciones de una placa.

## Ejercicio 3.2: cuantos temporizadores hay

**Pregunta:** ¿cuántos temporizadores de propósito general independientes incorpora el ESP32-S3?

**Respuesta:** incorpora **cuatro**, organizados en **dos grupos de dos temporizadores**. Son contadores de **54 bits**, con prescalers de **16 bits**, y admiten cuenta ascendente/descendente, alarmas y recarga automática. En el ejercicio 3.1 se usan dos de esos cuatro recursos. [PDF, pp. 9 y 11; datasheet ESP32-S3.]

No hay que confundirlos con el temporizador del sistema, los watchdog, el reloj del sistema operativo ni el temporizador del periférico LEDC. Un recurso tiene una finalidad y una interfaz concretas; que todos usen relojes no los convierte en el mismo temporizador.

## ADC: conceptos del ejercicio 2.1

### ¿Qué hace el ADC y qué significan sus 12 bits?

Convierte una tensión analógica en un código digital. Con 12 bits existen:

```text
2^12 = 4096 códigos, desde 0 hasta 4095.
```

La **resolución** indica cuántos códigos se distinguen nominalmente. La **exactitud** expresa cuánto se aproxima una medida al valor real. Tener 12 bits no garantiza que todos ellos aporten precisión efectiva ni que no existan ruido, no linealidad o errores de referencia. [PDF, pp. 4-5.]

Para un ADC ideal con rango de entrada de anchura `V_FS`, un escalón nominal vale aproximadamente `V_FS / 4096`. Si se inventa un ejemplo ideal de 0 a 3,3 V, resulta unos **0,806 mV por escalón**. Esa cuenta **no es una calibración del ESP32-S3**. Dividir una lectura por 4095 para hacer coincidir extremos es otro modelo idealizado; no corrige los errores físicos.

### ¿Por qué GPIO7 es canal 6?

Por el mapeo del chip: GPIO1-10 corresponden a ADC1, canales 0-9. Por tanto, GPIO7 corresponde a `ADC_UNIT_1` y `ADC_CHANNEL_6`. La regla «GPIO menos uno» no se debe aplicar indiscriminadamente a ADC2: GPIO11 pertenece a **ADC2 canal 0**, no al canal 10. [PDF, pp. 4-5; datasheet.]

### ¿Qué son `ADC_ATTEN_DB_12` y `ADC_BITWIDTH_DEFAULT`?

La atenuación adapta el rango de entrada del ADC; se utiliza el valor de 12 dB porque lo pide la práctica. `ADC_BITWIDTH_DEFAULT` selecciona el ancho predeterminado del ADC, de 12 bits en este caso. Configurar una atenuación no convierte al pin en tolerante a 5 V. El rango útil de conversión, la alimentación y el máximo absoluto del pin son conceptos diferentes. [PDF, pp. 4-7.]

### ¿Qué significa oneshot?

Cada llamada a `adc_oneshot_read` solicita una conversión y entrega un código. La función espera a que termine esa conversión; no es un flujo continuo por DMA. Se llama desde una tarea y se comprueba su resultado antes de usar el dato. No se realiza la conversión dentro de una ISR. [Espressif, ADC oneshot.]

### ¿Qué mejora el promedio de dos muestras?

La media aritmética es:

```text
media = (muestra_anterior + muestra_actual) / 2.0
Ejemplo: (2030 + 2051) / 2.0 = 2040.5
```

Puede reducir fluctuaciones aleatorias; no corrige un error sistemático de calibración. En esta solución se reutiliza la última muestra, por lo que es una media móvil de longitud 2. Esto también suaviza los cambios rápidos y hace que resultados consecutivos compartan una muestra.

La división entre `2.0` conserva decimales. Dividir dos enteros entre `2` produciría división entera y perdería el `0.5` del ejemplo.

### ¿Cuál es la frecuencia de muestreo?

Se toma una muestra cada 100 ms, además de la inicial, y se publica una media en cada ciclo:

```text
T_muestreo = 0.1 s
f_muestreo = 1 / T_muestreo = 10 muestras/s
```

No son 20 muestras nuevas por segundo: una muestra de cada pareja procede del ciclo anterior. El README original describía otro algoritmo; el README actual y el C ya coinciden.

## Temporizadores: cálculos del ejercicio 3.1

### Frecuencia, periodo y alarma

La relación entre frecuencia y periodo es `f = 1 / T`. Si un callback cambia el GPIO de 0 a 1 o de 1 a 0, hace **medio ciclo**; hay que esperar otro callback para completar el ciclo.

```text
T_alarma = T_senal / 2 = 1 / (2 * f_senal)
cuentas_alarma = resolucion_timer_hz / (2 * f_senal)
```

Con resolución de 1 MHz:

| Señal | Periodo completo | Tiempo entre alarmas | Cuentas |
|---|---:|---:|---:|
| 1 Hz | 1 s | 0,5 s | 500 000 |
| 120 Hz | 8,333… ms | 4,1666… ms | 4 166,666… ideales; se usan 4 167 |

Con 4 167 cuentas se obtiene `1 000 000 / (2 × 4 167) = 119,9904 Hz`. Si se programaran **8 333 µs entre inversiones**, la señal sería aproximadamente **60 Hz**: sería el error de confundir periodo con semiperiodo. [Derivación a partir del enunciado y el modelo de timer, PDF pp. 10-11.]

### ¿Qué hace la recarga automática?

Al alcanzar `alarm_count`, el contador vuelve al valor `reload_count` si `auto_reload_on_alarm` está activado. La alarma se repite sin que el bucle principal tenga que volver a programarla. En este código `reload_count` vale 0. [PDF, pp. 10-11.]

### ¿En qué orden se configura GPTimer?

1. Crear el temporizador con `gptimer_new_timer`.
2. Establecer alarma mediante `gptimer_set_alarm_action`.
3. Registrar el callback mediante `gptimer_register_event_callbacks`.
4. Habilitar con `gptimer_enable`.
5. Arrancar con `gptimer_start`.

El driver mantiene un estado interno; usar una función fuera de su secuencia permitida puede devolver un error. El callback recibe parámetros del driver y devuelve si ha despertado una tarea de mayor prioridad. En este caso devuelve `false`. [PDF, pp. 10-11; Espressif, GPTimer.]

### ¿Por qué no hacemos `printf` o `vTaskDelay` en el callback?

Se ejecuta en contexto de interrupción. Debe terminar rápidamente y no esperar a otras tareas. La impresión puede bloquear o emplear recursos que no sean adecuados para una ISR. Si el trabajo fuera más largo, el callback señalaría a una tarea para que lo hiciera fuera de la interrupción. [PDF, p. 11; reglas de ISR de ESP-IDF.]

## Tick, CPU y timestamp: tres escalas distintas

| Concepto | Valor usado aquí | Significado |
|---|---:|---|
| Reloj de CPU | 80 MHz | Ritmo de funcionamiento del procesador. |
| Tick de FreeRTOS | 1000 Hz | Base del tick del sistema operativo; nominalmente 1 ms. |
| Resolución de GPTimer | 1 MHz | Cada cuenta de ese temporizador representa 1 µs. |
| `esp_timer_get_time()` | Devuelve microsegundos | Tiempo monotónico desde la inicialización del temporizador de ESP-IDF durante el arranque. |

`pdMS_TO_TICKS(100)` convierte milisegundos a ticks del sistema operativo, **no a ciclos de CPU**. Cambiar la CPU a 80 MHz no obliga a convertir manualmente todos los `vTaskDelay`.

**Corrección de una imprecisión del PDF:** `vTaskDelay` bloquea la tarea llamante durante un número de ticks; no mantiene la CPU haciendo una espera activa. El planificador puede ejecutar otras tareas o la tarea idle. El problema de P1 era que la misma tarea dejaba de consultar el botón durante esa espera. GPTimer permite atender eventos sin depender de ese sondeo. [PDF, p. 8, contrastado con FreeRTOS/ESP-IDF.]

`xTaskDelayUntil` calcula la espera a partir de una referencia periódica previa. `vTaskDelay` la calcula respecto al momento en que se llama. Ninguna garantiza por sí sola el instante físico exacto de una lectura o de una impresión.

## Intervalo entre pulsaciones: ejercicio 3.3

Se necesita una marca anterior y otra actual:

```text
intervalo_us = pulsacion_actual_us - pulsacion_anterior_us
intervalo_ms = intervalo_us / 1000.0
```

La primera pulsación solo proporciona una referencia: **todavía no existe intervalo**. Después de imprimir, la pulsación actual pasa a ser la anterior. Así se miden pulsaciones consecutivas, no siempre el tiempo respecto a la primera.

El rebote del contacto produce varios cambios eléctricos para una sola acción física. El código exige 30 ms de estabilidad tanto al pulsar como al soltar. Mantener BOOT a 0 no es un flanco nuevo. El instante registrado procede del sondeo de 5 ms; la resolución de `esp_timer_get_time` no elimina esa limitación. [Enunciado, p. 12; algoritmo de este repositorio.]

## PWM: conceptos del ejercicio 4.1

### ¿Qué cambia al incrementar el duty?

Se mantiene la frecuencia y cambia la fracción del periodo en nivel alto:

```text
T = Ton + Toff
f = 1 / T
D (%) = 100 * Ton / T
```

A 100 Hz, `T = 10 ms`. Para 20 % ideal, `Ton = 2 ms` y `Toff = 8 ms`. Al 80 %, `Ton = 8 ms` y `Toff = 2 ms`. Al 0 y al 100 % ya no hay conmutación de nivel: se mantiene bajo o alto. [PDF, pp. 13-15.]

### ¿Duty es un porcentaje o un número?

El porcentaje es una forma de expresar el objetivo; la función `ledc_set_duty` recibe una **cuenta entera**. Con 10 bits, el periodo se divide en 1024 pasos. Los valores usados son 0, 205, 410, 614 y 819, que aproximan 0-80 %. No se pueden representar exactamente todos los porcentajes con un número finito de pasos.

Un valor de `2^N - 1` no significa matemáticamente un nivel alto continuo: por ejemplo, `1023/1024 ≈ 99,902 %`. Para el 100 % exacto, el código fuerza alto con `ledc_stop(..., 1)`. ESP-IDF también admite una cuenta de `2^N` en ciertas resoluciones, pero hay restricciones cuando se usa la resolución máxima del hardware. Es más importante entender el periodo, el contador y los límites del periférico que memorizar que «máximo siempre equivale a 100 %». [Espressif, LEDC.]

### ¿Por qué se llama a `ledc_update_duty`?

Para aplicar al canal el nuevo duty programado. Se usan las dos llamadas como muestra el enunciado. Un temporizador fija frecuencia y resolución; un canal asocia ese temporizador con el GPIO y su duty. [PDF, pp. 14-15.]

### ¿La tensión en el GPIO se convierte en analógica?

No: sigue alternando entre dos niveles digitales. El brillo medio del LED puede variar porque cambia el tiempo durante el que circula corriente. Un filtro puede extraer una componente media para otras aplicaciones, con limitaciones de rizado y velocidad de respuesta. [PDF, pp. 13 y 16.]

## Ampliacion voluntaria: servomotor RC

El PDF pide investigar el **servo concreto** que se vaya a utilizar. No se ha indicado ningún modelo, por lo que no sería correcto adjudicarle un rango exacto de impulsos.

Como ejemplo de cálculo, **si** la ficha de un servo indicase un periodo de 20 ms y pulsos de 1, 1,5 y 2 ms, la frecuencia sería 50 Hz y los duty correspondientes serían 5 %, 7,5 % y 10 %. Estos valores son una hipótesis didáctica, **no la especificación de tu servo**. Para controlar posición hay que respetar la anchura de impulso y el periodo que indique el fabricante.

La respuesta definitiva debe contener: fabricante/modelo, enlace a su ficha técnica, periodo permitido, anchura mínima, anchura de centro, anchura máxima y alimentación. No deduzcas el rango mecánico a partir de esos valores sin comprobar la ficha. No se incluye un programa que mueva un servo sin conocer esos límites. [Ampliación del PDF, p. 15.]

## Apartado 5: DAC y alternativas

El ESP32-S3 **no incorpora un DAC de tensión general** como el de otros modelos ESP32. Las alternativas propuestas en el PDF son:

| Alternativa | Idea | Limitación principal |
|---|---|---|
| PWM y filtro paso bajo | Obtener una tensión media aproximada, `V_media ≈ D × V_alto`, donde D está entre 0 y 1. | Compromiso entre rizado, rapidez y efecto de la carga. |
| Modulación sigma-delta | Codificar el nivel mediante una secuencia binaria cuya densidad media puede filtrarse. | Requiere comprender el modulador, su filtrado y sus límites; es una ampliación. |
| DAC externo por I²C | Usar, por ejemplo, el MCP4725 citado en el PDF. | Añade un componente, cableado y comunicaciones; sus prestaciones dependen de su ficha. |

Para un filtro RC ideal de primer orden, `fc = 1 / (2πRC)`. Una frecuencia de corte baja respecto a la portadora PWM atenúa la conmutación, pero también ralentiza los cambios útiles. Aumentar la frecuencia PWM puede reducir el filtrado necesario, a cambio del compromiso entre frecuencia y resolución del periférico. [PDF, p. 16; deducción del modelo RC.]

**Reflexión voluntaria del apartado 5.** No todos los microcontroladores incluyen los mismos periféricos: un DAC ocupa área, impone requisitos analógicos y no todas las aplicaciones lo necesitan. Un componente externo permite escoger rango, resolución y precisión para el caso concreto. Esta es una respuesta razonada a la pregunta de diseño; no pretende reproducir el hilo externo enlazado por el PDF.

## Preguntas rápidas para ensayar en voz alta

1. **¿Por qué hay que indicar ADC1 y canal 6, no solamente GPIO7?** El driver configura una unidad y un canal analógico según el mapeo del chip.
2. **¿Qué pasa si se promedian siempre dos veces la misma variable?** No se han tomado dos muestras distintas y no se está filtrando nada.
3. **¿Qué diferencia hay entre muestrear cada 100 ms y esperar 100 ms después de imprimir?** En el segundo caso se acumula el tiempo de trabajo al periodo.
4. **¿Por qué 120 Hz exige unas 240 interrupciones por segundo?** Cada interrupción invierte el nivel y hay dos inversiones por periodo completo.
5. **¿Podrías poner la conmutación en el `while` del ejercicio 3.1?** No cumple el requisito: las señales deben proceder de los callbacks de GPTimer.
6. **¿Qué significa `IRAM_ATTR`?** Pide colocar esa función en RAM de instrucciones. Por sí solo no convierte automáticamente todo lo que llama en seguro cuando la caché está desactivada.
7. **¿Qué obtienes en la primera pulsación?** Una marca temporal; el primer intervalo se obtiene en la segunda.
8. **¿Por qué filtrar también la liberación del botón?** Para que los rebotes al soltar no se interpreten como otra pulsación.
9. **¿Por qué 100 Hz no es 100 %?** Hz mide repeticiones por segundo; porcentaje mide la fracción del periodo en alto.
10. **¿El brillo visual varía linealmente con el porcentaje?** El tiempo de conducción cambia proporcionalmente al duty, pero la percepción de brillo no tiene por qué ser lineal.
11. **¿Por qué un osciloscopio puede no medir frecuencia al 100 %?** No hay flancos periódicos; la salida está permanentemente alta.
12. **¿Desactivar TWDT implica desactivar IWDT?** No, son mecanismos distintos. El perfil de P2 solo desactiva el de tareas.

## Qué deberías dominar del código

- `ejercicio_2_1.c`: secuencia unidad → canal → lectura; media móvil; unidades de timestamp.
- `ejercicio_3_1.c`: cálculo del semiperiodo, recarga, registro del callback y diferencia entre ISR y tarea.
- `ejercicio_3_3.c`: estado anterior, estado estable, flanco y dos marcas de tiempo.
- `ejercicio_4_1.c`: timer frente a canal LEDC, porcentaje frente a cuentas y vuelta de 100 a 0 %.

## Fuentes

- **Práctica 2, Sistemas Empotrados, Francisco Javier Burgoa, curso 26-27**, PDF aportado: pp. 3-7 (configuración y ADC), pp. 8-12 (temporización), pp. 13-15 (PWM/servo) y p. 16 (DAC).
- [Soluciones originales, commit `0215954584839e431d32918761647f8f2685cafa`](https://github.com/Alvorak/SistemasEmpotrados/tree/0215954584839e431d32918761647f8f2685cafa/Practica2).
- [ESP32-S3 Datasheet, Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf).
- [ADC oneshot, Espressif](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/adc_oneshot.html).
- [GPTimer, Espressif](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/gptimer.html).
- [LEDC, Espressif](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/peripherals/ledc.html).
- [Implementación LEDC, ESP-IDF v5.5.1](https://github.com/espressif/esp-idf/blob/v5.5.1/components/esp_driver_ledc/src/ledc.c): `ledc_stop` y reactivación de la salida en `ledc_update_duty`.
- [FreeRTOS: funciones de tareas en ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/release-v5.5/esp32s3/api-reference/system/freertos_idf.html).
