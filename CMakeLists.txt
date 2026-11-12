# =============================================================================
# BCC 5.5 - static recompilation hosts for bcc32.exe and ilink32.exe
#
#   py -3 run_lift.py bcc32 ; py -3 run_lift.py ilink32   # -> src/recomp/<tool>/
#   build.cmd            # vcvarsall x86 + CMake + Ninja -> build/bcc32recomp.exe, build/ilink32recomp.exe
#   py -3 tools/bcc_diff.py   # the conformance harness
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(bcc55 C)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "the host must be 32-bit: use build.cmd (vcvarsall x86)")
endif()
if(NOT MSVC)
  message(FATAL_ERROR "runtime/native32 is MSVC x86 inline assembly")
endif()
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(PCRECOMP "${CMAKE_SOURCE_DIR}/../pcrecomp" CACHE PATH "pcrecomp checkout")
option(BCC_TRACE "build with pcrecomp recomp_trace hooks" OFF)
set(GEN_OPT "/O2" CACHE STRING "optimisation for the generated C")

# Both tools are based at 0x00400000, so each gets its own host.
foreach(tool bcc32 ilink32)
  file(GLOB GEN_${tool} CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/recomp/${tool}/*.c")
  if(NOT GEN_${tool})
    message(FATAL_ERROR "src/recomp/${tool} is empty -- run `py -3 run_lift.py ${tool}` first")
  endif()
  add_executable(${tool}
    src/runtime/host.c
    ${PCRECOMP}/runtime/native32/native32.c
    ${PCRECOMP}/runtime/recomp32/image_loader.c
    ${PCRECOMP}/runtime/recomp32/recomp_trace.c
    ${GEN_${tool}})
  target_include_directories(${tool} PRIVATE
    src/runtime src/recomp/${tool} ${PCRECOMP}/runtime/native32 ${PCRECOMP}/runtime/recomp32)
  target_compile_definitions(${tool} PRIVATE BCC_TOOL="${tool}" RECOMP_RETADDR=0u
                             _CRT_SECURE_NO_WARNINGS RECOMP_FLAT_MEMORY RECOMP_LOCAL_REGS)
  # Not large-address-aware, like the originals: ilink32 looks for the
  # largest free region below 0xFFFF0000 and aligns its base up by powers of
  # two; with 4 GB visible that region sits above 2 GB, the power of two
  # wraps to 0 and the loop never ends (0x0040D6CA).
  target_link_options(${tool} PRIVATE /BASE:0x60000000 /FIXED:NO /DYNAMICBASE:NO
                      /NXCOMPAT /LARGEADDRESSAWARE:NO /STACK:0x1000000 /MANIFESTUAC:NO /MANIFEST:NO)
  set_source_files_properties(${GEN_${tool}} TARGET_DIRECTORY ${tool} PROPERTIES COMPILE_OPTIONS "/w;${GEN_OPT}")
  target_link_libraries(${tool} PRIVATE user32 gdi32 kernel32 advapi32 ole32)
  set_target_properties(${tool} PROPERTIES OUTPUT_NAME ${tool}recomp)
  if(BCC_TRACE)       # every lifted entry through recomp_trace (BCC55_DIAG)
    target_compile_definitions(${tool} PRIVATE RECOMP_TRACE=1)
  endif()
endforeach()
set_source_files_properties(src/runtime/host.c TARGET_DIRECTORY bcc32 ilink32 PROPERTIES COMPILE_OPTIONS "/W3")
