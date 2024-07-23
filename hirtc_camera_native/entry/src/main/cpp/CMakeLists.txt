# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)
project(hirtc_camera_demo)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

include_directories(${NATIVERENDER_ROOT_PATH}/libyuv/include)

add_library(entry SHARED napi_init.cpp ohos_camera.cpp ohos_render.cpp ohos_audio.cpp wav_wrapper.cpp)

#target_link_libraries(entry PUBLIC libace_napi.z.so)
target_link_libraries(entry PUBLIC libace_napi.z.so libhilog_ndk.z.so libohaudio.so libohcamera.so libimage_ndk.z.so libimage_receiver_ndk.z.so libnative_image.so)
target_link_libraries(entry PUBLIC libohimage.so libimage_receiver.so libnative_buffer.so)
target_link_libraries(entry PUBLIC ${NATIVERENDER_ROOT_PATH}/libyuv/lib/arm64/libyuv.a)
target_link_libraries(entry PUBLIC libace_ndk.z.so libnative_window.so)
