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

# Helpful status if a toolchain is in use (e.g., vcpkg)
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Using toolchain: ${CMAKE_TOOLCHAIN_FILE}")
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

# --- Target-scoped warnings (don’t affect third-party deps) ---
function(cro_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 $<$<BOOL:${CRO_WARNINGS_AS_ERRORS}>:/WX>)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
      $<$<BOOL:${CRO_WARNINGS_AS_ERRORS}>:-Werror>
    )
  endif()
endfunction()

# --- Target-scoped sanitizers (non-MSVC) ---
function(cro_apply_sanitizers target)
  if(MSVC)
    return()
  endif()
  if(CRO_ENABLE_ASAN)
    target_compile_options(${target} PRIVATE -fsanitize=address)
    target_link_options(${target} PRIVATE -fsanitize=address)
  endif()
  if(CRO_ENABLE_UBSAN)
    target_compile_options(${target} PRIVATE -fsanitize=undefined)
    target_link_options(${target} PRIVATE -fsanitize=undefined)
  endif()
endfunction()

# --- Dependencies from vcpkg ---
# Use vcpkg manifests (vcpkg.json) to install: ftxui, sqlite3
# Make sure you do NOT also FetchContent FTXUI at the same time.
find_package(ftxui CONFIG REQUIRED)                 # ftxui::screen ftxui::dom ftxui::component

# SQLite can be either the official CMake package or vcpkg’s unofficial one.
# Prefer the official; fall back to unofficial if needed.
find_package(SQLite3 QUIET)
if(TARGET SQLite::SQLite3)
  set(CRO_SQLITE_TARGET SQLite::SQLite3)
else()
  find_package(unofficial-sqlite3 CONFIG REQUIRED)
  set(CRO_SQLITE_TARGET unofficial::sqlite3::sqlite3)
endif()

# --- Core library (shared logic) ---
add_library(cro_core
  src/core/config.cpp
  src/core/db.cpp
  src/core/exercise.cpp
)

target_include_directories(cro_core
  PUBLIC ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/generated)

target_compile_features(cro_core PUBLIC cxx_std_20)

target_link_libraries(cro_core PUBLIC ${CRO_SQLITE_TARGET})

cro_apply_warnings(cro_core)
cro_apply_sanitizers(cro_core)

# --- CLI executable ---
add_executable(cro
  src/main.cpp
  src/cli.cpp
  src/tui.cpp
)

target_link_libraries(cro PRIVATE cro_core ftxui::component)

target_include_directories(cro PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/generated)

cro_apply_warnings(cro)
cro_apply_sanitizers(cro)

# --- Install ---
include(GNUInstallDirs)
install(TARGETS cro RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# --- Tests ---
include(CTest)
if(BUILD_TESTING)
  add_test(NAME cro_version COMMAND $<TARGET_FILE:cro> --version)
endif()
