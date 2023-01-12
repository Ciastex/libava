cmake_minimum_required(VERSION 3.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "You're not using VitaSDK, chief.")
    endif()
endif()

project("ava" VERSION 1.0)

add_library(${PROJECT_NAME} STATIC
    "src/netdbg.c"
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    PUBLIC_HEADER "src/netdbg.h;include/ava.h"
)

install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION "include/ava")