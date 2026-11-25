/*
 * Practica 3, ejercicio 4.1: TWDT de 6 s; BOOT alimenta una vez por pulsacion.
 * Copiar TODO a main/main.c. Ver README: activar TWDT y panic con reinicio.
 * Probar SIN sesion de depuracion OpenOCD/JTAG, que altera los watchdogs.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#if !defined(CONFIG_ESP_TASK_WDT_EN)
#error "Ejercicio 4.1: activa CONFIG_ESP_TASK_WDT_EN; consulta practica3/README.md."
#endif

#define BOOT_GPIO GPIO_NUM_0
#define REBOTE_US 20000
#define TIMEOUT_MS 6000

static const char *causa_reset(esp_reset_reason_t causa)
{
    switch (causa) {
    case ESP_RST_POWERON: return "encendido";
    case ESP_RST_TASK_WDT: return "watchdog de tarea (TWDT)";
    case ESP_RST_INT_WDT: return "watchdog de interrupciones (IWDT)";
    case ESP_RST_PANIC: return "panic; consultar tambien el mensaje anterior al reset";
    case ESP_RST_SW: return "reinicio por software";
    default: return "otra causa";
    }
}

void app_main(void)
{
    const esp_reset_reason_t causa = esp_reset_reason();
    printf("\nCausa de arranque: %s (codigo %d).\n", causa_reset(causa), (int)causa);

    gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOOT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));

    esp_task_wdt_config_t vigilancia = {
        .timeout_ms = TIMEOUT_MS,
        // Vigilar tambien las tareas idle de los nucleos presentes.
        // Nuestro vTaskDelay permite que esas tareas se ejecuten.
        .idle_core_mask = (1U << portNUM_PROCESSORS) - 1U,
        .trigger_panic = true,
    };
    esp_err_t resultado = esp_task_wdt_init(&vigilancia);
    if (resultado == ESP_ERR_INVALID_STATE) {
        // ESP-IDF puede haber inicializado el TWDT durante el arranque.
        ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&vigilancia));
    } else {
        ESP_ERROR_CHECK(resultado);
    }
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL)); // NULL identifica la tarea actual.

    int estable = gpio_get_level(BOOT_GPIO);
    int candidato = estable;
    int64_t desde_cambio = esp_timer_get_time();
    const int64_t inicio = desde_cambio;
    int64_t ultima_decima = -1;
    printf("Pulsa y suelta BOOT antes de 6 s; mantenerlo pulsado no alimenta repetidamente.\n");

    while (1) {
        const int64_t ahora = esp_timer_get_time();
        const int nivel = gpio_get_level(BOOT_GPIO);

        // Una transicion solo se acepta tras 20 ms sin cambios (antirrebote).
        if (nivel != candidato) {
            candidato = nivel;
            desde_cambio = ahora;
        }
        if (candidato != estable && ahora - desde_cambio >= REBOTE_US) {
            const int anterior = estable;
            estable = candidato;
            if (anterior == 1 && estable == 0) {
                ESP_ERROR_CHECK(esp_task_wdt_reset());
                printf("BOOT: watchdog alimentado.\n");
            }
        }

        const int64_t decimas = (ahora - inicio) / 100000;
        if (decimas != ultima_decima) {
            printf("Tiempo: %" PRId64 ".%" PRId64 " s\n", decimas / 10, decimas % 10);
            ultima_decima = decimas;
        }

        // Ceder CPU NO alimenta la suscripcion de esta tarea al TWDT.
        // Solo la pulsacion valida ejecuta esp_task_wdt_reset().
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
