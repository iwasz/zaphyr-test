# SPDX-License-Identifier: Apache-2.0

# set (BOARD nucleo_h743zi)
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(cpp_synchronization)

target_sources(app PRIVATE 
    src/main.c
    src/dhcp.c
)
