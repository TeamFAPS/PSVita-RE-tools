set(ELF net_logging_mgr_settings.elf)
set(SELF eboot.bin)
set(VPK NetLoggingMgrSettings.vpk)
set(VITA_APP_NAME "NetLoggingMgr Settings")
set(VITA_TITLEID  "NLMS00000")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3 -std=gnu99")

function(ADD_RESOURCES out_var)
  set(result)
  foreach(in_f ${ARGN})
    set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${in_f}.o")
    get_filename_component(out_dir ${out_f} DIRECTORY)
    add_custom_command(OUTPUT ${out_f}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
      COMMAND ${CMAKE_LINKER} -r -b binary -o ${out_f} ${in_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Building resource ${out_f}"
      VERBATIM
      )
    list(APPEND result ${out_f})
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

file(GLOB res_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.gxp)
add_resources(gxp_res ${res_files})

add_executable("${ELF}"
  ${gxp_res}
  src/main.c
  src/gxm.c
)

target_include_directories("${ELF}"
  PRIVATE "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries("${ELF}"
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

dolce_create_self("${SELF}"
  "${ELF}"
  UNSAFE
)

dolce_create_vpk("${VPK}"
  "${VITA_TITLEID}"
  "${SELF}"
  VERSION "${VITA_VERSION}"
  NAME "${VITA_APP_NAME}"
)
