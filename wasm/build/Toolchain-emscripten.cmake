# Ensure Emscripten cache resolves inside the workspace (no global write perms needed)
if(NOT DEFINED ENV{EM_CACHE})
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.emcache")
    set(ENV{EM_CACHE} "${CMAKE_CURRENT_LIST_DIR}/../.emcache")
endif()

# Emscripten toolchain file for FastTracker 2 WASM build
set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

# Specify the cross compiler
set(CMAKE_C_COMPILER "emcc")
set(CMAKE_CXX_COMPILER "em++")
set(CMAKE_AR "emar" CACHE FILEPATH "Emscripten ar")
set(CMAKE_RANLIB "emranlib" CACHE FILEPATH "Emscripten ranlib")

# Set target environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# SDL2 port: needed at compile time (headers) and at link time (library).
# All other link options live in CMakeLists.txt.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sUSE_SDL=2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sUSE_SDL=2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sUSE_SDL=2")

# Set output format
set(CMAKE_EXECUTABLE_SUFFIX ".js")
