/* Practica 1 - 4.5: observar la perdida de respuesta con polling cada 3 s.
 * Copia completo a main/main.c. Dependencia: esp_driver_gpio.
 * Se conserva a proposito el problema que pide observar el enunciado.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define LED_ROJO   GPIO_NUM_4  // Activo en bajo (blanco en el montaje original).
#define LED_VERDE  GPIO_NUM_5  // Activo en alto (azul en el montaje original).
#define BOTON_BOOT GPIO_NUM_0

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(LED_ROJO));
    ESP_ERROR_CHECK(gpio_reset_pin(LED_VERDE));
    ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, 1));
    ESP_ERROR_CHECK(gpio_set_level(LED_VERDE, 0));
    ESP_ERROR_CHECK(gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_reset_pin(BOTON_BOOT));
    ESP_ERROR_CHECK(gpio_set_direction(BOTON_BOOT, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(BOTON_BOOT, GPIO_PULLUP_ONLY));

    printf("4.5: prueba pulsaciones cortas y largas durante cada espera.\n");
    int nivel_rojo = 1;
    while (1) {
        nivel_rojo = !nivel_rojo;
        ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, nivel_rojo));
        ESP_ERROR_CHECK(gpio_set_level(LED_VERDE,
                                      gpio_get_level(BOTON_BOOT) == 0));

        // El texto pide 3 segundos: 3000, aunque el PDF muestra 1000 por error.
        // BOOT solo se consulta cada 3 s. Una pulsacion corta puede perderse.
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
