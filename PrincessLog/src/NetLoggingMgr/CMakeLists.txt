cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{DOLCESDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{DOLCESDK}/share/dolce.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(NetLoggingMgr LANGUAGES C)
include("$ENV{DOLCESDK}/share/dolce.cmake" REQUIRED)

add_subdirectory(kernel_module)
add_subdirectory(settings_app)
