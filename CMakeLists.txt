cmake_minimum_required(VERSION 3.16)

project(Tiwut-ENV CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(WLR_USE_UNSTABLE)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)
pkg_check_modules(WLROOTS REQUIRED wlroots-0.18)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_check_modules(WAYLAND_SCANNER REQUIRED wayland-scanner)

pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
pkg_get_variable(WAYLAND_SCANNER_BIN wayland-scanner wayland_scanner)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/xdg-shell-protocol.h
    COMMAND ${WAYLAND_SCANNER_BIN} server-header ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml ${CMAKE_BINARY_DIR}/xdg-shell-protocol.h
)

add_executable(tiwut-env main.cpp ${CMAKE_BINARY_DIR}/xdg-shell-protocol.h)

target_include_directories(tiwut-env PRIVATE
    ${WAYLAND_SERVER_INCLUDE_DIRS}
    ${WLROOTS_INCLUDE_DIRS}
    ${XKBCOMMON_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(tiwut-env PRIVATE
    ${WAYLAND_SERVER_LIBRARIES}
    ${WLROOTS_LIBRARIES}
    ${XKBCOMMON_LIBRARIES}
)
