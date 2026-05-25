cmake_minimum_required(VERSION 3.16)
project(rice_cooker_sim C)

set(CMAKE_C_STANDARD 11)

# ---------- LVGL source path ----------
# Adjust this path to your local LVGL v9.x source tree
if(NOT DEFINED LVGL_PATH)
    # Try common locations
    if(EXISTS "${CMAKE_SOURCE_DIR}/../../../lvgl")
        set(LVGL_PATH "${CMAKE_SOURCE_DIR}/../../../lvgl")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/../../lvgl")
        set(LVGL_PATH "${CMAKE_SOURCE_DIR}/../../lvgl")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/lvgl")
        set(LVGL_PATH "${CMAKE_SOURCE_DIR}/lvgl")
    else()
        message(FATAL_ERROR "LVGL_PATH not set. Use -DLVGL_PATH=<path-to-lvgl-source>")
    endif()
endif()
message(STATUS "LVGL_PATH = ${LVGL_PATH}")

# ---------- SDL2 ----------
# Try find_package first; fall back to pkg-config or manual
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 REQUIRED sdl2)
    else()
        # Manual fallback: assume SDL2 in compiler's default paths
        set(SDL2_INCLUDE_DIRS "")
        set(SDL2_LIBRARIES mingw32 SDL2main SDL2)
    endif()
endif()
message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")

# ---------- LVGL library ----------
# We build LVGL as part of this project
file(GLOB_RECURSE LVGL_SRCS "${LVGL_PATH}/src/*.c")
add_library(lvgl STATIC ${LVGL_SRCS})
target_include_directories(lvgl PUBLIC
    "${LVGL_PATH}"
    "${CMAKE_SOURCE_DIR}"          # picks up lv_conf.h
)
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE
)

# ---------- UI source files (shared with target) ----------
set(UI_DIR "${CMAKE_SOURCE_DIR}/../ui_files")
file(GLOB UI_SRCS "${UI_DIR}/*.c")
# Exclude hardware-dependent files (stubs provided in sim_stubs.c)
list(FILTER UI_SRCS EXCLUDE REGEX "ui_buzzer\\.c$")
list(FILTER UI_SRCS EXCLUDE REGEX "ui_clock\\.c$")
list(FILTER UI_SRCS EXCLUDE REGEX "ui_keypad\\.c$")

# ---------- Simulator sources ----------
set(SIM_SRCS
    main.c
    sim_stubs.c
)

add_executable(rice_cooker_sim ${SIM_SRCS} ${UI_SRCS})

target_include_directories(rice_cooker_sim PRIVATE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/.."
    "${UI_DIR}"
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(rice_cooker_sim PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    SIMULATOR=1
)

target_link_libraries(rice_cooker_sim PRIVATE lvgl ${SDL2_LIBRARIES})
