set(ELF net_logging_mgr.elf)
set(SELF net_logging_mgr.skprx)
set(LIB NetLoggingMgr_stub)
set(LIB_FILE "lib${LIB}.a")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -Wall -Wextra -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

add_executable("${ELF}"
  src/main.c
  src/ringbuf.c
)

target_include_directories("${ELF}"
  PRIVATE "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries("${ELF}"
  gcc

  taihenForKernel_stub
  taihenModuleUtils_stub
  SceSysmemForDriver_stub
  SceSysclibForDriver_stub
  SceSysrootForKernel_stub
  SceKernelUtilsForDriver_stub

  SceNetPsForDriver_stub
  SceSblSsMgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceSblACMgrForDriver_stub

  SceQafMgrForDriver_stub

  SceModulemgrForDriver_stub
  SceIofilemgrForDriver_stub

  SceCtrlForDriver_stub
  SceDebugForDriver_stub
)

dolce_create_self("${SELF}"
  "${ELF}"
  CONFIG exports.yml
  UNSAFE
)

dolce_create_stubs(stubs
  "${ELF}"
  exports.yml
  KERNEL
)

add_library("${LIB}" STATIC IMPORTED GLOBAL)

add_dependencies("${LIB}" stubs)

set_target_properties("${LIB}" PROPERTIES
  IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/stubs/${LIB_FILE}"
)
