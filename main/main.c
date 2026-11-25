/*
 * Practica 3, ejercicio 3.1: rojo, verde, azul y blanco, dos segundos cada uno.
 * Copiar TODO a main/main.c. Componente oficial: espressif/led_strip 3.0.0.
 * El WS2812 de la placa del laboratorio recibe datos por GPIO48.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "led_strip.h"

#define LED_RGB_GPIO 48
#define INTENSIDAD 150 // Valor 0..255. Menos brillo/consumo, no una proteccion electrica.

void app_main(void)
{
    led_strip_config_t configuracion_led = {
        .strip_gpio_num = LED_RGB_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        // Orden de bytes que espera el LED. set_pixel() sigue recibiendo R,G,B.
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t configuracion_rmt = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, // 10 MHz: una unidad RMT dura 0,1 us.
        .mem_block_symbols = 64,
        .flags.with_dma = false, // Para un solo LED no necesitamos DMA.
    };
    led_strip_handle_t led = NULL;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&configuracion_led,
                                            &configuracion_rmt, &led));

    while (1) {
        // set_pixel prepara el color; refresh transmite los pulsos con RMT.
        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, INTENSIDAD, 0, 0));
        ESP_ERROR_CHECK(led_strip_refresh(led));
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 0, INTENSIDAD, 0));
        ESP_ERROR_CHECK(led_strip_refresh(led));
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, 0, 0, INTENSIDAD));
        ESP_ERROR_CHECK(led_strip_refresh(led));
        vTaskDelay(pdMS_TO_TICKS(2000));

        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, INTENSIDAD, INTENSIDAD, INTENSIDAD));
        ESP_ERROR_CHECK(led_strip_refresh(led));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
