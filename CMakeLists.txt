cmake_minimum_required(VERSION 3.10)
project(as2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L)

add_executable(light_sampler
    src/main.c
    src/hal/adc.c
    src/hal/rotary_encoder.c
    src/hal/led.c
    modules/sampler.c
    modules/dip_detector.c
    modules/udp_server.c
    modules/display.c
    modules/periodTimer.c
)

target_include_directories(light_sampler PRIVATE
    src
    src/hal
    modules
)

target_link_libraries(light_sampler pthread gpiod m)
