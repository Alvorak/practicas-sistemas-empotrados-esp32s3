/*
 * PRACTICA 2 - EJERCICIO 4.1: PWM de 100 Hz, pasos de 20% mediante BOOT.
 * Copia TODO este archivo a main/main.c.
 * LED: GPIO5 -> resistencia limitadora -> anodo LED; catodo -> GND.
 * BOOT = GPIO0. Componentes: esp_driver_gpio, esp_driver_ledc, esp_timer.
 * ledc_stop es parte del MISMO driver LEDC, no una libreria adicional.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"

#define LED GPIO_NUM_5
#define BOOT GPIO_NUM_0
#define PWM_MODO LEDC_LOW_SPEED_MODE
#define PWM_TIMER LEDC_TIMER_0
#define PWM_CANAL LEDC_CHANNEL_0
#define PWM_FRECUENCIA_HZ 100
#define PWM_RESOLUCION LEDC_TIMER_10_BIT
#define CUENTAS_POR_PERIODO (1U << 10)
#define ANTIRREBOTE_US 30000

static void aplicar_porcentaje(unsigned porcentaje)
{
    if (porcentaje == 100) {
        /* 100% debe ser alto permanente. Detenemos la conmutacion y dejamos
         * el nivel de reposo a 1; no enviamos un duty fuera del rango normal.
         * 1023/1024 seria casi 100%, pero todavia tendria un pulso bajo. */
        ESP_ERROR_CHECK(ledc_stop(PWM_MODO, PWM_CANAL, 1));
        printf("Duty=100%% | salida fija a nivel alto\n");
    } else {
        /* Escala de 1024 ticks por periodo. +50 redondea al entero mas cercano.
         * Aqui porcentaje solo es 0,20,40,60,80; duty siempre es <= 1023. */
        const uint32_t duty = (CUENTAS_POR_PERIODO * porcentaje + 50U) / 100U;
        ESP_ERROR_CHECK(ledc_set_duty(PWM_MODO, PWM_CANAL, duty));
        /* Tambien reactiva la salida del canal si venimos de ledc_stop(100%). */
        ESP_ERROR_CHECK(ledc_update_duty(PWM_MODO, PWM_CANAL));
        printf("Duty solicitado=%u%% | duty=%" PRIu32 "/1024\n", porcentaje, duty);
    }
}

void app_main(void)
{
    /* Primero el temporizador; despues el canal que lo utiliza. */
    const ledc_timer_config_t timer = {
        .speed_mode = PWM_MODO,
        .duty_resolution = PWM_RESOLUCION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FRECUENCIA_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    const ledc_channel_config_t canal = {
        .gpio_num = LED,
        .speed_mode = PWM_MODO,
        .channel = PWM_CANAL,
        .timer_sel = PWM_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&canal));

    const gpio_config_t boton = {
        .pin_bit_mask = 1ULL << BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&boton));

    unsigned porcentaje = 0;
    aplicar_porcentaje(porcentaje);
    int ultima_lectura = gpio_get_level(BOOT);
    int estado_estable = ultima_lectura;
    int64_t cambio_us = esp_timer_get_time();

    printf("P2 E4.1: PWM100Hz GPIO5. BOOT: 0-20-40-60-80-100-0.\n");
    while (1) {
        const int lectura = gpio_get_level(BOOT);
        const int64_t ahora_us = esp_timer_get_time();
        if (lectura != ultima_lectura) {
            ultima_lectura = lectura;
            cambio_us = ahora_us;
        }

        /* Confirmamos pulsacion Y liberacion estables durante 30 ms. */
        if (lectura != estado_estable && ahora_us - cambio_us >= ANTIRREBOTE_US) {
            estado_estable = lectura;
            if (estado_estable == 0) {
                porcentaje = (porcentaje == 100) ? 0 : porcentaje + 20;
                aplicar_porcentaje(porcentaje);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
