/*
 * PRACTICA 2 - EJERCICIO 2.1: ADC y media de dos muestras.
 * Copia TODO este archivo a main/main.c; no se compila junto con otros ejercicios.
 * Montaje: extremos del potenciometro a 3V3 y GND; cursor a GPIO7 (ADC1_CH6).
 * Componentes ESP-IDF: esp_adc, esp_timer. No necesita una libreria externa.
 * Se conserva la media movil del C original del repositorio de clase.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_timer.h"

#define ADC_UNIDAD ADC_UNIT_1
#define ADC_CANAL ADC_CHANNEL_6
#define PERIODO_MS 100

void app_main(void)
{
    adc_oneshot_unit_handle_t adc = NULL;
    const adc_oneshot_unit_init_cfg_t unidad = {
        .unit_id = ADC_UNIDAD,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unidad, &adc));

    const adc_oneshot_chan_cfg_t canal = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT, /* 12 bits en el ESP32-S3. */
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, ADC_CANAL, &canal));

    printf("P2 E2.1: GPIO7/ADC1_CH6. Media movil de dos muestras, cada 100 ms.\n");
    printf("Los valores son codigos ADC, no voltios calibrados.\n");

    int anterior = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc, ADC_CANAL, &anterior));
    TickType_t ultimo_despertar = xTaskGetTickCount();

    while (1) {
        /* FreeRTOS: espera hasta el siguiente instante periodico. A diferencia
         * de vTaskDelay, no suma el tiempo de lectura/printf al periodo. */
        xTaskDelayUntil(&ultimo_despertar, pdMS_TO_TICKS(PERIODO_MS));

        int actual = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc, ADC_CANAL, &actual));
        const int64_t tiempo_us = esp_timer_get_time();
        const double media = (anterior + actual) / 2.0;

        printf("t=%" PRId64 " us | anterior=%d | actual=%d | media=%.1f\n",
               tiempo_us, anterior, actual, media);

        /* La muestra actual se reutiliza como anterior en el siguiente ciclo.
         * Cada media combina dos instantes separados aproximadamente 100 ms. */
        anterior = actual;
    }
}
