cmake_minimum_required(VERSION 3.25)
project(QLICLite VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(EMSCRIPTEN)
    add_executable(qlic_lite_web qlic_lite_decoder.c)
    target_compile_definitions(qlic_lite_web PRIVATE QLIC_WASM QLI_MAX_CPU_TIER=0)
    set_target_properties(qlic_lite_web PROPERTIES OUTPUT_NAME qlic_lite SUFFIX ".mjs")
    target_link_options(qlic_lite_web PRIVATE
        --no-entry -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node
        -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=536870912 -sSTACK_SIZE=1048576
        -sEXPORTED_RUNTIME_METHODS=HEAPU8
        -sEXPORTED_FUNCTIONS=_qlic_lite_limits_default,_qlic_lite_decode,_qlic_lite_image_free,_qlic_lite_wasm_decode,_qlic_lite_wasm_samples,_qlic_lite_wasm_size,_qlic_lite_wasm_width,_qlic_lite_wasm_height,_qlic_lite_wasm_bits,_qlic_lite_wasm_channels,_qlic_lite_wasm_free,_malloc,_free
    )
    return()
endif()

add_executable(qlic_lite_full qlic_lite_full.c)
set_target_properties(qlic_lite_full PROPERTIES OUTPUT_NAME qlic_lite)
add_library(qlic_lite SHARED qlic_lite.c)
add_library(qlic_lite_decoder SHARED qlic_lite_decoder.c)
add_executable(qlic_lite_cli_adapter qlic_lite_cli_adapter.c)
target_link_libraries(qlic_lite_cli_adapter PRIVATE qlic_lite)

option(QLIC_LITE_IMAGE_IO "Build CLI importers and exporters" ON)
if(QLIC_LITE_IMAGE_IO)
    include(cmake/image-libraries.cmake)
    qli_cli_image_io(qlic_lite_full)
    qli_cli_image_io(qlic_lite_cli_adapter)
endif()

foreach(library IN ITEMS qlic_lite qlic_lite_decoder)
    target_include_directories(${library} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${library} PRIVATE QLIC_LITE_BUILD_SHARED)
    set_target_properties(${library} PROPERTIES C_VISIBILITY_PRESET hidden)
endforeach()
target_compile_definitions(qlic_lite_decoder INTERFACE QLIC_LITE_DECODER_ONLY)

if(NOT WIN32)
    find_package(Threads REQUIRED)
    foreach(target IN ITEMS qlic_lite_full qlic_lite qlic_lite_decoder)
        target_link_libraries(${target} PRIVATE Threads::Threads m)
    endforeach()
endif()

if(MSVC)
    foreach(target IN ITEMS qlic_lite_full qlic_lite qlic_lite_decoder qlic_lite_cli_adapter)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${target} PRIVATE /Gy /Gw)
        target_link_options(${target} PRIVATE /OPT:REF)
    endforeach()
elseif(MINGW)
    target_link_options(qlic_lite_full PRIVATE -municode)
    target_link_options(qlic_lite_cli_adapter PRIVATE -municode)
endif()
