cmake_minimum_required(VERSION 3.16)

# Se construye main y sus dependencias. Los ejemplos de practica1/2/3
# son archivos de estudio: NO se compilan todos a la vez.
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(practicas_sistemas_empotrados_esp32s3)
