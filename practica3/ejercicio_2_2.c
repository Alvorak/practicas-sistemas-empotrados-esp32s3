/*
 * Practica 3, ejercicio 2.2: medir frecuencia con el contador hardware PCNT.
 * Copiar TODO este archivo a main/main.c. El ejercicio 2.1 es teorico.
 * Generador: salida 0..3,3 V a GPIO4 y GND comun. Desconectar el LED rojo.
 * Drivers: esp_driver_pcnt, esp_timer; ventana nominal de un segundo.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"
#include "esp_timer.h"

#define ENTRADA_PULSOS 4
#define LIMITE_SUPERIOR 32767
#define LIMITE_INFERIOR (-32768)

void app_main(void)
{
    pcnt_unit_config_t configuracion = {
        .high_limit = LIMITE_SUPERIOR,
        .low_limit = LIMITE_INFERIOR,
        // El driver suma los desbordamientos al resultado de get_count().
        // Es una funcion del mismo PCNT; no es una libreria adicional.
        .flags.accum_count = true,
    };
    pcnt_unit_handle_t contador = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&configuracion, &contador));

    pcnt_glitch_filter_config_t filtro = {
        .max_glitch_ns = 1000, // Descarta impulsos de menos de aproximadamente 1 us.
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(contador, &filtro));

    pcnt_chan_config_t configuracion_canal = {
        .edge_gpio_num = ENTRADA_PULSOS,
        .level_gpio_num = -1, // No usamos una segunda entrada de direccion.
    };
    pcnt_channel_handle_t canal = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(contador, &configuracion_canal, &canal));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(canal,
                      PCNT_CHANNEL_EDGE_ACTION_HOLD,
                      PCNT_CHANNEL_EDGE_ACTION_INCREASE)); // Solo bajadas: 1 por periodo.
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(canal,
                      PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                      PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    // Necesarios para que accum_count contabilice cada llegada a los limites.
    // La ISR interna del driver hace esa suma; no contamos cada pulso en C.
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(contador, LIMITE_SUPERIOR));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(contador, LIMITE_INFERIOR));
    ESP_ERROR_CHECK(pcnt_unit_enable(contador));

    printf("PCNT: entrada GPIO%d; un flanco de bajada por periodo.\n", ENTRADA_PULSOS);

    while (1) {
        // Cada medicion es una ventana independiente. Borramos estando parado.
        // Hay un hueco sin contar entre ventanas, durante lectura e impresion.
        ESP_ERROR_CHECK(pcnt_unit_clear_count(contador));
        const int64_t inicio_us = esp_timer_get_time();
        ESP_ERROR_CHECK(pcnt_unit_start(contador));
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_ERROR_CHECK(pcnt_unit_stop(contador));
        const int64_t fin_us = esp_timer_get_time();

        int pulsos = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(contador, &pulsos));
        const int64_t ventana_us = fin_us - inicio_us;
        const double frecuencia_hz = (double)pulsos * 1000000.0 / (double)ventana_us;

        // La ventana real puede durar algo mas de 1 s por el planificador.
        // start/stop tambien introducen un pequeno error en la delimitacion.
        printf("Pulsos: %d | ventana: %" PRId64 " us | frecuencia: %.2f Hz\n",
               pulsos, ventana_us, frecuencia_hz);
    }
}
