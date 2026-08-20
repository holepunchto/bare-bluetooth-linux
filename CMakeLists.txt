cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-harden REQUIRED PATHS node_modules/cmake-harden)

project(bare_bluetooth_linux C CXX)

fetch_package("github:holepunchto/libjstl#ca590d6")

enable_testing()

set(L2CAP_ENABLE_TESTS ON)

add_subdirectory(libl2cap)

add_bare_module(bare_bluetooth_linux)

harden(${bare_bluetooth_linux})

target_sources(
  ${bare_bluetooth_linux}
  PRIVATE
    binding.cc
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

target_include_directories(
  ${bare_bluetooth_linux}
  PRIVATE
    ${DBUS_INCLUDE_DIRS}
)

target_link_libraries(
  ${bare_bluetooth_linux}
  PRIVATE
    jstl
    l2cap_static
    ${DBUS_LIBRARIES}
)
