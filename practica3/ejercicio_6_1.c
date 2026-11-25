/*
 * Practica 3, ejercicio 6.1: linea UART hasta ENTER, eco y seleccion de color.
 * Copiar TODO a main/main.c. Conectar por el USB-UART de la placa.
 * 115200 baudios, 8 bits, sin paridad, 1 stop; GPIO43 TX y GPIO44 RX.
 * Se conserva la interpretacion del ZIP: una unica clase R/G/B presente.
 * Ejemplos: R -> rojo, HOLA -> sin cambio, GRIS (G y R) -> sin cambio.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "led_strip.h"

#define UART_PUERTO UART_NUM_0
#define UART_TX_GPIO 43
#define UART_RX_GPIO 44
#define LED_RGB_GPIO 48
#define INTENSIDAD 150
#define CAPACIDAD_LINEA 128 // 127 bytes utiles y un '\0' final.

static void escribir_texto(const char *texto)
{
    // uart_write_bytes recibe una longitud en bytes, sin el '\0' de C.
    uart_write_bytes(UART_PUERTO, texto, strlen(texto));
}

static void procesar_linea(const char *linea, led_strip_handle_t led)
{
    escribir_texto("Recibido: ");
    escribir_texto(linea);
    escribir_texto("\r\n");

    const bool rojo = strchr(linea, 'R') != NULL;
    const bool verde = strchr(linea, 'G') != NULL;
    const bool azul = strchr(linea, 'B') != NULL;
    const int colores = rojo + verde + azul;

    // Mayusculas; otras letras no cuentan como colores. RRR sigue siendo rojo.
    // Si hay dos o tres clases de color, no enviamos nada al LED.
    if (colores == 1) {
        ESP_ERROR_CHECK(led_strip_set_pixel(led, 0,
                        rojo ? INTENSIDAD : 0,
                        verde ? INTENSIDAD : 0,
                        azul ? INTENSIDAD : 0));
        ESP_ERROR_CHECK(led_strip_refresh(led));
    }
}

void app_main(void)
{
    uart_config_t configuracion_uart = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // Buffer RX de 1024 bytes; TX=0 significa envio bloqueante sin buffer TX.
    ESP_ERROR_CHECK(uart_driver_install(UART_PUERTO, 1024, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PUERTO, &configuracion_uart));
    ESP_ERROR_CHECK(uart_set_pin(UART_PUERTO, UART_TX_GPIO, UART_RX_GPIO,
                                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

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

    char linea[CAPACIDAD_LINEA];
    size_t usados = 0;
    bool desbordada = false;
    bool anterior_fue_cr = false;
    escribir_texto("\r\nEscribe texto y pulsa ENTER. R=rojo G=verde B=azul; GRIS conserva color.\r\n");

    while (1) {
        uint8_t recibidos[64];
        // El maximo solicitado es siempre el tamano REAL del array destino.
        const int n = uart_read_bytes(UART_PUERTO, recibidos, sizeof(recibidos),
                                      pdMS_TO_TICKS(10));
        for (int i = 0; i < n; ++i) {
            const uint8_t caracter = recibidos[i];

            // ENTER puede llegar como CR, LF o CRLF: este ultimo es un solo ENTER.
            if (caracter == '\n' && anterior_fue_cr) {
                anterior_fue_cr = false;
                continue;
            }
            anterior_fue_cr = caracter == '\r';

            if (caracter == '\r' || caracter == '\n') {
                if (desbordada) {
                    escribir_texto("Linea descartada: maximo 127 bytes; color sin cambios.\r\n");
                } else {
                    linea[usados] = '\0';
                    procesar_linea(linea, led);
                }
                usados = 0;
                desbordada = false;
            } else if (!desbordada) {
                if (caracter == '\b' || caracter == 127) {
                    if (usados > 0) {
                        --usados; // Admitir Backspace/Delete del terminal.
                    }
                } else if (caracter != '\0') {
                    if (usados < sizeof(linea) - 1) {
                        linea[usados++] = (char)caracter;
                    } else {
                        // No ejecutar una orden truncada: descartar hasta ENTER.
                        desbordada = true;
                    }
                }
            }
        }
        // Tambien cedemos CPU si el PC envia datos de manera continua.
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
