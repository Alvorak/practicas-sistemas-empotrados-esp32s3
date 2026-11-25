/* Practica 1 - 6.1: ejercicio 5.1 + numero y timestamp de cada pulsacion.
 * Copia completo a main/main.c. Dependencias: esp_driver_gpio, esp_timer.
 * Conserva la cola FreeRTOS del repositorio original: lleva una COPIA del
 * evento desde la ISR a la tarea del terminal. No se imprime desde la ISR.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_timer.h"

#define LED_ROJO   GPIO_NUM_4
#define LED_VERDE  GPIO_NUM_5
#define BOTON_BOOT GPIO_NUM_0
#define REBOTE_US  30000

typedef struct {
    uint32_t numero;
    int64_t instante_us;
    int estado_led;
} evento_boton_t;

static QueueHandle_t cola_boton;
// Estas dos variables solo las lee/escribe la ISR. La tarea recibe una copia.
static uint32_t contador = 0;
static int estado_verde = 0;

static void IRAM_ATTR boton_isr(void *arg)
{
    (void)arg;
    gpio_intr_disable(BOTON_BOOT);
    estado_verde = !estado_verde;
    gpio_set_level(LED_VERDE, estado_verde);
    const evento_boton_t evento = {
        .numero = ++contador,
        .instante_us = esp_timer_get_time(),
        .estado_led = estado_verde,
    };

    BaseType_t despertar = pdFALSE;
    // Con BOOT deshabilitado hasta consumir el evento solo hay uno pendiente.
    // La rama de error evita dejar BOOT deshabilitado si la cola se llenara.
    if (xQueueSendFromISR(cola_boton, &evento, &despertar) != pdTRUE) {
        gpio_intr_enable(BOTON_BOOT);
    }
    if (despertar == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void tarea_terminal(void *arg)
{
    (void)arg;
    evento_boton_t evento;
    while (1) {
        if (xQueueReceive(cola_boton, &evento, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        printf("BOOT # %" PRIu32 " | t = %" PRId64 " us | GPIO5 = %d\n",
               evento.numero, evento.instante_us, evento.estado_led);

        // Una nueva pulsacion solo se acepta tras soltar durante 30 ms.
        // Asi tampoco se cuentan rebotes al SOLTAR un boton mantenido.
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

    cola_boton = xQueueCreate(4, sizeof(evento_boton_t));
    if (cola_boton == NULL) {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
    if (xTaskCreate(tarea_terminal, "terminal_boot", 4096, NULL, 5,
                    NULL) != pdPASS) {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    const gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOTON_BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BOTON_BOOT, boton_isr, NULL));

    printf("6.1: contador + timestamp en us; terminal a 115200 baudios.\n");
    int nivel_rojo = 1;
    while (1) {
        nivel_rojo = !nivel_rojo;
        ESP_ERROR_CHECK(gpio_set_level(LED_ROJO, nivel_rojo));
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
