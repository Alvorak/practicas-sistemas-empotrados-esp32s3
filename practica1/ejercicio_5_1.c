/* Practica 1 - 5.1: GPIO4 cambia cada 3 s; BOOT alterna GPIO5 por interrupcion.
 * Copia completo a main/main.c. Dependencias: esp_driver_gpio, esp_timer.
 * FreeRTOS ya viene con ESP-IDF. La tarea auxiliar solo rearma el boton:
 * la ISR hace el cambio inmediato; despues espera una liberacion estable.
 */
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_timer.h"

#define LED_ROJO   GPIO_NUM_4
#define LED_VERDE  GPIO_NUM_5
#define BOTON_BOOT GPIO_NUM_0
#define REBOTE_US  30000

static TaskHandle_t tarea_rearme = NULL;
// Solo la ISR modifica este estado; no hace falta compartirlo con main.
static int estado_verde = 0;

static void IRAM_ATTR boton_isr(void *arg)
{
    (void)arg;
    // Ignorar los rebotes hasta que la tarea detecte que se ha soltado BOOT.
    gpio_intr_disable(BOTON_BOOT);
    estado_verde = !estado_verde;
    gpio_set_level(LED_VERDE, estado_verde);

    // Notificacion: avisa a la tarea sin esperar ni hacer printf en la ISR.
    BaseType_t despertar = pdFALSE;
    vTaskNotifyGiveFromISR(tarea_rearme, &despertar);
    if (despertar == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void rearmar_boton(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        int64_t libre_desde = -1;
        while (1) {
            int64_t ahora = esp_timer_get_time();
            if (gpio_get_level(BOTON_BOOT) == 0) {
                libre_desde = -1;
            } else if (libre_desde < 0) {
                libre_desde = ahora;
            } else if (ahora - libre_desde >= REBOTE_US) {
                break;
            }
            // Espera de una tarea, nunca de la ISR. GPIO4 sigue independiente.
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ESP_ERROR_CHECK(gpio_intr_enable(BOTON_BOOT));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(LED_ROJO));
    ESP_ERROR_CHECK(gpio_reset_pin(LED_VERDE));
    ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, 1));
    ESP_ERROR_CHECK(gpio_set_level(LED_VERDE, 0));
    ESP_ERROR_CHECK(gpio_set_direction(LED_ROJO, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT));

    const gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOTON_BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));
    // Crear la tarea ANTES de habilitar su productor de notificaciones.
    if (xTaskCreate(rearmar_boton, "rearme_boot", 2048, NULL, 5,
                    &tarea_rearme) != pdPASS) {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOTON_BOOT, boton_isr, NULL));

    printf("5.1: BOOT alterna GPIO5 sin esperar los 3 s de GPIO4.\n");
    int nivel_rojo = 1;
    while (1) {
        nivel_rojo = !nivel_rojo;
        ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, nivel_rojo));
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
