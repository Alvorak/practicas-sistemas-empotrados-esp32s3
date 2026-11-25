/*
 * Practica 3, ejercicio 5.1: BOOT -> deep sleep 10 s -> continuar contador RTC.
 * Copiar TODO a main/main.c. Ver README para medir consumo por USB-UART.
 * Se suman los 10 s nominales dormidos, como en la solucion original.
 * Es un contador didactico, no un reloj civil calibrado.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "led_strip.h"

#define BOOT_GPIO GPIO_NUM_0
#define LED_RGB_GPIO 48
#define SLEEP_TIME_SEC 10
#define CONTAR_TIEMPO_DORMIDO 1 // 0: el contador se pausa durante el deep sleep.
#define REBOTE_US 20000

// Esta RAM se conserva durante deep sleep; no es memoria permanente sin energia.
RTC_SLOW_ATTR uint64_t decimas = 0;

static void apagar_led_rgb(void)
{
    // Un WS2812 puede conservar el ultimo color del ejercicio anterior.
    // Lo apagamos para que no falsee la comparacion de consumo.
    led_strip_config_t configuracion_led = {
        .strip_gpio_num = LED_RGB_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    };
    led_strip_rmt_config_t configuracion_rmt = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };
    led_strip_handle_t led = NULL;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&configuracion_led,
                                            &configuracion_rmt, &led));
    ESP_ERROR_CHECK(led_strip_clear(led));
    ESP_ERROR_CHECK(led_strip_del(led)); // Liberar RMT: no lo necesitamos durante la cuenta.
}

void app_main(void)
{
    const esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
    if (causa == ESP_SLEEP_WAKEUP_TIMER) {
#if CONTAR_TIEMPO_DORMIDO
        decimas += SLEEP_TIME_SEC * 10ULL;
#endif
        printf("\nDespertar por temporizador RTC. Se conserva el contador.\n");
    } else {
        decimas = 0;
        printf("\nArranque normal: contador a cero.\n");
    }

    apagar_led_rgb();

    gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOOT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_TIME_SEC * 1000000ULL));

    // Leer el estado inicial evita interpretar BOOT mantenido como nueva pulsacion.
    // Hay que soltarlo y volverlo a pulsar para repetir el ejercicio.
    int estable = gpio_get_level(BOOT_GPIO);
    int candidato = estable;
    int64_t ultimo_avance = esp_timer_get_time();
    int64_t desde_cambio = ultimo_avance;
    uint64_t ultima_mostrada = UINT64_MAX;

    while (1) {
        const int64_t ahora = esp_timer_get_time();
        // No contamos vueltas del bucle: usamos tiempo medido y evitamos deriva
        // acumulada por printf/vTaskDelay durante cada tramo despierto.
        const uint64_t nuevas_decimas = (uint64_t)((ahora - ultimo_avance) / 100000);
        decimas += nuevas_decimas;
        ultimo_avance += (int64_t)nuevas_decimas * 100000;

        if (decimas != ultima_mostrada) {
            printf("Tiempo acumulado: %" PRIu64 ".%" PRIu64 " s\n", decimas / 10, decimas % 10);
            ultima_mostrada = decimas;
        }

        const int nivel = gpio_get_level(BOOT_GPIO);
        if (nivel != candidato) {
            candidato = nivel;
            desde_cambio = ahora;
        }
        if (candidato != estable && ahora - desde_cambio >= REBOTE_US) {
            const int anterior = estable;
            estable = candidato;
            if (anterior == 1 && estable == 0) {
                printf("BOOT: entrando en deep sleep durante %d s.\n", SLEEP_TIME_SEC);
                fflush(stdout); // Vaciar buffer de C; deep_sleep_start espera las UART.
                esp_deep_sleep_start();
                // No se continua aqui: al despertar vuelve a ejecutarse app_main().
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
