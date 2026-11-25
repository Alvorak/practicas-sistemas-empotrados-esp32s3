/* Practica 1 - 4.4: GPIO4 cambia cada 100 ms; GPIO5 sigue BOOT.
 * Copia el archivo completo a main/main.c. Dependencia: esp_driver_gpio.
 * PDF: rojo/verde. Repositorio original: blanco/azul. Mismos pines y polaridad.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define LED_ROJO   GPIO_NUM_4  // Activo en bajo: 3V3 -> resistencia -> LED -> GPIO4.
#define LED_VERDE  GPIO_NUM_5  // Activo en alto: GPIO5 -> resistencia -> LED -> GND.
#define BOTON_BOOT GPIO_NUM_0  // Suelto = 1; pulsado = 0.

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

    printf("4.4: GPIO4 cambia cada 100 ms; GPIO5 sigue BOOT.\n");
    int nivel_rojo = 1;
    while (1) {
        nivel_rojo = !nivel_rojo;
        ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, nivel_rojo));

        int pulsado = (gpio_get_level(BOTON_BOOT) == 0);
        ESP_ERROR_CHECK(gpio_set_level(LED_VERDE, pulsado));

        // Esta tarea no lee BOOT durante la espera; otras tareas/ISR si corren.
        // 100 ms encendido + 100 ms apagado = periodo de 200 ms, aprox. 5 Hz.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
