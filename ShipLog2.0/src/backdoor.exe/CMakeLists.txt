cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(backdoor_exe)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
	backdoor_exe.c
	cj_net.c
	utils.c
)

target_link_libraries(${PROJECT_NAME}
	gcc
	taihenForKernel_stub
	SceStdio_stub
	SceIofilemgrForDriver_stub
	SceSysclibForDriver_stub
	SceSysmemForDriver_stub
	SceSysmemForKernel_stub
	SceThreadmgrForDriver_stub
	SceIofilemgr_stub
	SceNetPsForDriver_stub
	SceDebugForKernel_stub
	SceDebugForDriver_stub
	taihenModuleUtils_stub
)
vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/exports.yml
)
vita_create_stubs(${PROJECT_NAME}-stubs ${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/exports.yml
  KERNEL
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)

install(FILES backdoor_exe.h
  DESTINATION include
)
