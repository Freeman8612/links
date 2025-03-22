cmake_minimum_required(VERSION 3.5)

project(fontlib_spi VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/)
include_directories(${PROJECT_SOURCE_DIR}/W25QXXBV)

add_executable(fontlib ./main.c)

add_library(Flash ${PROJECT_SOURCE_DIR}/W25QXXBV/Flash.c)
add_library(W25Q32BV ${PROJECT_SOURCE_DIR}/W25QXXBV/W25Q32BV.c)
add_library(SPI ${PROJECT_SOURCE_DIR}/spi.c)
add_library(FFBF ${PROJECT_SOURCE_DIR}/ffbf.c)

target_link_libraries(fontlib Flash)
target_link_libraries(fontlib W25Q32BV)
target_link_libraries(fontlib SPI)
target_link_libraries(fontlib FFBF)