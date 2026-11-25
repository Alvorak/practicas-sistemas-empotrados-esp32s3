# Solo main.c proporciona app_main(). Para cambiar de ejercicio, reemplaza
# su contenido por el de uno de los .c de las carpetas practicaX.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        freertos
        esp_common
        esp_system
        esp_hw_support
        esp_timer
        esp_driver_gpio
        esp_driver_gptimer
        esp_driver_ledc
        esp_driver_pcnt
        esp_driver_uart
        esp_adc
        espressif__led_strip
)
