/*
 * PRACTICA 2 - EJERCICIO 3.3: intervalo entre pulsaciones validas de BOOT.
 * Copia TODO este archivo a main/main.c. BOOT = GPIO0, activo a nivel bajo.
 * Componentes: esp_driver_gpio, esp_timer. Sin colas ni tareas adicionales.
 * Se usa sondeo: el enunciado no exige ISR en este ejercicio.
 * Precision de deteccion: limitada por el sondeo de 5 ms y el planificador.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"

#define BOOT GPIO_NUM_0
#define ANTIRREBOTE_US 30000
#define SONDEO_MS 5

void app_main(void)
{
    const gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));

    int ultima_lectura = gpio_get_level(BOOT);
    int estado_estable = ultima_lectura;
    int64_t cambio_us = esp_timer_get_time();
    int64_t pulsacion_anterior_us = 0;
    bool hay_pulsacion_anterior = false;
    unsigned numero_pulsacion = 0;

    printf("P2 E3.3: suelta BOOT y pulsa dos veces para medir un intervalo.\n");
    while (1) {
        const int lectura = gpio_get_level(BOOT);
        const int64_t ahora_us = esp_timer_get_time();

        if (lectura != ultima_lectura) {
            ultima_lectura = lectura;
            cambio_us = ahora_us;
        }

        /* Aceptamos un estado solo tras 30 ms sin cambios. Se filtran tanto
         * la pulsacion como la liberacion: un rebote al soltar no suma otra. */
        if (lectura != estado_estable && ahora_us - cambio_us >= ANTIRREBOTE_US) {
            estado_estable = lectura;
            if (estado_estable == 0) {
                ++numero_pulsacion;
                /* Usamos la marca del flanco observado, no la del printf.
                 * El flanco solo se acepta despues de validar el antirrebote. */
                if (hay_pulsacion_anterior) {
                    const double intervalo_ms =
                        (cambio_us - pulsacion_anterior_us) / 1000.0;
                    printf("Pulsacion %u | intervalo=%.3f ms\n",
                           numero_pulsacion, intervalo_ms);
                } else {
                    printf("Primera pulsacion registrada; todavia no hay intervalo.\n");
                    hay_pulsacion_anterior = true;
                }
                pulsacion_anterior_us = cambio_us;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SONDEO_MS));
    }
}
