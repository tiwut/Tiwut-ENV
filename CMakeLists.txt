cmake_minimum_required(VERSION 3.16)

project(Tiwut-ENV CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(WAYLAND_SERVER REQUIRED wayland-server)

add_executable(tiwut-env main.cpp)

target_include_directories(tiwut-env PRIVATE
    ${WAYLAND_SERVER_INCLUDE_DIRS}
)

target_link_libraries(tiwut-env PRIVATE
    ${WAYLAND_SERVER_LIBRARIES}
)
