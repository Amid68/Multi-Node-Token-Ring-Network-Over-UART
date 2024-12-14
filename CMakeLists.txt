cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_token_ring)

target_include_directories(app PRIVATE subsys/token_management/include)
target_sources(app PRIVATE src/main.c subsys/token_management/src/token_manager.c)