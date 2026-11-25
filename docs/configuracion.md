# Configuración del proyecto

## Valores iniciales

`sdkconfig.defaults` proporciona los valores de partida al generar `sdkconfig` por primera vez. **Si `sdkconfig` ya existe, sus valores tienen prioridad.** Copiar otro ejercicio a `main.c` no reinicia la configuración.

| Opción | Valor inicial | Motivo |
|---|---|---|
| Target | `esp32s3` | Microcontrolador de las prácticas. |
| `CONFIG_FREERTOS_HZ` | `1000` | Un tick nominal de 1 ms; ajuste pedido por P2. |
| `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ` | `80` | Frecuencia pedida por P2. |
| Consola | UART0, 115200 baudios | Monitor y recepción de P3.6.1 mediante USB-UART. |
| Flash | 4 MB | Configuración conservadora, suficiente para estos programas. |
| PSRAM | Desactivada | Ningún ejercicio la necesita. |
| Optimización | Debug (`-Og`) | Facilita la depuración de P1.6.2. |
| `CONFIG_ESP_TASK_WDT_EN` | Activado | Permite el ejercicio del watchdog. |
| `CONFIG_ESP_TASK_WDT_INIT` | Activado | ESP-IDF inicia el TWDT durante el arranque. |
| `CONFIG_ESP_TASK_WDT_TIMEOUT_S` | `6` | Valor de P3.4.1; ese ejemplo también lo configura en ejecución. |
| `CONFIG_ESP_TASK_WDT_PANIC` | Activado | El vencimiento invoca el manejador de pánico. |
| Comportamiento del pánico | Imprimir y reiniciar | Permite observar el error antes del reinicio. |
| Watchdog de interrupciones, IWDT | Activado | Es independiente del watchdog de tareas. |

El módulo N16R8 que aparece en el enunciado dispone de más flash que los 4 MB declarados aquí. Puedes seleccionar 16 MB si verificas que tu módulo es efectivamente N16R8. No es necesario activar PSRAM para estas prácticas. El modelo/revisión de la placa también determina si el RGB está en GPIO48 o en otro pin: comprueba su esquema o serigrafía.

## Cambiar opciones en VS Code / terminal

Abre una terminal ESP-IDF en la raíz y ejecuta:

```bash
idf.py menuconfig
```

También puedes usar el editor de configuración de ESP-IDF de VS Code. En `menuconfig`, la búsqueda `/` por nombre de símbolo evita depender de cómo esté organizado el menú en tu versión.

Para la práctica 2, comprueba:

| Búsqueda | Ajuste |
|---|---|
| `FREERTOS_HZ` | 1000 Hz. |
| `ESP_DEFAULT_CPU_FREQ_MHZ` | 80 MHz. |
| `ESP_TASK_WDT_EN` | Desactivado para reproducir la indicación del PDF. |

El enunciado solo dice «deshabilita el Watchdog»; no especifica si habla del de tareas o del de interrupciones. Aquí se interpreta como **TWDT**, y se conserva **IWDT**. Confirma esa interpretación si el profesor pide expresamente otra cosa. Los programas de P2 ceden la CPU y no necesitan desactivar IWDT.

Al volver a P3.4.1, activa `ESP_TASK_WDT_EN` y `ESP_TASK_WDT_INIT`, fija 6 segundos, activa `ESP_TASK_WDT_PANIC` y selecciona imprimir/reiniciar para el pánico. Guarda, compila y flashea de nuevo. El ejemplo acepta que TWDT ya esté inicializado: usa `esp_task_wdt_reconfigure()` en ese caso. No pruebes el vencimiento bajo una sesión OpenOCD/JTAG, que puede desactivar los watchdogs.

## Perfil opcional de P2 sin alterar tu configuración normal

Si quieres conservar dos configuraciones, existe `config/sdkconfig.practica2.defaults`. Esta alternativa se utiliza desde terminal; los botones habituales de VS Code seguirán usando `build/` y `sdkconfig`, salvo que configures la extensión de otro modo.

Con `sdkconfig.p2` todavía inexistente:

```powershell
idf.py -B build_p2 -D SDKCONFIG=sdkconfig.p2 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.practica2.defaults" build
idf.py -B build_p2 -D SDKCONFIG=sdkconfig.p2 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.practica2.defaults" -p COM5 flash monitor
```

Las comillas mantienen el punto y coma dentro del argumento tanto en PowerShell como en Bash. Si ya habías generado `sdkconfig.p2`, ajusta sus opciones con la misma selección:

```powershell
idf.py -B build_p2 -D SDKCONFIG=sdkconfig.p2 -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/sdkconfig.practica2.defaults" menuconfig
```

Para regresar a la configuración normal:

```powershell
idf.py -B build -D SDKCONFIG=sdkconfig -D SDKCONFIG_DEFAULTS=sdkconfig.defaults build
idf.py -B build -D SDKCONFIG=sdkconfig -D SDKCONFIG_DEFAULTS=sdkconfig.defaults -p COM5 flash monitor
```

Para empezar, el flujo de un solo `sdkconfig` con `menuconfig` es suficiente. No necesitas usar esta alternativa para copiar y ejecutar los ejercicios.

## Componentes y cabeceras

`main/CMakeLists.txt` ya incluye todos estos componentes. No tienes que cambiarlo al pasar de ejercicio.

| Cabecera / funciones | Componente |
|---|---|
| `freertos/FreeRTOS.h`, tareas y colas | `freertos` |
| `driver/gpio.h` | `esp_driver_gpio` |
| `driver/gptimer.h` | `esp_driver_gptimer` |
| `driver/ledc.h` | `esp_driver_ledc` |
| `driver/pulse_cnt.h` | `esp_driver_pcnt` |
| `driver/uart.h` | `esp_driver_uart` |
| `esp_adc/adc_oneshot.h` | `esp_adc` |
| `esp_timer.h` | `esp_timer` |
| `esp_task_wdt.h`, `esp_system.h` | `esp_system` |
| `esp_sleep.h` | `esp_hw_support` |
| `esp_err.h`, atributos comunes | `esp_common` |
| `led_strip.h` | `espressif__led_strip`, descargado por el manifiesto |

Los drivers son los de ESP-IDF que presentan los laboratorios. `led_strip` es el único componente externo al SDK solicitado por el proyecto y es precisamente el que introduce la práctica 3. Las colas y tareas adicionales que aparecen en algún ejercicio son de FreeRTOS, ya incluido en ESP-IDF; sus comentarios explican qué trabajo sacan de una ISR.

## Problemas frecuentes

| Síntoma | Qué revisar |
|---|---|
| `idf.py` no se reconoce | Abre la terminal del entorno ESP-IDF; una terminal genérica puede no tenerlo activado. |
| Varias definiciones de `app_main` | Mantén `SRCS "main.c"`; los ejemplos no se añaden a CMake. |
| No encuentra `led_strip.h` | Revisa el manifiesto, Internet y Component Manager; no cambies al azar la cabecera. |
| Errores sobre `esp_driver_*` | Comprueba la versión: este proyecto no usa las APIs de ESP-IDF 4.x. |
| El watchdog no reinicia | Reactiva TWDT, comprueba pánico/reinicio y cierra OpenOCD. |
| P3.6.1 muestra mensajes pero no recibe | Usa el puerto del puente USB-UART con UART0, no únicamente el USB nativo. |
| Se queda en modo descarga | Suelta BOOT y pulsa RESET; BOOT está conectado a un pin de arranque. |
| El LED RGB no responde | Confirma el GPIO y la alimentación del LED en tu revisión de placa. |
| Cambiar `sdkconfig.defaults` no hace efecto | Modifica `sdkconfig` mediante `menuconfig`; los defaults no sobrescriben opciones existentes. |

## Referencias

- Enunciado P2, pág. 3; P3, págs. 8-12 y 16-18.
- [Configuración ESP-IDF 5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/kconfig.html).
- [Sistema de construcción](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-guides/build-system.html).
- [Watchdogs y efecto de JTAG](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/api-reference/system/wdts.html).
