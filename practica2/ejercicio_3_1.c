/*
 * PRACTICA 2 - EJERCICIO 3.1: senales con dos GPTimer.
 * Copia TODO este archivo a main/main.c.
 * GPIO4: 1 Hz. GPIO5: aproximadamente 120 Hz. Osciloscopio con GND comun.
 * Componentes: esp_driver_gpio, esp_driver_gptimer.
 * Cada callback cambia un nivel: la alarma debe ocurrir cada MEDIO periodo.
 */
#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"
#include "esp_err.h"

#define SALIDA_1_HZ GPIO_NUM_4
#define SALIDA_120_HZ GPIO_NUM_5
#define RESOLUCION_HZ 1000000
#define SEMIPERIODO_1_HZ 500000
#define SEMIPERIODO_120_HZ 4167

/* Solo cada ISR accede a su propio estado; la tarea no lo lee ni lo modifica. */
static int nivel_1_hz = 0;
static int nivel_120_hz = 0;

static bool IRAM_ATTR callback_1_hz(gptimer_handle_t timer,
                                  const gptimer_alarm_event_data_t *evento,
                                  void *datos)
{
    (void)timer;
    (void)evento;
    (void)datos;
    nivel_1_hz = !nivel_1_hz;
    gpio_set_level(SALIDA_1_HZ, nivel_1_hz);
    /* No hacer printf ni esperar dentro de una ISR. No se despierta una tarea. */
    return false;
}

static bool IRAM_ATTR callback_120_hz(gptimer_handle_t timer,
                                    const gptimer_alarm_event_data_t *evento,
                                    void *datos)
{
    (void)timer;
    (void)evento;
    (void)datos;
    nivel_120_hz = !nivel_120_hz;
    gpio_set_level(SALIDA_120_HZ, nivel_120_hz);
    return false;
}

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(SALIDA_1_HZ));
    ESP_ERROR_CHECK(gpio_reset_pin(SALIDA_120_HZ));
    ESP_ERROR_CHECK(gpio_set_direction(SALIDA_1_HZ, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SALIDA_120_HZ, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(SALIDA_1_HZ, 0));
    ESP_ERROR_CHECK(gpio_set_level(SALIDA_120_HZ, 0));

    /* Ambos timers tienen 1 tick = 1 us, pero son unidades independientes. */
    const gptimer_config_t configuracion = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = RESOLUCION_HZ,
    };
    gptimer_handle_t timer_1 = NULL;
    gptimer_handle_t timer_120 = NULL;
    ESP_ERROR_CHECK(gptimer_new_timer(&configuracion, &timer_1));
    ESP_ERROR_CHECK(gptimer_new_timer(&configuracion, &timer_120));

    const gptimer_alarm_config_t alarma_1 = {
        .reload_count = 0,
        .alarm_count = SEMIPERIODO_1_HZ,
        .flags.auto_reload_on_alarm = true,
    };
    const gptimer_alarm_config_t alarma_120 = {
        .reload_count = 0,
        .alarm_count = SEMIPERIODO_120_HZ,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_1, &alarma_1));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_120, &alarma_120));

    const gptimer_event_callbacks_t callbacks_1 = {.on_alarm = callback_1_hz};
    const gptimer_event_callbacks_t callbacks_120 = {.on_alarm = callback_120_hz};
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_1, &callbacks_1, NULL));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_120, &callbacks_120, NULL));

    ESP_ERROR_CHECK(gptimer_enable(timer_1));
    ESP_ERROR_CHECK(gptimer_enable(timer_120));
    ESP_ERROR_CHECK(gptimer_start(timer_1));
    ESP_ERROR_CHECK(gptimer_start(timer_120));

    printf("P2 E3.1: GPIO4=1 Hz; GPIO5=119.9904 Hz (redondeo a 4167 us).\n");
    /* El enunciado exige que el bucle no genere las senales. */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
