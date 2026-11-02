cmake_minimum_required(VERSION 3.10.2)
project("qemu_android_engine")

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Preserve Step 1 diagnostics library
add_library(
    nativediagnostics
    SHARED
    NativeDiagnostics.cpp
)

find_library(
    log-lib
    log
)

target_link_libraries(
    nativediagnostics
    ${log-lib}
)

# Step 2: Native QEMU ARMv7 Engine & JNI Bridge
add_library(
    qemubridge
    SHARED
    QemuBridge.cpp
    qemu/qemu-main.c
    qemu/machine-ranchu.c
    qemu/tcg-core.c
    qemu/cpu-arm.c
    qemu/memory-manager.c
    qemu/mini-glib.c
    qemu/qemu-logger.c
)

target_include_directories(
    qemubridge
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/qemu
)

target_link_libraries(
    qemubridge
    ${log-lib}
    dl
)
