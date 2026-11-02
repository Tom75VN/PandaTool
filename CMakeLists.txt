cmake_minimum_required(VERSION 3.20)
project(PandaTool LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "PandaTool must be built for Win32 (32-bit): cmake -A Win32")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(minhook STATIC
    external/minhook/src/buffer.c
    external/minhook/src/hook.c
    external/minhook/src/trampoline.c
    external/minhook/src/hde/hde32.c
)
target_include_directories(minhook PUBLIC external/minhook/include)

add_library(PandaTool SHARED
    src/dllmain.cpp
    src/engine.cpp
    src/api.cpp
)
target_link_libraries(PandaTool PRIVATE minhook)
target_compile_definitions(PandaTool PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(PandaTool PRIVATE /W4 /permissive-)
set_target_properties(PandaTool PROPERTIES OUTPUT_NAME PandaTool)
