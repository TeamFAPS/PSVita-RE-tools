cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME eboot)
project(NetLoggingMgrSettings)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "NetLoggingMgr Settings")
set(VITA_TITLEID  "NLMS00000")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# Builds
function(ADD_RESOURCES out_var)
  set(result)
  foreach(in_f ${ARGN})
    set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${in_f}.o")
    get_filename_component(out_dir ${out_f} DIRECTORY)
    add_custom_command(OUTPUT ${out_f}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
      COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building resource ${out_f}"
      VERBATIM
      )
    list(APPEND result ${out_f})
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

file(GLOB res_files RELATIVE ${CMAKE_SOURCE_DIR} *.gxp)
add_resources(gxp_res ${res_files})

add_executable(${PROJECT_NAME}
  ${gxp_res}
  src/main.c
  src/gxm.c
)

target_link_libraries(${PROJECT_NAME}
 # ${CMAKE_CURRENT_BINARY_DIR}/../NetLoggingMgrKernel_new/stubs/libNetLoggingMgr_stub.a
  NetLoggingMgr_stub
  taihen_stub
  SceDisplay_stub
  SceCtrl_stub
  SceGxm_stub
  SceAppUtil_stub
  SceCommonDialog_stub
  SceSysmodule_stub
  SceNet_stub
  ScePower_stub
  SceVshBridge_stub
)

vita_create_self(eboot.bin ${PROJECT_NAME}
  UNSAFE
)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
