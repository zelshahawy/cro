cmake_minimum_required(VERSION 3.24)
project(cro VERSION 0.1.0 LANGUAGES CXX)

# Options for maintainability
option(CRO_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(CRO_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CRO_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Obtain git commit (optional)
execute_process(
  COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT GIT_COMMIT)
  set(GIT_COMMIT "unknown")
endif()

# Configure generated header
configure_file(${CMAKE_SOURCE_DIR}/include/cro/version.hpp.in
               ${CMAKE_BINARY_DIR}/generated/cro/version.hpp @ONLY)

# Warnings
if(MSVC)
  add_compile_options(/W4)
  if(CRO_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor)
  if(CRO_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

# Libraries from vcpkg
include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

find_package(SQLite3 REQUIRED)

# Sanitizers (non‑MSVC)
if(NOT MSVC)
  if(CRO_ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()
  if(CRO_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

# shared logic.
add_library(cro_core
  src/core/config.cpp
)

# Public headers and generated headers
target_include_directories(cro_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/generated)

target_compile_features(cro_core PUBLIC cxx_std_20)

# CLI executable
add_executable(cro
  src/main.cpp
  src/cli.cpp
)

target_link_libraries(cro PRIVATE cro_core)
target_link_libraries(cro_core PUBLIC SQLite::SQLite3)
target_link_libraries(cro PRIVATE cro_core ftxui::screen ftxui::dom ftxui::component)
target_sources(cro PRIVATE src/tui.cpp)
target_sources(cro_core PRIVATE src/core/db.cpp)

target_include_directories(cro PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/generated)

# Install
include(GNUInstallDirs)
install(TARGETS cro RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Tests
include(CTest)
if(BUILD_TESTING)
  add_test(NAME cro_version COMMAND $<TARGET_FILE:cro> --version)
endif()
