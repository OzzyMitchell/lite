find_package(PNG 1.6 REQUIRED)
find_package(TIFF 4.7 REQUIRED)
find_path(QLIC_LITE_TURBOJPEG_INCLUDE turbojpeg.h REQUIRED)
find_library(QLIC_LITE_TURBOJPEG_LIBRARY NAMES turbojpeg REQUIRED)
find_path(QLIC_LITE_WEBP_INCLUDE webp/decode.h REQUIRED)
find_library(QLIC_LITE_WEBP_LIBRARY NAMES webpdecoder webp REQUIRED)
find_path(QLIC_LITE_JXL_INCLUDE jxl/decode.h REQUIRED)
find_library(QLIC_LITE_JXL_LIBRARY NAMES jxl_dec jxl REQUIRED)

function(qli_cli_image_io target)
    target_compile_definitions(${target} PRIVATE QLIC_LITE_IMAGE_IO)
    target_include_directories(${target} SYSTEM PRIVATE
        "${QLIC_LITE_TURBOJPEG_INCLUDE}" "${QLIC_LITE_WEBP_INCLUDE}" "${QLIC_LITE_JXL_INCLUDE}")
    target_link_libraries(${target} PRIVATE PNG::PNG TIFF::TIFF
        "${QLIC_LITE_TURBOJPEG_LIBRARY}" "${QLIC_LITE_WEBP_LIBRARY}" "${QLIC_LITE_JXL_LIBRARY}")
endfunction()
