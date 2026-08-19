cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)
find_package(cmake-harden REQUIRED PATHS node_modules/cmake-harden)

project(bare_bluetooth_linux C CXX)

fetch_package("github:holepunchto/libjstl#ca590d6")

add_library(l2cap STATIC libl2cap/src/linux.c)

harden(l2cap)

set_target_properties(
  l2cap
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(
  l2cap
  PUBLIC
    libl2cap/include
)

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
    l2cap
    ${DBUS_LIBRARIES}
)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(libl2cap/test)
endif()
