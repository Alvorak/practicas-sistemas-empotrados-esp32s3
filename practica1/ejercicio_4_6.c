/* Practica 1 - 4.6: detectar flanco de bajada por polling y alternar GPIO5.
 * Copia completo a main/main.c. Dependencia: esp_driver_gpio.
 * Se filtran pulsacion y liberacion: el nivel debe durar 30 ms para aceptarlo.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define LED_VERDE  GPIO_NUM_5
#define BOTON_BOOT GPIO_NUM_0
#define REBOTE_MS  30

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(LED_VERDE));
    ESP_ERROR_CHECK(gpio_set_level(LED_VERDE, 0));
    ESP_ERROR_CHECK(gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_reset_pin(BOTON_BOOT));
    ESP_ERROR_CHECK(gpio_set_direction(BOTON_BOOT, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(BOTON_BOOT, GPIO_PULLUP_ONLY));

    int estado_led = 0;
    int ultima_lectura = gpio_get_level(BOTON_BOOT);
    int estado_estable = ultima_lectura;
    TickType_t ultimo_cambio = xTaskGetTickCount();

    printf("4.6: cada nueva pulsacion alterna GPIO5.\n");
    while (1) {
        int lectura = gpio_get_level(BOTON_BOOT);
        TickType_t ahora = xTaskGetTickCount();
        if (lectura != ultima_lectura) {
            ultima_lectura = lectura;
            ultimo_cambio = ahora;
        }

        // La resta de ticks sin signo sigue funcionando al desbordar el contador.
        if (lectura != estado_estable &&
            (TickType_t)(ahora - ultimo_cambio) >= pdMS_TO_TICKS(REBOTE_MS)) {
            int anterior = estado_estable;
            estado_estable = lectura;
            if (anterior == 1 && estado_estable == 0) {
                estado_led = !estado_led;
                ESP_ERROR_CHECK(gpio_set_level(LED_VERDE, estado_led));
                printf("BOOT: GPIO5 %s\n", estado_led ? "encendido" : "apagado");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
