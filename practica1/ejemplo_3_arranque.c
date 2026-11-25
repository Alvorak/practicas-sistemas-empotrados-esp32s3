/* Practica 1, apartado 3: comprobar compilacion y terminal serie.
 * Copia este archivo completo en main/main.c.
 * El while vacio del PDF se sustituye por una espera que cede la CPU.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Inicio del programa: ESP32-S3 y ESP-IDF.\n");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
